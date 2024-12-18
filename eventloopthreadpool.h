#pragma once

#include "noncopyable.h"

#include <functional>
#include <string>
#include <memory>
#include <vector>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string&& name);
    ~EventLoopThreadPool() {}

    bool started() const { return started_; }
    const std::string& name() const { return name_; }

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // 如果工作在多线程中，baseLoop默认以轮询的方式分配channel给subLoop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

private:
    EventLoop* baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};
