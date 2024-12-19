#include "buffer.h"

#include <sys/uio.h>
#include <cerrno>
#include <unistd.h>

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    // 栈上空间：64K
    char extrabuf[65536] = { 0 };
    struct iovec vec[2];
    const auto writableLen = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writableLen;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const auto iovcnt = (writableLen < sizeof extrabuf) ? 2 : 1;
    const auto n = ::readv(fd, vec, iovcnt);
    if (n < 0) {
        *savedErrno = errno;
    } else if (n <= writableLen) {
        writerIndex_ += n;
    } else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writableLen);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int* savedErrno)
{
    auto n = ::write(fd, peek(), readableBytes());
    if (n < 0) {
        *savedErrno = errno;
    }

    return n;
}
