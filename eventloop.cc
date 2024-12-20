#include "eventloop.h"
#include "channel.h"
#include "poller.h"
#include "logger.h"

#include <sys/eventfd.h>
#include <unistd.h>

// 防止一个线程创建多个EventLoop
thread_local EventLoop* t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间：10s
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventFd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL("create eventfd failed:%d", errno);
    }

    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , threadId_(current_thread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventFd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
    , callingPendingFunctors_(false)
{
    LOG_DEBUG("EventLoop created %p in thread %d", this, threadId_);

    if (t_loopInThisThread) {
        LOG_FATAL("Another EventLoop %p exists in this thread %d",
            t_loopInThisThread, threadId_);
    } else {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个EventLoop都将监听wakeup channel的EPOLLIN读事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO("%s => EventLoop %p start looping", __FUNCTION__, this);

    for(; !quit_; ) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (auto channel : activeChannels_) {
            // poller监听到哪些channel发生事件了，上报给eventloop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping", this);
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;

    // 如果在其它线程中调用quit，例如在一个subLoop（work）中调用了mainLoop（IO）的quit
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) { // 在当前的loop线程中执行cb
        cb();
    } else { // 唤醒loop所在线程执行cb
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒需要执行上面回调操作的相应loop线程
    // 1、loop不在当前线程
    // 2、当前loop正在执行回调，但又有了新回调
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) const
{
    return poller_->hasChannel(channel);
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    auto n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("%s writes %ld bytes instead of 8", __FUNCTION__, n);
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    auto n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("%s reads %ld bytes instead of 8", __FUNCTION__, n);
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const auto& functor : functors) {
        functor(); // 执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;
}
