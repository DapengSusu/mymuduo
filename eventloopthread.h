#pragma once

#include "noncopyable.h"
#include "thread.h"

#include <condition_variable>
#include <functional>
#include <mutex>

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    explicit EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
        const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();

private:
    // 此方法在单独的新线程里运行
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};
