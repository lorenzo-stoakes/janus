#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace janus
{
template<typename T>
class channel
{
public:
	channel() : _val{}, _acked{false} {}

	void send(T val)
	{
		do {
			std::unique_lock<std::mutex> lock(_mutex);
			_val = val;
			lock.unlock();
			_cond_var.notify_one();
		} while (!_acked.load());
		_acked.store(false);
	}

	auto receive() -> T
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_cond_var.wait(lock);
		_acked.store(true);
		return _val;
	}

private:
	std::mutex _mutex;
	std::condition_variable _cond_var;
	T _val;
	std::atomic<bool> _acked;
};
} // namespace janus
