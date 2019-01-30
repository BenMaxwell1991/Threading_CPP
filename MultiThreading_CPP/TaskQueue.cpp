#include "TaskQueue.h"

TaskQueue::TaskQueue()
{

}


void TaskQueue::push(std::function<void(int id)>* const value)
{
	std::unique_lock<std::mutex> lock(mtx);
	q.push(value);
}


bool TaskQueue::pop(std::function<void(int id)>* &aFunc)
{
	std::unique_lock<std::mutex> lock(this->mtx);

	if (q.empty())
	{
		return false;
	}

	aFunc = q.front();
	q.pop();
	return true;
}


bool TaskQueue::isEmpty()
{
	std::unique_lock<std::mutex> lock(mtx);
	return q.empty();
}

int TaskQueue::size()
{
	return q.size();
}

TaskQueue::~TaskQueue()
{

}