
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "SortedList.h"

int num_threads = 1;
int num_iterators = 1;
char opt_sync = 'n';
//long long counter = 0;
pthread_mutex_t* mutex;
int* spin_lock;
int* hashtb;
long long* delays;
int length = 0;

int opt_yield = 0;
char* par_yield = NULL;
int opt_list = 1;

SortedList_t* sublist_arr;
SortedListElement_t* elements; 

char* table = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM";

void* thd(void* argument){
	int num_elements = num_threads * num_iterators;
	int tid = *(int *)argument;
	struct timespec start_thd;
	struct timespec end_thd;

	int i;
	for(i = tid; i < num_elements; i+= num_threads)
	{
		switch(opt_sync)
		{
			case 'm':
				clock_gettime(CLOCK_MONOTONIC, &start_thd);
				pthread_mutex_lock(&mutex[hashtb[i]]);
				clock_gettime(CLOCK_MONOTONIC, &end_thd);
				SortedList_insert(&sublist_arr[ hashtb[i] ], &elements[i]);
				delays[tid] += ((end_thd.tv_sec - start_thd.tv_sec) * 1000000000) + (end_thd.tv_nsec - start_thd.tv_nsec);
				pthread_mutex_unlock(&mutex[hashtb[i]]);

				break;
			case 's':
				clock_gettime(CLOCK_MONOTONIC, &start_thd);
				while(__sync_lock_test_and_set(&spin_lock[hashtb[i]], 1))
					{}; //spin
				clock_gettime(CLOCK_MONOTONIC, &end_thd);
				SortedList_insert(&sublist_arr[ hashtb[i] ], &elements[i]);
				delays[tid] += ((end_thd.tv_sec - start_thd.tv_sec) * 1000000000) + (end_thd.tv_nsec - start_thd.tv_nsec);				
				__sync_lock_release(&spin_lock[ hashtb[i] ]);

				break;
			default:
				SortedList_insert(&sublist_arr[hashtb[i]], &elements[i]);
		}
	}

	
	for(i = 0; i < opt_list; i++)
	{
		switch(opt_sync)
		{
			case 'm':
				clock_gettime(CLOCK_MONOTONIC, &start_thd);
				pthread_mutex_lock(&mutex[i]);
				clock_gettime(CLOCK_MONOTONIC, &end_thd);
				length = length + SortedList_length(&sublist_arr[i]);
				delays[tid] += ((end_thd.tv_sec - start_thd.tv_sec) * 1000000000) + (end_thd.tv_nsec - start_thd.tv_nsec);

				pthread_mutex_unlock(&mutex[i]);
				break;
			case 's':
				clock_gettime(CLOCK_MONOTONIC, &start_thd);
				while(__sync_lock_test_and_set(&spin_lock[i], 1))
					{}; //spin
				clock_gettime(CLOCK_MONOTONIC, &end_thd);
				length = length + SortedList_length(&sublist_arr[i]);
				delays[tid] += ((end_thd.tv_sec - start_thd.tv_sec) * 1000000000) + (end_thd.tv_nsec - start_thd.tv_nsec);
				
				__sync_lock_release(&spin_lock[i]);

				break;
			default:
				length = length + SortedList_length(&sublist_arr[i]);
		}
	}
	

	SortedListElement_t* t_key;
	//int i;
	for(i = tid; i < num_elements; i += num_threads)
	{
		switch(opt_sync)
		{
			case 'm':
				clock_gettime(CLOCK_MONOTONIC, &start_thd);
				pthread_mutex_lock(&mutex[hashtb[i]]);
				clock_gettime(CLOCK_MONOTONIC, &end_thd);

				t_key = SortedList_lookup(&sublist_arr[hashtb[i]], elements[i].key);
				if (t_key == NULL)
				{
					fprintf(stderr, "Error: Key not in the list\n");
					exit(2);
				}
				SortedList_delete(t_key);
				delays[tid] += ((end_thd.tv_sec - start_thd.tv_sec) * 1000000000) + (end_thd.tv_nsec - start_thd.tv_nsec);

				pthread_mutex_unlock(&mutex[hashtb[i]]);

				break;
			case 's':
				clock_gettime(CLOCK_MONOTONIC, &start_thd);
				while(__sync_lock_test_and_set(&spin_lock[hashtb[i]], 1))
					{}; //spin
				clock_gettime(CLOCK_MONOTONIC, &end_thd);

				t_key = SortedList_lookup(&sublist_arr[hashtb[i]], elements[i].key);
				if (t_key == NULL)
				{
					fprintf(stderr, "Error: Key not in the list\n");
					exit(2);
				}
				SortedList_delete(t_key);
				delays[tid] += ((end_thd.tv_sec - start_thd.tv_sec) * 1000000000) + (end_thd.tv_nsec - start_thd.tv_nsec);

				__sync_lock_release(&spin_lock[hashtb[i]]);

				break;
			default:
				t_key = SortedList_lookup(&sublist_arr[hashtb[i]], elements[i].key);
				if (t_key == NULL)
				{
					fprintf(stderr, "Error: Key not in the list\n");
					exit(2);
				}
				SortedList_delete(t_key);
		}
	}
	return NULL;
}

