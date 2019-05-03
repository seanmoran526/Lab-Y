#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

// Semaphore for agent, pushers and smokers//
sem_t agentSem;
sem_t smokerSem[3];
sem_t pusherSem[3];

// Pusher Block//
sem_t pusherBlock;

//Prototypes//
void* smoker(void* arg);
void* pusher(void* arg);
void* agent(void* arg);

int main()
{
	// Seed our random number since we will be using random numbers
	srand(time(NULL));

	// There is only one agent semaphore since only one set of items may be on
	// the table at any given time. A values of 1 = nothing on the table
	sem_init(&agent_ready, 0, 1);

	// Initalize the pusher lock semaphore
	sem_init(&pusher_lock, 0, 1);

	// Initialize the semaphores for the smokers and pusher
	for (int i = 0; i < 3; ++i)
	{
		sem_init(&smoker_semaphors[i], 0, 0);
		sem_init(&pusher_semaphores[i], 0, 0);
	}

	// Smoker ID's will be passed to the threads. Allocate the ID's on the stack
	int smoker_ids[6];

	pthread_t smoker_threads[6];

	// Create the 6 smoker threads with IDs
	for (int i = 0; i < 6; ++i)
	{
		smoker_ids[i] = i;

		if (pthread_create(&smoker_threads[i], NULL, smoker, &smoker_ids[i]) == EAGAIN)
		{
			perror("Insufficient resources to create thread");
			return 0;
		}
	}

	// Pusher ID's will be passed to the threads. Allocate the ID's on the stack
	int pusher_ids[6];

	pthread_t pusher_threads[6];

	for (int i = 0; i < 3; ++i)
	{
		pusher_ids[i] = i;

		if (pthread_create(&pusher_threads[i], NULL, pusher, &pusher_ids[i]) == EAGAIN)
		{
			perror("Insufficient resources to create thread");
			return 0;
		}
	}

	// Agent ID's will be passed to the threads. Allocate the ID's on the stack
	int agent_ids[6];

	pthread_t agent_threads[6];

	for (int i = 0; i < 3; ++i)
	{
		agent_ids[i] =i;

		if (pthread_create(&agent_threads[i], NULL, agent, &agent_ids[i]) == EAGAIN)
		{
			perror("Insufficient resources to create thread");
			return 0;
		}
	}

	// Make sure all the smokers are done smoking
	for (int i = 0; i < 6; ++i)
	{
		pthread_join(smoker_threads[i], NULL);
	}

	return 0;
}


void* smoker(void* arg)
{
	int smoker_id = *(int*) arg;
	int type_id   = smoker_id % 3;

	// Smoke 3 times
	for (int i = 0; i < 3; ++i)
	{
		printf("\033[0;37mSmoker %d \033[0;31m>>\033[0m Waiting for %s\n",
			smoker_id, smoker_types[type_id]);

		// Wait for the proper combination of items to be on the table
		sem_wait(&smoker_semaphors[type_id]);

		// Make the cigarette before releasing the agent
		printf("\033[0;37mSmoker %d \033[0;32m<<\033[0m Now making the a cigarette\n", smoker_id);
		usleep(rand() % 50000);
		sem_post(&agent_ready);

		// We're smoking now
		printf("\033[0;37mSmoker %d \033[0;37m--\033[0m Now smoking\n", smoker_id);
		usleep(rand() % 50000);
	}

	return NULL;
}

void* pusher(void* arg)
{
	int pusher_id = *(int*) arg;

	for (int i = 0; i < 12; ++i)
	{
		// Wait for this pusher to be needed
		sem_wait(&pusher_semaphores[pusher_id]);
		sem_wait(&pusher_lock);

		// Check if the other item we need is on the table
		if (items_on_table[(pusher_id + 1) % 3])
		{
			items_on_table[(pusher_id + 1) % 3] = false;
			sem_post(&smoker_semaphors[(pusher_id + 2) % 3]);
		}
		else if (items_on_table[(pusher_id + 2) % 3])
		{
			items_on_table[(pusher_id + 2) % 3] = false;
			sem_post(&smoker_semaphors[(pusher_id + 1) % 3]);
		}
		else
		{
			// The other item's aren't on the table yet
			items_on_table[pusher_id] = true;
		}

		sem_post(&pusher_lock);
	}

	return NULL;
}

void* agent(void* arg)
{
	int agent_id = *(int*) arg;

	for (int i = 0; i < 6; ++i)
	{
		usleep(rand() % 200000);

		// Wait for a lock on the agent
		sem_wait(&agent_ready);

		// Release the items this agent gives out
		sem_post(&pusher_semaphores[agent_id]);
		sem_post(&pusher_semaphores[(agent_id + 1) % 3]);

		// Say what type of items we just put on the table
		printf("\033[0;35m==> \033[0;33mAgent %d giving out %s\033[0;0m\n",
			agent_id, smoker_types[(agent_id + 2) % 3]);
	}

	return NULL;
}
