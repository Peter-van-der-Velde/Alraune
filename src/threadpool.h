#ifndef __threadpool_h__
#define __threadpool_h__

#include <functional>
#include <thread>
#include <mutex>
#include <iostream>
#include <vector>
#include <deque>
#include <sstream>
#include <condition_variable>

class ThreadPool; // forward declare

class Worker
{
    public:
	Worker(ThreadPool& s)
		: pool(s)
	{
	}
	void operator()();

    private:
	ThreadPool& pool;
};

class ThreadPool
{
    public:
	ThreadPool(size_t threads);
	void enqueue(std::function<void()> f);
	~ThreadPool();

    private:
	friend class Worker;

	std::vector<std::thread> workers;
	std::deque<std::function<void()>> tasks;

	std::condition_variable cv;

	std::mutex queue_mutex;
	bool stop;
};

#endif