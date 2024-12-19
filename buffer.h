#pragma once

#include <vector>
#include <string>
#include <cstddef>

/**
    网络库底层的缓冲器类型
 */
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {
    }

    void swap(Buffer& rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const { return begin() + readerIndex_; }

    void retrieve(size_t len)
    {
        if (len < readableBytes()) { // 只读取了一部分可读数据（长度len）
            readerIndex_ += len;
        } else { // len == readableBytes()
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAsString(size_t len)
    {
        // 读取缓冲区中可读数据
        std::string result(peek(), len);
        // 缓冲区复位
        retrieve(len);

        return result;
    }

    // 将onMessage函数上报的Buffer数据转成string返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    // 向writeable缓冲区写入数据
    void append(const char* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    // 从fd上读取数据：Poller工作在LT模式，数据未读取完毕时，会持续上报
    ssize_t readFd(int fd, int* savedErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int* savedErrno);

    // 确保writeable缓冲区空间大于等于len，不足时扩容
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len) {
            makeSpace(len);
        }
    }

    const char* beginWrite() const { return begin() + writerIndex_; }
    char* beginWrite() { return begin() + writerIndex_; }

private:
    // vector底层数组的起始地址（只读）
    const char* begin() const { return &*buffer_.cbegin(); }
    // vector底层数组的起始地址
    char* begin() { return &*buffer_.begin(); }

    // 扩容空间
    void makeSpace(size_t len)
    {
        //* |kCheapPrepend| (free1) |reader----|writer   (free2)    |
        //* |kCheapPrepend|-----------------------len------------------------|
        //* free1是完成数据读取后的空闲空间，free2是未使用的可写空间
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) { // 底层数组扩容
            buffer_.resize(writerIndex_ + len);
        } else { // 移动数据：利用free1的空间
            // 未读取的数据长度
            auto readableLen = readableBytes();
            std::copy(begin()+readerIndex_, begin()+writerIndex_, begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readableLen;
        }
    }

private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};
