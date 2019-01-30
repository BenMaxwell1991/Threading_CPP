#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <functional>
#include <atomic>
#include "TaskQueue.h"

class ThreadPool
{
public:

	ThreadPool(int threadCount = 0);

	// Getter funcs
	int size();
	int qSize();
	int idleThreads();
	std::thread& getThread(int i);

	// Empties this thread pools Task Queue
	void clearQueue();

	// Push task into thread pool
	void pushTask(std::function<void(int id)>* aTask);

	// Pop function from task queue, returning a functional wrapper to it
	std::function<void(int)> pop();

	// Resizes number of threads within pool
	void resize(int nThreads);

	// Stops all threads once their respective tasks are completed
	void stop(bool isWait = false);	

	~ThreadPool();

private:

	// Begins thread 'i', will wait on condVar until tasks are added to 'myQueue'
	void beginThread(int i);

	std::vector<std::unique_ptr<std::thread>> Threads;
	std::vector<std::shared_ptr<std::atomic<bool>>> Flags;

	std::unique_ptr<TaskQueue> myQueue;

	std::atomic<bool>isStopped;
	std::atomic<bool>isCompleted;
	std::atomic<int> nIdle;
	
	std::mutex mtx;
	std::condition_variable condVar;
};

