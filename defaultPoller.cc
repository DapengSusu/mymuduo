#include "poller.h"
#include "epollpoller.h"

#include <cstdlib>

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if (::getenv("MUDUO_USE_POLL")) {
        return nullptr; // 生成poll实例
    } else {
        return new EPollPoller(loop); // 生成epoll实例
    }
}
