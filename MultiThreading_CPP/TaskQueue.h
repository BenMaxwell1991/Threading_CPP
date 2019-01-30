#pragma once
#include <functional>
#include <queue>
#include <mutex>

class TaskQueue
{
public:
	
	// Pushes a task into the Queue, must be a void function that accepts the id of its working thread
	void push(std::function<void(int id)>* const value);

	// Pops the first element of the Queue, (sets aFunc to this elements value)
	// If queue is empty, returns false.
	bool pop(std::function<void(int id)>* &aFunc);

	// Check if queue is empty
	bool isEmpty();

	// Returns the size of the queue
	int size();

	TaskQueue();
	~TaskQueue();

private:
	std::queue<std::function<void(int id)>*> q;
	std::mutex mtx;
};
