#include "eventloopthread.h"
#include "eventloop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string&& name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), std::move(name))
    , mutex_()
    , cond_()
    , callback_(cb)
{
}
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;

    if (loop_ != nullptr) {
        loop_->quit();
        // 等待底层子线程结束
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    // 启动底层新线程
    thread_.start();

    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        for (; loop == nullptr; ) {
            cond_.wait(lock);
        }
        loop = loop_;
    }

    return loop;
}

void EventLoopThread::threadFunc()
{
    // 创建独立的EventLoop，和上面的线程一一对应（one loop per thread）
    EventLoop loop;

    if (callback_) {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // EventLoop loop() => 开启底层poller.poll

    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
