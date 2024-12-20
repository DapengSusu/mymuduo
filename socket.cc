#include "socket.h"
#include "logger.h"
#include "inetaddress.h"

#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

Socket::~Socket()
{
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localaddr)
{
    if (0 != ::bind(sockfd_, (const sockaddr*)&localaddr.getSockAddr(), sizeof(sockaddr_in))) {
        LOG_FATAL("bind sockfd %d failed!", sockfd_);
    }
}

void Socket::listen()
{
    if (0 != ::listen(sockfd_, 1024)) {
        LOG_FATAL("listen sockfd %d failed!", sockfd_);
    }
}

int Socket::accept(InetAddress* peeraddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    ::memset(&addr, 0, sizeof addr);

    int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) {
        LOG_INFO("%s => accept ok", __FUNCTION__);
        peeraddr->setSockAddr(addr);
    }

    return connfd;
}

void Socket::shutdownWrite()
{
    if (0 != ::shutdown(sockfd_, SHUT_WR)) {
        LOG_ERROR("shutdown sockfd %d write failed", sockfd_);
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
        &optval, sizeof optval);
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
        &optval, sizeof optval);
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
        &optval, sizeof optval);
#else
    if (on) {
        LOG_ERROR("SO_REUSEPORT is not supported!");
    }
#endif
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
        &optval, sizeof optval);
}
