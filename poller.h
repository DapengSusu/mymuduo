#pragma once

#include "noncopyable.h"
#include "timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

/**
    muduo库中多路事件分发器的核心IO复用模块
 */

class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    explicit Poller(EventLoop* loop) : ownerLoop_(loop) {}
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    // 判断channel是否在当前Poller中
    virtual bool hasChannel(Channel* channel) const;

    // 获取默认IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    // key：sockfd，value：fd所属的channel
    using ChannelMap = std::unordered_map<int, Channel*>;

    ChannelMap channels_;

private:
    EventLoop* ownerLoop_; // 定义Poller所属的事件循环EventLoop
};
