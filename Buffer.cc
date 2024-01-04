#include <sys/uio.h>
#include <unistd.h>

#include "Buffer.h"

Buffer::Buffer(size_t initialSize)
  : buffer_(kCheapPrepend + kInitialSize)
  , readerIndex_(kCheapPrepend)
  , writerIndex_(kInitialSize)
{

}

size_t Buffer::readableBytes() const
{
    return writerIndex_ - readerIndex_;
}

size_t Buffer::writableBytes() const
{
    return buffer_.size() - writerIndex_;
}

size_t Buffer::prependableBytes() const
{
    return readerIndex_;
}

char *Buffer::begin()
{
    return &*buffer_.begin();
}

const char* Buffer::begin() const
{
    return &*buffer_.begin();
}

const char* Buffer::peek() const
{
    return begin() + readerIndex_;
}

void Buffer::retrieve(size_t len)
{
    if (len < readableBytes())
    {
        readerIndex_ += len; // 应用只读取了可读缓冲区数据的一部分，就是 len，还剩下 readerIndex_ += len    ->    writerIndex_
    }
    else // len == readableBytes()
    {
        retrieveAll();
    }
}

void Buffer::retrieveAll()
{
    readerIndex_ = writerIndex_ = kCheapPrepend;
}

std::string Buffer::retrieveAllAsString()
{
    return retrieveAsString(readableBytes()); // 应用可读取数据的长度
}

std::string Buffer::retrieveAsString(size_t len)
{
    std::string result(peek(), len);
    retrieve(len); // 上面一句把缓冲区中可读的数据，已经读取出来，这里肯定要对缓冲区进行复位操作
    return result;
}

void Buffer::ensureWritableBytes(size_t len)
{
    if (writableBytes() < len)
    {
        makeSpace(len); // 扩容函数
    }
}

void Buffer::makeSpace(size_t len)
{
    if (writableBytes() + prependableBytes() < len + kCheapPrepend)
    {
        buffer_.resize(writerIndex_ + len);
    }
    else
    {
        size_t readable = readableBytes();
        std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
    }
}

// 把[data, data + len] 内存上的数据，添加到 writable 缓冲区当中
void Buffer::append(const char *data, size_t len)
{
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
}

char* Buffer::beginWrite()
{
    return begin() + writerIndex_;
}

const char* Buffer::beginWrite() const
{
    return begin() + writerIndex_;
}

// 从 fd 上读取数据  Poller 工作在 LT 模式
// Buffer 缓冲区是有大小的，但是从 fd 上读数据的时候，却不知道 tcp 数据最终的大小
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; // 栈上的内存空间
    struct iovec vec[2];

    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable) // Buffere 的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_ += n;
    }
    else // extrabuf 里面也写入了数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);  // writerIndex_ 开始写 n - writable 大小的数据
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}