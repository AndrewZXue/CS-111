#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

long long num_threads = 1;
long long num_iterators = 1;
int opt_yield = 0;
char opt_sync = 'n';
long long counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int spin_lock = 0;

void add(long long* pointer, long long value){
	long long sum = *pointer + value;
	if(opt_yield)
		sched_yield();
	*pointer = sum;
}

void* thd(){
	long long pre = 0, next = 0;
	int i;
	for(i = 0; i < num_iterators; i++)
	{
		switch(opt_sync)
		{
			case 'm':
				pthread_mutex_lock(&mutex);
				add(&counter, 1);
				pthread_mutex_unlock(&mutex);
				break;
			case 's':
				while(__sync_lock_test_and_set(&spin_lock, 1))
					{}; //spin
				add(&counter, 1);
				__sync_lock_release(&spin_lock);
				break;
			case 'c':
				do{
					pre = counter;
					next = pre + 1;
					if (opt_yield)
						sched_yield();
				}
				while (__sync_val_compare_and_swap(&counter, pre, next) != pre);
				break;
			default:
				add(&counter, 1);
		}
	}
	for(i = 0; i < num_iterators; i++)
	{
		switch(opt_sync)
		{
			case 'm':
				pthread_mutex_lock(&mutex);
				add(&counter, -1);
				pthread_mutex_unlock(&mutex);
				break;
			case 's':
				while(__sync_lock_test_and_set(&spin_lock, -1))
					{}; //spin
				add(&counter, -1);
				__sync_lock_release(&spin_lock);
				break;
			case 'c':
				do{
					pre = counter;
					next = pre - 1;
					if (opt_yield)
						sched_yield();
				}
				while (__sync_val_compare_and_swap(&counter, pre, next) != pre);
				break;
			default:
				add(&counter, -1);
		}
	}
	return NULL;
}

int main(int argc, char* argv[]){

	

	static struct option long_ops[] = {
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", no_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

	int ret = 0;
	while ((ret = getopt_long(argc, argv, "t:i:", long_ops, NULL)) != -1)
	{
		switch(ret)
		{
			case 't':
				num_threads = atoi(optarg);
				break;
			case 'i':
				num_iterators = atoi(optarg);
				break;
			case 'y':
				opt_yield = 1;
				break;
			case 's':
				opt_sync = optarg[0];
				if (strlen(optarg) != 1)
				{
					fprintf(stderr, "Error: only one parameter for -sync.\n");
					exit(1);
				}
				else if(!(optarg[0] == 'm' || optarg[0] == 's' || optarg[0] == 'c'))
				{
					fprintf(stderr, "Error: choose from 'm', 's', or 'c'.\n");
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "Error: illegal parameters.\n");
				exit(1);
		}
	}

	struct timespec start;
	struct timespec end;
	int sret = clock_gettime(CLOCK_MONOTONIC, &start);
	if (sret < 0)
	{
		perror ("Error: Start Time.");
		exit(1);
	}

	pthread_t* threads = malloc (sizeof(pthread_t) * num_threads);

	int i;
	for (i = 0; i != num_threads; i++)
	{
		int pth_ret = pthread_create(&threads[i], NULL, thd, NULL);
		if (pth_ret != 0)
		{
			fprintf(stderr, "Error: Cannot create thread.\n");
			exit(1);
		}
	}

	for (i = 0; i != num_threads; i++)
	{
		int pth_ret = pthread_join(threads[i], NULL);
		if (pth_ret != 0)
		{
			fprintf(stderr, "Error: Cannot join thread.\n");
			exit(1);
		}
	}

	int eret = clock_gettime(CLOCK_MONOTONIC, &end);
	if (eret < 0)
	{
		perror ("Error: End Time.");
		exit(1);
	}

	long long delay = (end.tv_nsec - start.tv_nsec) + (end.tv_sec - start.tv_sec) * 1000000000;
	long long num_operations = num_threads * num_iterators * 2;
	long long ave_delay = delay / num_operations;

	fprintf(stdout, "add");
	if (opt_yield == 1)
		fprintf(stdout, "-yield");
	switch (opt_sync)
	{
		case 'm': 
			fprintf(stdout, "-m");
			break;
		case 's': 
			fprintf(stdout, "-s");
			break;
		case 'c': 
			fprintf(stdout, "-c");
			break;
		default:
			fprintf(stdout, "-none");
	}

	fprintf(stdout, ",%lld", num_threads);
	fprintf(stdout, ",%lld", num_iterators);
	fprintf(stdout, ",%lld", num_operations);
	fprintf(stdout, ",%lld", delay);
	fprintf(stdout, ",%lld", ave_delay);
	fprintf(stdout, ",%lld\n", counter);

	free(threads);

	exit(0);
}