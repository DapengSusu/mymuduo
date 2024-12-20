#include "eventloopthreadpool.h"
#include "eventloopthread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& name)
    : baseLoop_(baseLoop)
    , name_(name)
    , started_(false)
    , numThreads_(0)
    , next_(0u)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    started_ = true;

    for (int i = 0; i < numThreads_; ++i) {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);

        auto t = new EventLoopThread(cb, std::string(buf));
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        // 底层创建线程，绑定一个新的EventLoop，并返回该loop地址
        loops_.push_back(t->startLoop());
    }

    // 整个服务端只有一个线程，运行着baseLoop
    if (numThreads_ == 0 && cb) {
        cb(baseLoop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    auto loop = baseLoop_;

    // 通过轮询获取下一个处理事件的loop
    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if (next_ >= static_cast<int>(loops_.size())) {
            next_ = 0;
        }
    }

    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty()) {
        return std::vector<EventLoop*>(1, baseLoop_);
    } else {
        return loops_;
    }
}
