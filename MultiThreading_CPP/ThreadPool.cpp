#include "ThreadPool.h"



ThreadPool::ThreadPool(int threadCount)
{
	isCompleted = false;
	isStopped = false;
	nIdle = 0;
	myQueue = std::make_unique<TaskQueue>();

	if (threadCount > 0)
	{
		resize(threadCount);
	}
}

// Returns size of thread pool
int ThreadPool::size()
{
	return Threads.size();
}

int ThreadPool::qSize()
{
	return myQueue->size();
}

// Returns number of idle threads
int ThreadPool::idleThreads()
{
	return nIdle;
}

// Returns a pointer to the specified thread
std::thread& ThreadPool::getThread(int i)
{
	return *Threads[i];
}

// Empties the Task Queue
void ThreadPool::clearQueue()
{
	std::function<void(int id)> *_f = nullptr;

	//Empty the taskqueue
	while (myQueue->pop(_f)) {}; 
}

// Pops a function from the front of the task queue, returning a copy of it
std::function<void(int)> ThreadPool::pop() 
{
	// Handles popping of task queue
	std::function<void(int id)> * _f = nullptr;
	myQueue->pop(_f);

	// Unique ptr ensures function is deleted even if exception occurs
	std::unique_ptr<std::function<void(int id)>> func(_f);

	// Copy of function is made so that it can be returned after original is deleted
	std::function<void(int)> f = *_f;

	return f;
}

// Resize threadpool
void ThreadPool::resize(int nThreads)
{
	// Ensure threadpool is not stopped or completed
	if (!isCompleted && !isStopped)
	{
		int prevThreadCount = size();

		// Increase  thread count
		if (prevThreadCount <= nThreads)
		{
			Threads.resize(nThreads);
			Flags.resize(nThreads);

			// Loop through new threads, instantiating them and their flags
			for (int i = prevThreadCount; i < nThreads; i++)
			{
				Flags[i] = std::make_shared<std::atomic<bool>>(false);
				beginThread(i);
			}
		}
		else
		{
			// Loop through down through surplus threads, detatching and removing them
			for (int i = prevThreadCount - 1; i >= nThreads; i--)
			{
				*Flags[i] = true;
				Threads[i]->detach();
			}

			// Stop detatched threads that were waiting
			std::unique_lock<std::mutex> lock(mtx);
			condVar.notify_all();

			Threads.resize(nThreads);
			Flags.resize(nThreads);
		}
	}
}

void ThreadPool::pushTask(std::function<void(int id)>* aTask)
{
	myQueue->push(aTask);
	std::unique_lock<std::mutex> lock(mtx);

	// Notify one thread to begin work
	condVar.notify_one();
}

void ThreadPool::beginThread(int i)
{
	// Copy the shared pointer to the flag
	std::shared_ptr<std::atomic<bool>> flag(Flags[i]);
	std::cout << "Thread Created" << std::endl;
	
	auto lambda = [this, i, flag]()
	{
		// Copies the shared pointer to flag
		std::atomic<bool> &_flag = *flag;

		// Checks the front of the task queue, returning true and a pointer to the function if it exists
		std::function<void(int id)> *_f = nullptr;
		bool isPop = myQueue->pop(_f);

		while (true)
		{
			// Check to see if anything is currently in the TaskQueue
			while (isPop)
			{
				// ====== Executes the function ======
				(*_f)(i);
				// ====== Executes the function ======

				if (_flag)
				{
					return;
				}
				else
				{
					isPop = this->myQueue->pop(_f);
				}
			}
			// Queue is empty, so we wait for the next command
			std::unique_lock<std::mutex> lock(this->mtx);

			this->nIdle++;

			// When condVar notifies thread, this function is checked
			auto aFunc = [this, &_f, &isPop, &_flag]()
			{
				isPop = this->myQueue->pop(_f);
				return isPop || isCompleted || _flag;
			};

			this->condVar.wait(lock, aFunc);

			this->nIdle--;
			
			// If the queue is empty and 'completed or *flag' then return
			if (!isPop)
			{
				return;
			}
		}
	};
	// Thread[i] is created and set to lambda
	Threads[i].reset(new std::thread(lambda));
}

void ThreadPool::stop(bool isWait)
{
	if (!isWait) // Stops the threadpool without waiting for all functions in the task queue to complete
	{
		if (isStopped)
		{
			// Thread pool already stopped, no work to be done, return
			return;
		}
		else
		{
			isStopped = true;
			for (int i = 0; i < size(); i++)
			{
				// Threads with 'flag = true' are commanded to stop
				*Flags[i] = true;
			}
			// Empty task queue
			clearQueue();
		}
	}
	else // Waits on the task queue to complete before stopping threadpool
	{
		if (isCompleted || isStopped)
		{
			// Thread pool already completed or stopped, no work to be done, return
			return;
		}
		else
		{
			// Commands idle threads to finish once woken
			isCompleted = true;
		}
	}

	{
		std::unique_lock<std::mutex> lock(mtx);
		// Stops all idle threads
		condVar.notify_all();
	}
	

	// Joins all joinable threads
	for (int i = 0; i < static_cast<int>(Threads.size()); i++)
	{
		if (Threads[i]->joinable())
		{
			Threads[i]->join();
		}
	}

	// If there are no threads in pool, but functions in queue, ensure functions are still deleted:
	clearQueue();
	Threads.clear();
	Flags.clear();
}

ThreadPool::~ThreadPool()
{
	// Finish all threads waiting for queue to clear, then joining them
	stop(true);
}
