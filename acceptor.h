#pragma once

#include "noncopyable.h"
#include "channel.h"
#include "socket.h"

#include <functional>

class InetAddress;
class EventLoop;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int/* sockfd */, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenaddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        newConnectionCallback_ = std::move(cb);
    }

    bool listening() const { return listening_; }
    void listen();

private:
    // listenfd有事件发生：新用户连接
    void handleRead();

    EventLoop* loop_; // Acceptor用的就是用户定义的baseLoop，即mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
};
