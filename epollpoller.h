#pragma once

#include "poller.h"

#include <vector>
#include <sys/epoll.h>

class EPollPoller : public Poller
{
public:
    explicit EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;

    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    static const int kInitEventListSize = 16;

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};
