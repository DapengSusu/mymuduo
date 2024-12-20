#pragma once

#include "noncopyable.h"
#include "currentthread.h"
#include "timestamp.h"

#include <functional>
#include <sched.h>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

/**
    事件循环类，主要包含`Channel`和`Poller`（epoll的抽象）两个模块
 */
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前的loop中执行cb
    void runInLoop(Functor cb);
    // 把cb放入队列中，唤醒loop所在线程，执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在的线程，向wakeupfd写一个数据，wakeupchannel发生读事件，当前loop线程就会被唤醒
    void wakeup();

    // 调用Poller的updateChannel方法
    void updateChannel(Channel* channel);
    // 调用Poller的removeChannel方法
    void removeChannel(Channel* channel);
    // 调用Poller的hasChannel方法
    bool hasChannel(Channel* channel) const;

    // 判断EventLoop对象是否在自己的线程里
    bool isInLoopThread() const { return threadId_ == current_thread::tid(); }

private:
    // wake up
    void handleRead();
    // 执行回调
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; // 原子操作，通过CAS实现
    std::atomic_bool quit_; // 标识退出loop循环

    const pid_t threadId_; // 记录当前loop所在线程的ID

    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_; // 当mainLoop获取一个新用户的channel时通过轮询算法选择一个subLoop，通过该成员唤醒subLoop处理channel
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有回调操作
    std::mutex mutex_; // 互斥锁，保护上面vector容器的线程安全操作
};
