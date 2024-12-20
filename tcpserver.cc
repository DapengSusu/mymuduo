#include "tcpserver.h"
#include "tcpconnection.h"
#include "logger.h"

#include <cstring>

using namespace std::placeholders;

static const EventLoop* CheckLoopNotNull(const EventLoop* loop)
{
    if (loop == nullptr) {
        LOG_FATAL("%s => mainLoop is null!", __FUNCTION__);
    }

    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr,
    const std::string& name, Option option)
    : loop_(const_cast<EventLoop*>(CheckLoopNotNull(loop)))
    , ipPort_(listenAddr.toIpPort())
    , name_(name)
    , acceptor_(new Acceptor(loop, listenAddr, option == Option::kReusePort))
    , threadPool_(std::make_shared<EventLoopThreadPool>(loop, name_))
    , connectionCallback_()
    , messageCallback_()
    , started_(0)
    , nextConnId_(1)
{
    // 当有新用户连接时，会执行TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    LOG_INFO("TcpServer::~TcpServer [%s] destructing", name_.c_str());

    for (auto& item : connections_) {
        auto conn(item.second);
        item.second.reset(); // 释放TcpConnection对象

        // 销毁连接
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if (started_++ == 0) { // 防止一个TcpServer被start多次
        // 启动底层线程池
        threadPool_->start(threadInitCallback_);

        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    // 轮询算法，选择一个subloop来管理channel
    auto ioLoop = threadPool_->getNextLoop();
    char buf[64] = { 0 };
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName(name_ + buf);

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s",
        name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // 通过sockfd获取其绑定的本机ip和port
    sockaddr_in addr;
    ::memset(&addr, 0, sizeof addr);
    auto addrlen = static_cast<socklen_t>(sizeof addr);
    if (::getsockname(sockfd, (sockaddr*)&addr, &addrlen) < 0) {
        LOG_ERROR("%s => getsockname error: %d", __FUNCTION__, errno);
    }
    InetAddress localAddr(addr);
    // 根据连接成功的sockfd创建TcpConnection连接对象
    auto conn = std::make_shared<TcpConnection>(
        ioLoop, connName, sockfd, localAddr, peerAddr);

    connections_[connName] = conn;
    // TcpServer => TcpConnection => Channel => Poller => notify channel调用回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置如何关闭连接的回调 conn->shutdown()
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));

    // 直接调用TcpConnection::connectEstablished方法
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s",
        name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name());
    conn->getLoop()->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
