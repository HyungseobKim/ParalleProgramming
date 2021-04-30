#include "gol.h"

#include <pthread.h>
#include <semaphore.h>
#include <algorithm>

sem_t read_mutex;
sem_t write_mutex;
pthread_mutex_t count_mutex;

int threads;
int count = 0;

std::vector<std::tuple<int,int>>* population;
int iteration;
int width;
int height;

const int neighbor_x[8] = { 1, 1, 1, 0, 0, -1, -1, -1 };
const int neighbor_y[8] = { 1, 0, -1, 1, -1, 1, 0, -1 };

void* NextGeneration(void* pos)
{
	const int x = (*static_cast<int*>(pos))%width;
	const int y = (*static_cast<int*>(pos))/width;
	
	for (int i = 0; i < iteration; ++i)
	{
		int neighbors = 0;
		
		for (int j = 0; j < 8; ++j)
		{
			const int currX = x + neighbor_x[j];
			if (currX < 0 || currX >= width)
				continue;
			
			const int currY = y + neighbor_y[j];
			if (currY < 0 || currY >= height)
				continue;
			
			auto itr = std::find(population->begin(), population->end(), std::make_tuple(currX, currY));
			if (itr != population->end())
				++neighbors;
		}
		
		// Last thread makes a signal
		pthread_mutex_lock(&count_mutex);
		++count;
		if (count == threads)
		{
			sem_wait(&read_mutex);
			sem_post(&write_mutex);
		}
		pthread_mutex_unlock(&count_mutex);
		
		auto cell = std::make_tuple(x, y);
		
		// Barrier - waiting until all threads to finish their calculation
		sem_wait(&write_mutex);
		
		auto itr = std::find(population->begin(), population->end(), cell);
		if (itr == population->end())
		{
			if (neighbors == 3)
				population->push_back(cell);
		}
		else
		{
			if (neighbors < 2 || neighbors > 3)
				population->erase(itr);
		}
		
		sem_post(&write_mutex);
		
		// Last thread makes a signal
		pthread_mutex_lock(&count_mutex);
		--count;
		if (count == 0)
		{
			sem_wait(&write_mutex);
			sem_post(&read_mutex);
		}
		pthread_mutex_unlock(&count_mutex);
		
		// Barrier - waiting until all threads to finish their writing
		sem_wait(&read_mutex);
		sem_post(&read_mutex);
	}
	
	delete static_cast<int*>(pos);
	return 0;
}

std::vector<std::tuple<int,int>> run(std::vector<std::tuple<int,int>> initial_population, int num_iter, int max_x, int max_y)
{
	threads = max_x * max_y;
	pthread_t* threadIDs = new pthread_t[threads];
	
	// Initialize mutex and semmaphores
	sem_init(&read_mutex, 0, 1);
	sem_init(&write_mutex, 0, 0);
	pthread_mutex_init(&count_mutex, NULL);
	
	// Set sharing variables
	population = &initial_population;
	iteration = num_iter;
	width = max_x;
	height = max_y;
	
	// Create and join threads
	for (int i = 0; i < threads; ++i)
		pthread_create(&threadIDs[i], 0, NextGeneration, new int(i));
	
	for (int i = 0; i < threads; ++i)
		pthread_join(threadIDs[i], 0);
	
	// Destroy mutext and semaphores
	sem_destroy(&read_mutex);
	sem_destroy(&write_mutex);
	pthread_mutex_destroy(&count_mutex);
	
	delete [] threadIDs;
	return initial_population;
	
}