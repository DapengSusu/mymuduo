#include "epollpoller.h"
#include "channel.h"
#include "logger.h"
#include "timestamp.h"

#include <cerrno>
#include <cstring>
#include <unistd.h>

// channel未添加到poller中（channel成员index_ = -1）
const int kNew = -1;
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

// epoll_create
EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)
{
    if (epollfd_ < 0) {
        LOG_FATAL("epoll_create failed:%d", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, EPollPoller::ChannelList* activeChannels)
{
    // 此函数中使用LOG_DEBUG输出更为合理
    LOG_DEBUG("EPollPoller::poll fd total count:%lu", channels_.size());

    int numEvents = ::epoll_wait(epollfd_, events_.data(),
        static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    auto now(Timestamp::now());

    if (numEvents > 0) {
        LOG_INFO("%s => %d events happened", __FUNCTION__, numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == static_cast<int>(events_.size())) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_INFO("%s timeout!", __FUNCTION__);
    } else {
        // EINTR表示外部中断
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }

    return now;
}

void EPollPoller::updateChannel(Channel* channel)
{
    const auto index = channel->index();
    LOG_INFO("%s => fd=%d, events=%d, index=%d",
        __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            const auto fd = channel->fd();
            channels_[fd] = channel;
        }

        update(EPOLL_CTL_ADD, channel);
        channel->set_index(kAdded);
    } else { // channel已经在poller上注册过了
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel)
{
    const auto fd = channel->fd();
    const auto index = channel->index();

    channels_.erase(fd);
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    for (auto i = 0; i < numEvents; ++i) {
        auto channel = static_cast<Channel*>(events_[i].data.ptr);

        channel->set_revents(events_[i].events);
        // EventLoop获取到Poller返回给它的所有发生事件的channel列表
        activeChannels->push_back(channel);
    }
}

void EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl del error:%d", errno);
        } else {
            LOG_FATAL("epoll_ctl add/mod error:%d", errno);
        }
    }
}
