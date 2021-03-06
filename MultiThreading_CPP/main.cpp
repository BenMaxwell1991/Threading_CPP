#include "ThreadPool.h"
#include "TaskQueue.h"

std::atomic<int> Global_Count = 0;
std::mutex global_mtx;

// Increments global count by 1,000,000, prints to std::cout every 100,000
std::function<void(int id)> inc = [](int id = 0) 
{ 
	for (int i = 0; i < 1000000; i++)
	{
		Global_Count++;

		// Ensure count is not incremented by another thread before being printed to screen by this one
		// Simply cleans up output, no functional difference
		int Count_Copy = Global_Count;
		if (Count_Copy % 100000 == 0)
		{
			std::unique_lock<std::mutex> lock(global_mtx);
			std::cout << "Global Count: " << Count_Copy << " Thread: " << id << std::endl;
		}
	}
};

int main()
{
	ThreadPool myPool(8);

	// Adds 100 seperate tasks to the task pool, each incrementing Global_Count by 1,000,000
	for (int i = 0; i < 100; i++)
	{
		myPool.pushTask(&inc);	
	}

	// Stops the threadpool, waiting for queued tasks to finished
	myPool.stop(true);	

	return 0;
}