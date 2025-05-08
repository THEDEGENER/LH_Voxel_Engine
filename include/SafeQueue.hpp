#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

// templated to allow multi types to enter queue?
template<class T>
class SafeQueue {
public:
    // standard push pop methods for a queue
	void push(const T& val) 
    {
        {
            std::lock_guard<std::mutex> lock(_m);
            _q.push(val);
        }
        _cv.notify_one();
	}

    void push(const T&& val) 
    {
        {
            std::lock_guard<std::mutex> lock(_m);
            _q.push(val);
        }
        _cv.notify_one();
	}

	T pop() 
    {
        std::unique_lock<std::mutex> lock(_m);
        _cv.wait(lock, [&]{ return !_q.empty() || _closed == true; });  // blocks here until someone .push()
        if (_q.empty() && _closed) {
            return T();
        }
        T item = std::move(_q.front());
        _q.pop();
        return item;
    }

    bool hasItems() const 
    {
        std::lock_guard<std::mutex> lock(_m);
        return !_q.empty();
    }

    bool tryPop(T& out)
    {
        // may need to distingush closed as empty but still operational
        // vs empty and shutdown
        std::lock_guard<std::mutex> lock(_m);
        if (_q.empty()) return false;
        out = std::move(_q.front());
        _q.pop();
        return true;
    }

    void close() {
        {
          std::lock_guard<std::mutex> lock(_m);
          _closed = true;
        }
        _cv.notify_all();
      }


private:
    // private memebers 
	std::mutex _m;
	std::queue<T> _q;
    std::condition_variable _cv;
    bool _closed = false;
};
