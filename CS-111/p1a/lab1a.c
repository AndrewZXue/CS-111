
#include<stdio.h>
#include<stdlib.h>
#include<getopt.h>
#include<termios.h>
#include<unistd.h>
#include<sys/poll.h>
#include<sys/wait.h>
#include<sys/ioctl.h>
#include<sys/types.h>
#include<signal.h>

//#define CRLF {'\r', '\n'}

//Global Variables
int pipe1_fd[2];
int pipe2_fd[2];
struct pollfd poll_fd[2];
int s_fg = 0;
pid_t child_pid = -1;

struct termios saved_attributes;

void reset_input_mode (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void set_input_mode(void)
{
	struct termios tattr;

  if (!isatty (STDIN_FILENO))
    {
      fprintf (stderr, "Not a terminal.\n");
      exit (EXIT_FAILURE);
    }

  tcgetattr (STDIN_FILENO, &saved_attributes);
  atexit (reset_input_mode);

//instructed as the spec says
  tcgetattr (STDIN_FILENO, &tattr);
  	tattr.c_iflag = ISTRIP;	/* only lower 7 bits	*/
	tattr.c_oflag = 0;		/* no processing	*/
	tattr.c_lflag = 0;		/* no processing	*/	
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

//signal handler
void sig_handler(int sig)
{
	if(s_fg == 1 && sig == SIGINT)
	{
		kill (child_pid, SIGINT);
		reset_input_mode();
		exit(1);
	}
	if(sig == SIGPIPE)
	{
		reset_input_mode();
		exit(1);
	}
}

int main(int argc, char* argv[]){

	int in_fd = 0;
	//int out_fd = 1;

	//for --shell option
	static struct option long_ops[] = {
		{"shell", no_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

	set_input_mode();

	int ret = 0;
	while ((ret = getopt_long(argc, argv, "s", long_ops, NULL)) != -1)
	{
		switch(ret)
		{
			case 's':;
				signal(SIGINT, sig_handler);
				signal(SIGPIPE, sig_handler);
				s_fg = 1;
				break;
			default:
				fprintf(stderr, "Illegal Option.\n");
				reset_input_mode();
				exit(1);
				break;
		}
	}

	if(s_fg == 0){		//without --shell option
		char buffer[1024];
		ssize_t rret = read(in_fd, buffer, 1024);
		while(rret > 0)
		{
			int i;
			for(i = 0; i != rret; i++)
			{
				switch(buffer[i])
				{
					case 0x0D:
					case 0x0A:
					{
						char temp[2] = {'\r', '\n'};
						write(1, temp, 2);
						break;
					}
					case 0x04:
					{
						exit(0);
						break;
					}
					default:
					{
						write(1, buffer+i, 1);
						break;
					}
				}
			}
			rret = read(in_fd, buffer, 1024);
		}
	}

	//with --shell option
	else if (s_fg == 1){
		if (pipe(pipe1_fd) == -1) {			//parent->child
         	perror("Pipe Error");
         	exit(1);
     	 }
     	if (pipe(pipe2_fd) == -1) {			//child->parent
        	perror("Pipe Error");
         	exit(1);
     	 }

		//create a child process
		child_pid = fork();
		if (child_pid < 0)
		{
			perror("fork failure");
			exit(1);
		}
		else if (child_pid == 0)	//child process
		{
			close(pipe1_fd[1]);
			close(pipe2_fd[0]);
			dup2(pipe1_fd[0], STDIN_FILENO);
			dup2(pipe2_fd[1], STDOUT_FILENO);
			close(pipe1_fd[0]);
			close(pipe2_fd[1]);

			char *execvp_argv[2];
			char execvp_filename[] = "/bin/bash";
			execvp_argv[0] = execvp_filename;
			execvp_argv[1] = NULL;
			if (execvp(execvp_filename, execvp_argv) == -1) 
			{
				perror ("execvp() error.\n");
				kill (child_pid, SIGPIPE);
				exit(1);
			}
		}
		else						//parent process
		{
			poll_fd[0].fd = 0; 		//poll_fd[0] stands for stdin
			poll_fd[1].fd = pipe2_fd[0]; //poll_fd[1] stands for read from shell
			poll_fd[0].events = POLLIN;
			poll_fd[1].events = POLLIN;

			close(pipe1_fd[0]);
			close(pipe2_fd[1]);

			for (;;)
			{
				if (poll(poll_fd, 2, 0) > 0)
				{
					if(poll_fd[0].revents & POLLIN)  //if read from stdin, both output and write to pipe
					{
						char buffer[1];
						read(in_fd, buffer, 1);
						switch(buffer[0])
						{
							case 0x0D:
							case 0x0A:
							{
								char temp[2] = {'\r', '\n'};
								char lf_temp[1] = {'\n'};
								write(1, temp, 2);
								write(pipe1_fd[1], lf_temp, 1);
								break;
							}
							case 0x04:
							{
								close(pipe1_fd[1]);
								break;
							}
							case 0x03:
							{
								kill(child_pid, SIGINT);
								break;
							}
							default:
							{
								write(1, buffer/*[i+1]*/, 1);
								write(pipe1_fd[1], buffer, 1);
								break;
							}
						}
					}


					if(poll_fd[1].revents & POLLIN) //if read from shell, only output
					{
						char buffer[1];
						read(pipe2_fd[0], buffer, 1);
						switch(buffer[0])
						{
							case 0x0D:
							case 0x0A:
							{
								char temp[2] = {'\r','\n'};
								write(1, temp, 2);
								break;
							}
							default:
							{
								write(1, buffer/*[i+1]*/, 1);
								break;
							}
						}
					}

					if(poll_fd[0].revents & (POLLHUP + POLLERR))	//if keyboard input no longer has anything to read
					{
						perror("keyboard input error.\n");
						close(pipe1_fd[1]);
						close(pipe2_fd[0]);
						exit(1);
					}

					if(poll_fd[1].revents & (POLLHUP + POLLERR)) 	//if shell no long has anything to read
					{
						pid_t wait_pid;
						int state;
						close(pipe1_fd[1]);
						close(pipe2_fd[0]);
						wait_pid = waitpid(child_pid, &state, WNOHANG);
						if (wait_pid < 0)
						{
							perror("waitpid\n");
							exit(1);
						}
						else
						{
							fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(state), WEXITSTATUS(state));
							exit(0);
						}
					}
				}
			}
		}
	}
}




