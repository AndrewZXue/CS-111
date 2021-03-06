

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <errno.h>
// #include <mraa.h>
// #include <mraa/aio.h>
#include <unistd.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>

int sock_fd = 0;
struct sockaddr_in serv_addr;
struct hostent *server;
int port = 18000;
int ID = 111222333;
char* hostname;

int opt_period = 1;
int opt_print = 1;
int opt_log = 0;
char opt_scale = 'F';
FILE* flog = NULL;

void setup(){
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd<0)
	{
		fprintf(stderr, "Error: Cannot open socket.\n");
		exit(1);
	}
	server = gethostbyname(hostname);
	if (server == NULL)
	{
		fprintf(stderr, "Error: Host not found.\n");
		exit(2);
	}

	memset((void *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	memcpy((char *) &serv_addr.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
	if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		fprintf(stderr, "Error: Connection failed\n");
		exit(2);
	}
	dprintf(sock_fd, "ID=%d\n", ID);
    if(opt_log) 
    	fprintf(flog, "ID=%d\n", ID);
}


int main(int argc, char* argv[])
{

	static struct option long_ops[] = {
		{"period", required_argument, NULL, 'p'},	//optional
		{"scale", required_argument, NULL, 's'},	//optional
		{"log", required_argument, NULL, 'l'},		//optional
		{"id", required_argument, NULL, 'i'},
		{"host", required_argument, NULL, 'h'},
		{0, 0, 0, 0}
	};

	int ret = 0;
	while ((ret = getopt_long(argc, argv, "", long_ops, NULL)) != -1)
	{
		switch(ret)
		{
			case 'p':
				opt_period = atoi(optarg);
				if (opt_period < 0)
				{
					fprintf(stderr, "Illegal period.\n");
					exit(1);
				}
				break;
			case 's':
				opt_scale = optarg[0];
				if ((opt_scale != 'C' && opt_scale != 'F') || strlen(optarg) != 1)
				{
					fprintf(stderr, "Illegal scale.\n");
					exit(1);
				}
				break;
			case 'l':
				flog = fopen(optarg, "w");
				opt_log = 1;
				if (flog == NULL)
				{
					fprintf(stderr, "Cannot create log.\n");
					exit(1);
				}
				break;
			case 'i':
				ID = atoi(optarg);
				break;
			case 'h':
				hostname = optarg;
				break;
			default:
				fprintf(stderr, "Illegal argument.\n");
				exit(1);
		}
		port = atoi(argv[argc - 1]);
	}

	setup();

	struct tm* time_out;
	time_t clock;
	time_t start;
	time_t end;
	char timestr[9];

	struct pollfd pfd[1];
	pfd[0].fd = sock_fd;
	pfd[0].events = POLLIN | POLLHUP | POLLERR;

	//mraa_gpio_context btn;
	//btn = mraa_gpio_init(62);
	//mraa_gpio_dir(btn, MRAA_GPIO_IN);
	int temperature = 400;
	// mraa_aio_context sensor;
	// sensor = mraa_aio_init(1);
	// if (sensor == NULL)
	// {
	// 	fprintf(stderr, "Temperature sensor intialization failed.\n");
	// 	exit(1);
	// }

	const int B = 4275;
	while(1)
	{
		double t = 0;
		//temperature = mraa_aio_read(sensor);
		double RD = 1023.0/((double)temperature) - 1.0;
		RD *= 100000;
		switch(opt_scale)
		{
			case 'C':
				t = 1.0/ (log(RD/100000.0) / B + 1/298.15) - 273.15;
				break;
			case 'F':
				t = (1.0/ (log(RD/100000.0) / B + 1/298.15) - 273.15) * 1.8 + 32;
				break;
			default:
				fprintf(stderr, "Illegal scale indicator.\n");
				exit(1);
		}

		time(&clock);
		time_out = localtime(&clock);
		strftime(timestr, 9, "%H:%M:%S", time_out);

		dprintf(sock_fd, "%s %.1f\n", timestr, t);
		if(opt_log)
			fprintf(flog, "%s %.1f\n", timestr, t);
		else
			fprintf(stdout, "%s %.1f\n", timestr, t);

		time(&start);
		time(&end);
		char command[32];
		while (difftime(end, start) < opt_period)
		{
			// int button_status = mraa_gpio_read(btn);
			// if (button_status >= 1)
			// {
			// 	time(&clock);
			// 	time_out = localtime(&clock);
			// 	strftime(timestr, 9, "%H:%M:%S", time_out);
			// 	if(opt_log)
			// 		fprintf(flog, "%s SHUTDOWN\n", timestr);
			// 	exit(0);
			// }

			poll(pfd, 1, 0);
			
			if((pfd[0].revents & POLLIN))
			{
				int h = 0;
				char key;
				bzero(command, 32);
				while(1)
				{
					if(read(sock_fd, &key, 1) > 0)
					{
						if(key == '\n')
						{
							command[h] = '\0';
							break;
						}
						command[h] = key;
						h++;
					}
				}

				if (!strcmp(command, "OFF"))
				{
					if(opt_log)
						fprintf(flog, "OFF\n");
					time(&clock);
					time_out = localtime(&clock);
					strftime(timestr, 9, "%H:%M:%S", time_out);
					if(opt_log)
						fprintf(flog, "%s SHUTDOWN\n", timestr);
					dprintf(sock_fd, "%s SHUTDOWN\n", timestr);
					if(close(sock_fd) < 0)
					{
						fprintf(stderr, "%s\n", "Error: Cannot close socket.");
						exit(2);
					}
					exit(0);
				}
				else if(!strcmp(command, "SCALE=F"))
				{
					if(opt_log)
						fprintf(flog, "SCALE=F\n");
					opt_scale = 'F';
				}
				else if(!strcmp(command, "SCALE=C"))
				{
					if(opt_log)
						fprintf(flog, "SCALE=C\n");
					opt_scale = 'C';
				}
				else if(!strcmp(command, "STOP"))
				{
					if(opt_log)
						fprintf(flog, "STOP\n");
					opt_print = 0;
				}
				else if(!strcmp(command, "START"))
				{
					if(opt_log)
						fprintf(flog, "START\n");
					opt_print = 1;
				}
				else if(!strncmp(command, "PERIOD=", 7) && atoi(&command[7]) != 0)		//PERIOD = #
				{
					if(opt_log)
						fprintf(flog, "%s\n", command);
					opt_period = atoi(&command[7]);
				}
			}

			if (opt_print)
				time(&end);
		}
	}
	exit(0);
}
