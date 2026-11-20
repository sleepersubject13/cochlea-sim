#ifndef THREADSAFE_QUEUE_HXX
#define THREADSAFE_QUEUE_HXX

#include <queue>
#include <mutex>
#include <memory>
#include <utility>
#include <condition_variable>
#include <stop_token>

template <typename T>
class threadsafe_queue {

    public:
        threadsafe_queue() = default;
        // Since the following:
        //
        // std::mutex and std::condition_variable_any are strictly non-movable and
        // non-copyable, implicitly, this class becomes non-movable and non-copyable
        // as well.
        //
        // Here, we explicitly delete the copy constructor, copy assignment operator,
        // Move constructor, and move assignment operator
        threadsafe_queue(const threadsafe_queue& rhs) = delete;
        // Copy-assignment operator
        threadsafe_queue& operator=(const threadsafe_queue&) = delete;
        // Move constructor
        threadsafe_queue(threadsafe_queue&&) = delete;
        // Move-assignment operator
        threadsafe_queue& operator=(threadsafe_queue&&) = delete;
        //
        // --> Member functions
        void push(T);
        std::shared_ptr<T> try_pop();
        std::shared_ptr<T> wait_and_pop(std::stop_token stoken);
        bool empty() const;
        


    private:
        std::queue<T> m_data_queue;
        mutable std::mutex m_mutex;
        std::condition_variable_any m_data_condition;
};

template <typename T>
void threadsafe_queue<T>::push(T new_value) {
   
    {
        std::unique_lock<std::mutex> lock{m_mutex};
        m_data_queue.push(std::move(new_value));
    }
    // the lock on m_mutex will immediately be released after the above closing curly brace
    // allowing the consumer threads to be able to immediately acquire locks on it
    // once they're notified by notify_one():
    m_data_condition.notify_one();
}

template <typename T>
std::shared_ptr<T> threadsafe_queue<T>::wait_and_pop(std::stop_token stoken) {
    // We use a std::unique_lock<T> here because we do not want this
    // RAII class template to exclusively own the mutex passed to its
    // constructor. The code that follows will need to be able to lock
    // and unlock the mutex under different circumstances
    std::unique_lock<std::mutex> lock{m_mutex};

    // At this point, having acquired the aforementioned lock, we read from
    // m_data_queue to check whether there's any data to process. If there isn't,
    // wait() releases the acquired lock and puts the thread to sleep until either
    // new data arrives OR stoken receives a stop request.
    bool has_data = m_data_condition.wait(lock, stoken, [this]() -> bool {
        return !m_data_queue.empty();
    });

    // If wait() returned false, it woke up due to stoken.stop_requested() while empty
    if (!has_data) {
        return nullptr;
    }

    // If there is indeed data, wait returns still holding a lock on m_mutex
    std::shared_ptr<T> target_data {std::make_shared<T>(std::move(m_data_queue.front()))};
    m_data_queue.pop();
    return target_data;
}

template <typename T>
std::shared_ptr<T> threadsafe_queue<T>::try_pop() {
    std::unique_lock<std::mutex> lock{m_mutex};

    if (m_data_queue.empty()) {
        return nullptr;
    }

    std::shared_ptr<T> target_data {std::make_shared<T>(std::move(m_data_queue.front()))};
    m_data_queue.pop();
    return target_data;
}

template <typename T>
bool threadsafe_queue<T>::empty() const {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_data_queue.empty();
}

#endif
