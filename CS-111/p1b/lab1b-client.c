#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <mcrypt.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

struct pollfd poll_fd[2];
struct termios saved_attributes;
MCRYPT mcpt_fd;

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

int main(int argc, char* argv[]){
	int port;
	//char buffer[256];
	int in_fd = 0;
	//int out_fd = 1;

	//if(argc < 3)
	//{
//		fprintf(stderr, "%s\n", "Error: NO port number in argumet.");
//		exit(1);
//	}

	static struct option long_ops[] = {
		{"port", required_argument, NULL, 'p'},
		{"log", required_argument, NULL, 'l'},
		{"encrypt", required_argument, NULL, 'e'},
		{0, 0, 0, 0}
	};

	

	int encrypt_flag = 0;
	int ret = 0;
	int log_sig = 0;
	int log_fd = -1;
	while ((ret = getopt_long(argc, argv, "p:l:e", long_ops, NULL)) != -1)
	{
		switch(ret)
		{
			case 'p':
				port = atoi(optarg);
				break;
			case 'l':
				log_sig = 1;
				log_fd = creat(optarg, S_IRUSR | S_IWUSR);
				if (log_fd < 0)
				{
					fprintf(stderr, "Cannot create log file.\n");
					perror(0);
					exit(1);
				}
				break;
			case 'e':
				encrypt_flag = 1;
				encryption_init();
				break;
			default:
				fprintf(stderr, "Unrecoganized Argument.\n");
				reset_input_mode();
				exit(1);
				break;
		}
	}
	
	int sock_fd;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	//char s_buffer[256];

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd<0)
	{
		fprintf(stderr, "Error: Cannot open socket.\n");
		exit(1);
	}
	server = gethostbyname("localhost");
	if (server == NULL)
	{
		fprintf(stderr, "Error: Host not found.\n");
		exit(0);
	}

	memset((void *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	memcpy((char *) &serv_addr.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
	if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		fprintf(stderr, "Error: Connection failed\n");
		exit(1);
	}

	poll_fd[0].fd = in_fd; 		//poll_fd[0] stands for socket input
	poll_fd[1].fd = sock_fd; //poll_fd[1] stands for read from socket
	poll_fd[0].events = POLLIN;
	poll_fd[1].events = POLLIN;

	//close(pipe1_fd[0]);
	//close(pipe2_fd[1]);
	set_input_mode();
	for (;;)
	{
				if (poll(poll_fd, 2, 0) < 0)
				{
					fprintf(stderr, "Cannot create poll.\n");
					exit(1);
				}
				else
				{
					if(poll_fd[0].revents & POLLIN)  //if read from stdin, write to socket
					{
						char buffer[1];
						read(in_fd, buffer, 1);
						//else if ()
						switch(buffer[0])
						{
							case 0x0D:
							case 0x0A:
							{
								char temp[2] = {'\r', '\n'};
								write(1, temp, 2);
								if (encrypt_flag)
									mcrypt_generic(mcpt_fd, buffer, 1);
								if (log_sig)
								{
									char title[13] = "SENT 1 byte: ";
									write(log_fd, title, 13);
									write(log_fd, buffer, 1);
									write(log_fd, "\n", 1);
								}
								write(sock_fd, buffer, 1);
								break;
							}

							default:
							{
								write(1, buffer, 1);
								if (encrypt_flag)
									mcrypt_generic(mcpt_fd, buffer, 1);
								if (log_sig)
								{
									char title[13] = "SENT 1 byte: ";
									write(log_fd, title, 13);
									write(log_fd, buffer, 1);
									write(log_fd, "\n", 1);
								}
								write(sock_fd, buffer, 1);
								break;
							}
						}
					}


					if(poll_fd[1].revents & POLLIN) //if read from socket, display
					{
						char buffer[1];
						int ret = read(sock_fd, buffer, 1);
						if (ret < 0)
						{
							fprintf(stderr, "Cannot read from socket.\n" );
							exit(1);
						}
						if (ret == 0)
							exit(0);
						if (log_sig)
								{
									char title[17] = "RECEIVED 1 byte: ";
									write(log_fd, title, 17);
									write(log_fd, &buffer[0], 1);
									write(log_fd, "\n", 1);
								}
						if (encrypt_flag)
							mdecrypt_generic(mcpt_fd, buffer, 1);
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
								write(1, buffer, 1);
								break;
							}
						}
					}

					if(poll_fd[0].revents & (POLLHUP + POLLERR))	//if keyboard input no longer has anything to read
					{
						perror("keyboard input error.\n");
						break;
					}

					if(poll_fd[1].revents & (POLLHUP + POLLERR)) 	//if shell no long has anything to read
					{
//						pid_t wait_pid;
//						int state;
//						wait_pid = waitpid(child_pid, &state, WNOHANG);
//						if (wait_pid < 0)
//						{
//							perror("waitpid\n");
//							exit(1);
//						}
//						else
//						{
//							fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(state), WEXITSTATUS(state));
//							exit(0);
//						}
						exit(0);
					}
				}
			
	}
		mcrypt_generic_deinit(mcpt_fd);
		mcrypt_module_close(mcpt_fd);
		exit(0);

}

