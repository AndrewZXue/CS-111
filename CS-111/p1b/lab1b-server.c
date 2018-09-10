#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <termios.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <mcrypt.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
//#define CRLF {'\r', '\n'}

//Global Variables
int pipe1_fd[2];
int pipe2_fd[2];
struct pollfd poll_fd[2];
int s_fg = 0;
pid_t child_pid = -1;
MCRYPT mcpt_fd;


void encryption_init()
{
	char* key;
	char* IV;
	int key_len = 16;
	key = calloc(1, key_len);
	int key_fd = open("my.key", O_RDONLY);
	if (key_fd < 0)
		fprintf(stderr, "Error: Cannot Open Key File.\n");
	read(key_fd, key, key_len);
	mcpt_fd = mcrypt_module_open("twofish", NULL, "cfb", NULL);
	if (mcpt_fd == MCRYPT_FAILED)
	{
		fprintf(stderr, "Error: Cannot initiate encryption.\n");
		exit(1);
	}
	IV = malloc(mcrypt_enc_get_iv_size(mcpt_fd));
	int i;
	for (i = 0; i != mcrypt_enc_get_iv_size(mcpt_fd); i++)
		IV[i] = rand();
	int fin = mcrypt_generic_init(mcpt_fd, key, key_len, IV);
	if (fin < 0)
	{
		mcrypt_perror(fin);
		fprintf(stderr, "%s\n", "Error: Cannot generate initialization.");
		exit(1);
	}
	free(key);
	free(IV);
}

//signal handler
void sig_handler(int sig)
{
	if(s_fg == 1 && sig == SIGINT)
	{
		kill (child_pid, SIGINT);
		exit(1);
	}
	if(sig == SIGPIPE)
	{
		exit(1);
	}
}

int main(int argc, char* argv[]){

	int port;
	//char s_buffer[256];
	//int in_fd = 0;
	//int out_fd = 1;

	if(argc < 2)
	{
		fprintf(stderr, "%s\n", "Error: NO port number in argumet.");
		exit(1);
	}

	static struct option long_ops[] = {
		{"port", required_argument, NULL, 'p'},
		{"encrypt", required_argument, NULL, 'e'},
		{0, 0, 0, 0}
	};

	int encrypt_flag = 0;
	int ret = 0;
	while ((ret = getopt_long(argc, argv, "p:e", long_ops, NULL)) != -1)
	{
		switch(ret)
		{
			case 'p':
				port = atoi(optarg);
				break;
			case 'e':
				encrypt_flag = 1;
				encryption_init();
				break;
			default:
				fprintf(stderr, "Unrecoganized Argument.\n");
				exit(1);
				break;
		}
	}

	int sock_fd;
	int new_sock_fd;
	struct sockaddr_in serv_addr, cli_addr;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0)
	{
		printf("Cannot create socket.\n");
		exit(1);
	}
	memset((void *) &serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	if(bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Error: Binding Error.\n");
		exit(1);
	}

	listen(sock_fd, 5);
	socklen_t client_len = sizeof(cli_addr);
	new_sock_fd = accept(sock_fd, (struct sockaddr *) &cli_addr, &client_len);
	if(new_sock_fd < 0)
	{
		perror("Error: Acception Failed.\n");
		exit(1);
	}
	//for --shell option
	//dup2(new_sock_fd, 0);
	//dup2(new_sock_fd, 1);
	//dup2(new_sock_fd, 2);

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
			poll_fd[0].fd = new_sock_fd; 		//poll_fd[0] stands for socket input
			poll_fd[1].fd = pipe2_fd[0]; //poll_fd[1] stands for read from shell
			poll_fd[0].events = POLLIN;
			poll_fd[1].events = POLLIN;

			close(pipe1_fd[0]);
			close(pipe2_fd[1]);

			for (;;)
			{
				if (poll(poll_fd, 2, 0) > 0)
				{
					if(poll_fd[0].revents & POLLIN)  //if read from socket, both output and write to pipe
					{
						char buffer[1];
						read(new_sock_fd, buffer, 1);
						if (encrypt_flag)
							mdecrypt_generic(mcpt_fd, buffer, 1);
						switch(buffer[0])
						{
							case 0x0D:
							case 0x0A:
							{
								char lf_temp[1] = {'\n'};
								write(pipe1_fd[1], lf_temp, 1);
								break;
							}
							case 0x04:
							{
								close(pipe1_fd[1]);
								close(new_sock_fd);
								break;
							}
							case 0x03:
							{
								kill(child_pid, SIGINT);
								close(new_sock_fd);
								break;
							}
							default:
							{
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
							//case 0x0D:
							//case 0x0A:
							//{
							//	char temp[2] = {'\r','\n'};
							//	if (encrypt_flag)
							//		mcrypt_generic(mcpt_fd, temp, 2);
							//	write(new_sock_fd, temp, 2);
							//	break;
							//}
							default:
							{
								if (encrypt_flag)
									mcrypt_generic(mcpt_fd, buffer, 1);
								write(new_sock_fd, buffer/*[i+1]*/, 1);
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
		mcrypt_generic_deinit(mcpt_fd);
		mcrypt_module_close(mcpt_fd);
		exit(0);
}

