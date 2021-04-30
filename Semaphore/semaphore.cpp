#include "semaphore.h"

Semaphore::Semaphore(int threads)
	: count(threads) {}

void Semaphore::wait()
{
	std::unique_lock<std::mutex> lock(cv_m);
	
	mutex.lock();
	--count;
	mutex.unlock();
	
	if (count < 0)
		cv.wait(lock);
}

void Semaphore::signal()
{
	mutex.lock();
	++count;
	
	if (count <= 0)
		cv.notify_one();
	
	mutex.unlock();
}