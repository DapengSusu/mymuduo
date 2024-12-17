#include "poller.h"
#include "channel.h"

bool Poller::hasChannel(Channel* channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.cend() && it->second == channel;
}

//! 不在当前文件实现 `Poller* Poller::newDefaultPoller(EventLoop* loop)`