int main(int argc, char* argv[]){
	
	static struct option long_ops[] = {
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", required_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
		{"lists", required_argument, NULL, 'l'},
		{0, 0, 0, 0}
	};

	int ret = 0;
	while ((ret = getopt_long(argc, argv, "t:i:y:s:", long_ops, NULL)) != -1)
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
				if(strlen(optarg) < 1 || strlen(optarg) > 3)
				{
					fprintf(stderr, "Error: invalid yield option size.\n");
					exit(1);
				}
				par_yield = (char*)optarg;
				int i;
				int optsize = strlen(optarg);
				for(i=0; i!=optsize; i++)
				{
					switch(optarg[i])
					{
						case 'i':
							opt_yield |= INSERT_YIELD;
							break;
						case 'd':
							opt_yield |= DELETE_YIELD;
							break;
						case 'l':
							opt_yield |= LOOKUP_YIELD;
							break;
						default:
							fprintf(stderr, "Error: invalid yield options.\n");
							exit(1);
					}
				}
				break;
			case 's':
				opt_sync = optarg[0];
				if (strlen(optarg) != 1)
				{
					fprintf(stderr, "Error: only one parameter for -sync.\n");
					exit(1);
				}
				else if(!(optarg[0] == 'm' || optarg[0] == 's'))
				{
					fprintf(stderr, "Error: choose from 'm', 's'.\n");
					exit(1);
				}
				break;
			case 'l':
				opt_list = atoi(optarg);
				if (opt_list < 0)
				{
					fprintf(stderr, "Error: number of list must be greater than 0.\n");
					exit(2);
				}
				break;
			default:
				fprintf(stderr, "Error: illegal parameters.\n");
				exit(1);
		}
	}


	int num_elements = num_threads * num_iterators;
	elements = malloc(num_elements * sizeof(SortedListElement_t));

	sublist_arr = malloc(opt_list * sizeof(SortedList_t));
	int s;
	for (s = 0; s < opt_list; s++)
	{
		sublist_arr[s].next = &sublist_arr[s];
		sublist_arr[s].prev = &sublist_arr[s];
		sublist_arr[s].key = NULL;
	}

	mutex = malloc(opt_list * sizeof(pthread_mutex_t));
	int m;
	for(m = 0; m < opt_list; m++){
		pthread_mutex_init(&mutex[m], NULL);	
	}
	

	//list = malloc (sizeof(SortedList_t));
	//list->key = NULL;
	//list->next = list;
	//list->prev = list;

	spin_lock = malloc(opt_list * sizeof(int));
	for (m = 0; m < opt_list; m++)
	{
		spin_lock[m] = 0;
	}

	srand(time(NULL));
	int i;
	for (i=0; i!=num_elements; i++)
	{
		int keysize = rand() % 10 + 10;
		char* key = malloc((keysize+1)*sizeof(char));
		if(key == NULL)
		{
			fprintf(stderr, "Error: Cannont allocate random key.\n");
			exit(1);
		}
		int k;
		for (k=0; k!=keysize; k++)
		{
			int n = rand() % sizeof(table);
			key[k] = table[n];
		}
		key[keysize] = '\0';
		elements[i].key = key;

		free(key);
	}
	hashtb = malloc (num_elements * sizeof(int));
	for (i = 0; i!=num_elements; i++)
	{
		hashtb[i] = i % opt_list;
	}

	pthread_t* threads = malloc (num_threads * sizeof(pthread_t));
	int* id = malloc(num_threads * sizeof(int));

	delays = malloc(num_threads * sizeof(long long));
	struct timespec start;
	struct timespec end;

	int sret = clock_gettime(CLOCK_MONOTONIC, &start);
	if (sret < 0)
	{
		perror ("Error: Start Time.");
		exit(1);
	}

	int j;
	for (j = 0; j != num_threads; j++)
	{
		id[j] = j;
		int pth_ret = pthread_create(&threads[j], NULL, thd, &id[j]);
		if (pth_ret < 0)
		{
			fprintf(stderr, "Error: Cannot create thread.\n");
			exit(1);
		}
	}

	for (j = 0; j != num_threads; j++)
	{
		int pth_ret = pthread_join(threads[j], NULL);
		if (pth_ret < 0)
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

	if(length < 0)
	{
		perror ("Incorrect List Length.\n");
	}

	long long delay = (end.tv_nsec - start.tv_nsec) + (end.tv_sec - start.tv_sec) * 1000000000;
	long long lock_delay = 0;
	for (j = 0; j < num_threads; j++)
		lock_delay += delays[j];
	long long num_operations = num_threads * num_iterators * 3;
	long long ave_delay = delay / num_operations;
	long long ave_lock_delay = lock_delay / num_operations;

	fprintf(stdout, "list");
	if(!opt_yield)
		fprintf(stdout, "-none");
	else
		fprintf(stdout, "-%s", par_yield);
	switch (opt_sync)
	{
		case 'm': 
			fprintf(stdout, "-m");
			break;
		case 's': 
			fprintf(stdout, "-s");
			break;
		default:
			fprintf(stdout, "-none");
	}

	fprintf(stdout, ",%d", num_threads);
	fprintf(stdout, ",%d", num_iterators);
	fprintf(stdout, ",%d", opt_list);
	fprintf(stdout, ",%lld", num_operations);
	fprintf(stdout, ",%lld", delay);
	fprintf(stdout, ",%lld", ave_delay);
	if (opt_sync == 'm' || opt_sync == 's')
		fprintf(stdout, ",%lld\n", ave_lock_delay);
	else fprintf(stdout, "\n");
	//fprintf(stdout, ",%lld", counter);

	free(threads);
	free(elements);
	free(id);
	free(sublist_arr);
	free(mutex);
	free(spin_lock);
	free(hashtb);
	free(delays);
	exit(0);

}















