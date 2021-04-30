#include <mutex>
#include <condition_variable>

class Semaphore
{
public:
	Semaphore(int threads);

	void wait();
	void signal();
	
private:
	std::mutex cv_m;
	std::condition_variable cv;
	std::mutex mutex;
	int count;
};
