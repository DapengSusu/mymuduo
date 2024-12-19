#include "tcpserver.h"
#include "logger.h"

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
    , nextConnId_(1)
{
    // 当有新用户连接时，会执行TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
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

}
