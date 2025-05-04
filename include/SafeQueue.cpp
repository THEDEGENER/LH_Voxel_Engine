#include <thread>
#include <mutex>
#include <queue>

// templated to allow multi types to enter queue?
template<class T>
class SafeQueue {
public:
    // standard push pop methods for a queue
	void push(const T& val) {
		std::lock_guard<std::mutex> lock(_m);
		_q.push(val);
	}

	bool pop(T& val) {
		std::lock_guard<std::mutex> lock(_m);
		if (!_q.empty()) {
			val = _q.front();
			_q.pop();
			return true;
		} else {
			return false;
		}
	}
private:
    // private memebers 
	std::mutex _m;
	std::queue<T> _q;
};