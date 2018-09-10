#include <stdio.h>
#include <errno.h>                //for error message
#include <unistd.h>
#include <getopt.h>               //for getopt_long
#include <signal.h>               //for signal
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>               //for strerror

void signal_handlr(int sig) {
  if (sig == SIGSEGV){
    perror("Segmentational Fault.");
    exit(4);
  }
}

int main (int argc, char *argv[]){

  int in_fd = 0;              //initialize input fd
  int out_fd = 1;             //initialize output fd
  int seg_fault_i = 0;        //segmental fault indicator
  struct option op_long[]=
    {
      {"input", required_argument, NULL, 'i'},
      {"output", required_argument, NULL, 'o'},
      {"segfault", no_argument,  NULL, 's'},
      {"catch", no_argument, NULL, 'c'},
      {0, 0, 0, 0}
    };

  ssize_t op;
  //keep taking in argument until there's no more
  while ((op = getopt_long(argc, argv, "", op_long, NULL)) != -1)
    {
      switch(op){
      case 'i':
	in_fd = open(optarg, O_RDONLY);
        if(in_fd >= 0)
        {
                close(0);
                dup(in_fd);
                close(in_fd);               //set input file's fd to stdin
        }
        else
        {       int errmessage = errno;
                fprintf(stderr, "%s%s\n", "Unable to open the specified input file.", strerror(errmessage));
                exit(2);
        }
	break;
      case 'o':
	out_fd = creat(optarg, S_IRWXU);
        if(out_fd >= 0)
        {
                close(1);
                dup(out_fd);
                close(out_fd);              //set output file's fd to stdout
        }
        else
        {
                int errmessage = errno;
                fprintf(stderr, "%s%s\n", "unable to create the specified output file.", strerror(errmessage));
                exit(3);
        }
	break;
      case 's':
	seg_fault_i = 1;                           //set seg_fault indicator to 1 to trigger seg fault
	break;
      case 'c':
	signal(SIGSEGV, signal_handlr);            //use the signal function from library
	break;
      default:
	printf("ivalid argument\n manual: lab0 -i inputfile -o outputfile\n");
	exit(1);
      }
    }
 
 if(seg_fault_i == 1){
        //calling a subroutine that sets a char * pointer to NULL and then stores through the null pointer
        char* trigger = NULL;               
 	*trigger = 'n';
}

ssize_t rd_ret;
char buffer[65];
//actual read and write
while ((rd_ret = read(STDIN_FILENO, &buffer, 64))>0)
	write(STDOUT_FILENO, &buffer, rd_ret);
exit(0);
}



