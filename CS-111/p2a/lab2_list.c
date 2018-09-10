
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
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int spin_lock = 0;

int opt_yield = 0;
char* par_yield = NULL;

SortedList_t* list;
SortedListElement_t* elements; 

char* table = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM";

void* thd(void* argument){
	int num_elements = num_threads * num_iterators;
	int tid = *(int *)argument;
	int i;
	for(i = tid; i < num_elements; i+= num_threads)
	{
		switch(opt_sync)
		{
			case 'm':
				pthread_mutex_lock(&mutex);
				SortedList_insert(list, &elements[i]);
				pthread_mutex_unlock(&mutex);
				break;
			case 's':
				while(__sync_lock_test_and_set(&spin_lock, 1))
					{}; //spin
				SortedList_insert(list, &elements[i]);
				__sync_lock_release(&spin_lock);
				break;
			default:
				SortedList_insert(list, &elements[i]);
		}
	}

	SortedList_length(list);

	SortedListElement_t* t_key;
	//int i;
	for(i = tid; i < num_elements; i += num_threads)
	{
		switch(opt_sync)
		{
			case 'm':
				pthread_mutex_lock(&mutex);

				t_key = SortedList_lookup(list, elements[i].key);
				if (t_key == NULL)
				{
					fprintf(stderr, "Error: Key not in the list\n");
					exit(2);
				}
				SortedList_delete(t_key);

				pthread_mutex_unlock(&mutex);
				break;
			case 's':
				while(__sync_lock_test_and_set(&spin_lock, 1))
					{}; //spin

				t_key = SortedList_lookup(list, elements[i].key);
				if (t_key == NULL)
				{
					fprintf(stderr, "Error: Key not in the list\n");
					exit(2);
				}
				SortedList_delete(t_key);

				__sync_lock_release(&spin_lock);
				break;
			default:
				t_key = SortedList_lookup(list, elements[i].key);
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
			default:
				fprintf(stderr, "Error: illegal parameters.\n");
				exit(1);
		}
	}


	int num_elements = num_threads * num_iterators;
	elements = malloc(num_elements * sizeof(SortedListElement_t));

	list = malloc (sizeof(SortedList_t));
	list->key = NULL;
	list->next = list;
	list->prev = list;

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

	pthread_t* threads = malloc (num_threads * sizeof(pthread_t));
	int* id = malloc(num_threads * sizeof(int));

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

	long long delay = (end.tv_nsec - start.tv_nsec) + (end.tv_sec - start.tv_sec) * 1000000000;
	long long num_operations = num_threads * num_iterators * 3;
	long long ave_delay = delay / num_operations;

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
	fprintf(stdout, ",%d", 1);
	fprintf(stdout, ",%lld", num_operations);
	fprintf(stdout, ",%lld", delay);
	fprintf(stdout, ",%lld\n", ave_delay);
	//fprintf(stdout, ",%lld", counter);

	free(threads);
	free(elements);
	free(id);
	free(list);
	exit(0);

}















