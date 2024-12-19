#include "tcpconnection.h"
#include "eventloop.h"
#include "socket.h"
#include "channel.h"
#include "logger.h"

#include <cerrno>
#include <unistd.h>

using namespace std::placeholders;

static const EventLoop* CheckLoopNotNull(const EventLoop* loop)
{
    if (loop == nullptr) {
        LOG_FATAL("%s => TcpConnection loop is null!", __FUNCTION__);
    }

    return loop;
}


TcpConnection::TcpConnection(EventLoop* loop, const std::string& name,
    int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
    : loop_(const_cast<EventLoop*>(CheckLoopNotNull(loop)))
    , name_(name)
    , state_(kConnecting)
    // , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64*1024*1024) // 64M
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("%s => TcpConnection::ctor[%s] at fd:%d", __FUNCTION__, name_.c_str(), sockfd);

    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("%s => TcpConnection::dtor[%s] at fd:%d state:%d",
        __FUNCTION__, name_.c_str(), channel_->fd(), state_.load());
}

void TcpConnection::send(const std::string& buf)
{
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf.c_str(), buf.size());
        } else {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 之前调用过该connection的shutdown函数，不能再进行发送
    if (state_ == kDisconnected) {
        LOG_ERROR("%s => disconnected, give up writing", __FUNCTION__);
        return;
    }

    // channel第一次写数据且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0) { // 数据一次性发送完成，不用再给channel设置epollout事件了
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else { // nwrote < 0
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("%s => TcpConnection::sendInLoop failed!", __FUNCTION__);
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    if (!faultError && remaining > 0) {
        //* 说明当前这一次write没有把数据全部发送出去，剩余数据需要保存到缓冲区中，然后给channel注册epollout事件，
        //* poller发现TCP的发送缓冲区有空间，会通知相应channel调用writeCallback_回调方法,
        //* 也就是调用TcpConnection::handleWrite方法，把发送缓冲区中的数据全部发送完成

        // 目前发送缓冲区剩余待发送的数据长度
        auto oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ &&
            oldLen < highWaterMark_ && highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen+remaining));
        }
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) {
        // 当前outputBuffer_中的数据已经全部发送完成，关闭写端
        socket_->shutdownWrite();
    }
}

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    // 底层记录TCP连接生存期
    channel_->tie(shared_from_this());
    // 向poller注册channel的epollin事件
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected) {
        setState(kDisconnected);
        // 把channel所有感兴趣的事件从poller中删除
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    // 把channel从poller中删除
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    auto n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        // onMessage回调
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
        LOG_ERROR("%s => TcpConnection::handleRead", __FUNCTION__);
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting()) {
        int savedErrno = 0;
        auto n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }

                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_ERROR("%s => TcpConnection::handleWrite", __FUNCTION__);
        }
    } else {
        LOG_ERROR("%s => TcpConnection fd:%d is down, no more writing", __FUNCTION__, channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("%s => fd:%d state:%d", __FUNCTION__, channel_->fd(), state_.load());

    setState(kDisconnected);
    channel_->disableAll();

    auto guardThis(shared_from_this());
    if (connectionCallback_) {
        connectionCallback_(guardThis);
    }
    if (closeCallback_) {
        closeCallback_(guardThis);
    }
}

void TcpConnection::handleError()
{
    int err = 0;
    int optval;
    socklen_t optlen = sizeof optval;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }

    LOG_ERROR("%s => TcpConnection::handleError name:%s - SO_ERROR:%d", __FUNCTION__, name_.c_str(), err);
}
