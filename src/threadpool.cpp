/*
threadpool.cpp
 __    __                               __                          ___
/\ \__/\ \                             /\ \                        /\_ \
\ \ ,_\ \ \___   _ __    __     __     \_\ \  _____     ___     ___\//\ \
 \ \ \/\ \  _ `\/\`'__\/'__`\ /'__`\   /'_` \/\ '__`\  / __`\  / __`\\ \ \
  \ \ \_\ \ \ \ \ \ \//\  __//\ \L\.\_/\ \L\ \ \ \L\ \/\ \L\ \/\ \L\ \\_\ \_
   \ \__\\ \_\ \_\ \_\\ \____\ \__/.\_\ \___,_\ \ ,__/\ \____/\ \____//\____\
    \/__/ \/_/\/_/\/_/ \/____/\/__/\/_/\/__,_ /\ \ \/  \/___/  \/___/ \/____/
                                                \ \_\
                                                 \/_/
*/


#include <thread>
#include <mutex>
#include <iostream>
#include <vector>
#include <deque>
#include <sstream>
#include <functional>
#include <iostream>
#include <condition_variable>

#include "threadpool.h"


void Worker::operator()()
{

	std::function<void()> task;

	while (true) {

		std::unique_lock<std::mutex> locker(pool.queue_mutex);
		pool.cv.wait(locker, [&](){return pool.stop || !pool.tasks.empty(); });
		if (pool.stop) return;
		if(!pool.tasks.empty()) {
			task = pool.tasks.front();
			pool.tasks.pop_front();
			locker.unlock();

			task();
		}
		else {
			locker.unlock();
		}

	}
}


ThreadPool::ThreadPool(size_t threads): stop(false)
{
	for (size_t i = 0; i < threads; ++i)
		workers.push_back(std::thread(Worker(*this)));
}


ThreadPool::~ThreadPool()
{
    stop = true; // stop all threads
    cv.notify_all();

    for (auto &thread: workers)
    	thread.join();
}


void ThreadPool::enqueue(std::function<void()> f)
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		tasks.push_back(f);
	}
	cv.notify_one();
}
