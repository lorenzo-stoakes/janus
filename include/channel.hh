#pragma once

#include <condition_variable>
#include <mutex>

namespace janus
{
template<typename T>
class channel
{
public:
	channel() : _val{} {}

	void send(T val)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_val = val;
		lock.unlock();
		_cond_var.notify_one();
	}

	void send_all(T val)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_val = val;
		lock.unlock();
		_cond_var.notify_all();
	}

	auto receive() -> T
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_cond_var.wait(lock);
		return _val;
	}

private:
	std::mutex _mutex;
	std::condition_variable _cond_var;
	T _val;
};
} // namespace janus
