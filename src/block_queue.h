#include <deque>
#include <mutex>
#include <condition_variable>

template<typename T, std::size_t S = 0>
class BlockingQueue {
public:
    BlockingQueue() = default;
    ~BlockingQueue() = default;

    void push(const T& t) {
        std::unique_lock<std::mutex> l{lock_};

        cv_.wait(l, [&](){ return S == 0 || queue_.size() < S; });
        if (queue_.empty()) {
            cv_.notify_all();
        }
        queue_.push_back(t);
    };
    T pop() {
        std::unique_lock<std::mutex> l{lock_};

        cv_.wait(l, [&](){ return !queue_.empty(); });
        if (S != 0 && queue_.size() == S) {
            cv_.notify_all();
        }
        T t = queue_.front();
        queue_.pop_front();
        
        return t;
    }

    std::size_t size() const {
        std::unique_lock<std::mutex> l{lock_};
        return queue_.size();
    };
private:
    std::deque<T> queue_;
    std::mutex lock_;
    std::condition_variable cv_;
};

// 避免先唤醒，后等待。导致满足条件的死等
// 
