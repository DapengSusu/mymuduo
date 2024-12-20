#pragma once

#include "noncopyable.h"
#include "eventloopthreadpool.h"
#include "inetaddress.h"
#include "callbacks.h"
#include "eventloop.h"
#include "acceptor.h"
#include "tcpconnection.h"
#include "buffer.h"

#include <unordered_map>
#include <functional>
#include <string>
#include <memory>
#include <atomic>

/**
    对外服务器编程使用的类
 */
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum class Option
    {
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop* loop, const InetAddress& listenAddr,
        const std::string& name, Option option = Option::kNoReusePort);
    ~TcpServer();

    // 设置底层subloop的个数
    void setThreadNum(int numThreads);

    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }

    // 开启服务器监听
    void start();

private:
    // 有一个新客户端连接，acceptor会执行这个回调
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* loop_; // baseLoop 用户定义的loop
    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; // 运行在mainLoop，监听新连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread

    ConnectionCallback connectionCallback_; // 有新连接时的回调
    MessageCallback messageCallback_; // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成后的回调

    ThreadInitCallback threadInitCallback_; // loop线程初始化的回调

    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_; // 保存所有的连接 <连接名name, 连接对象tcpconnection>
};
