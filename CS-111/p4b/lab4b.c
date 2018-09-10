

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <mraa.h>
#include <mraa/aio.h>
#include <unistd.h>
#include <math.h>

int main(int argc, char* argv[])
{
	int opt_period = 1;
	int opt_print = 1;
	int opt_log = 0;
	char opt_scale = 'F';
	FILE* log = NULL;

	static struct option long_ops[] = {
		{"period", required_argument, NULL, 'p'},	//optional
		{"scale", required_argument, NULL, 's'},	//optional
		{"log", required_argument, NULL, 'l'},		//optional
		{0, 0, 0, 0}
	};

	int ret = 0;
	while ((ret = getopt_long(argc, argv, "p:s:l", long_ops, NULL)) != -1)
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
				if (opt_scale != 'C' || opt_scale != 'F' || strlen(optarg) != 1)
				{
					fprintf(stderr, "Illegal scale.\n");
					exit(1);
				}
				break;
			case 'l':
				log = fopen(optarg, "w");
				opt_log = 1;
				if (log == NULL)
				{
					fprintf(stderr, "Cannot create log.\n");
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "Illegal argument.\n");
				exit(1);
		}
	}

	struct tm* time_out;
	time_t clock;
	time_t start;
	time_t end;
	char timestr[9];

	struct pollfd pfd[1];
	pfd[0].fd = STDIN_FILENO;
	pfd[0].events = POLLIN | POLLHUP | POOLERR;

	mraa_gpio_context btn;
	btn = mraa_gpio_init(3);
	mraa_gpio_dir(btn, MRAA_GPIO_IN);
	int temperature = 0;
	mrra_aio_context sensor;
	sensor = mraa_aio_init(0);
	if (sensor == NULL)
	{
		fprintf(stderr, "Temperature sensor intialization failed.\n");
		exit(1);
	}

	while(true)
	{
		double t = 0;
		temperature = mraa_aio_read(sensor);
		double RD = 1023.0/((double)temperature) - 1.0;
		RD *= 100000;
		switch(opt_scale)
		{
			case 'C':
				t = 1.0/(log(RD/100000.0) / B + 1/298.15) - 273.15;
				break;
			case 'F':
				t = (1.0/(log(RD/100000.0) / B + 1/298.15) - 273.15) * 1.8 + 32;
				break;
			default:
				fprintf(stderr, "Illegal scale indicator.\n");
				exit(1);
		}

		time(&clock);
		time_out = localtime(&clock);
		strftime(timestr, 9, "%H:%M:%S", time_out);

		fprint(log, "%s %.1f\n", timestr, t);

		time(&start);
		time(&end);
		while (difftime(end, start) < opt_period)
		{
			int button_status = mraa_gpio_read(button);
			if (button_status > 1)
			{
				time(&clock);
				time_out = localtime(&clock);
				strftime(timestr, 9, "%H:%M:%S", time_out);
				fprintf(log, "%s SHUTDOWN\n", timestr);
				exit(0);
			}

			poll(pfd, 1, 0);
			char* command[32];
			if((pfd[0].revents & POLLIN))
			{
				fget(command, 32, 0);
				if (!strcmp(command, "OFF"))
				{
					if(opt_log)
						fprintf(log, "OFF\n");
					time(&clock);
					time_out = localtime(&clock);
					strftime(timestr, 9, "%H:%M:%S", time_out);
					fprintf(log, "%s SHUTDOWN\n", timestr);
					exit(0);
				}
				else if(!strcmp(command, "SCALE=F"))
				{
					if(opt_log)
						fprintf(log, "SCALE=F\n");
					opt_scale = 'F';
				}
				else if(!strcmp(command, "SCALE=C"))
				{
					if(opt_log)
						fprintf(log, "SCALE=C\n");
					opt_scale = 'C';
				}
				else if(!strcmp(command, "STOP"))
				{
					if(opt_log)
						fprintf(log, "STOP\n");
					opt_print = 0;
				}
				else if(!strcmp(command, "START"))
				{
					if(opt_log)
						fprintf(log, "START\n");
					opt_print = 1;
				}
				else if(!strncmp(command, "PERIOD=", 7) && atoi(&command[7]) != 0)		//PERIOD = #
				{
					if(opt_log)
						fprintf(log, "%s\n", command);
					opt_period = atoi(&command[7])
				}
			}

			if (opt_print)
				time(&end);
		}
	}
	exit(0);
}
