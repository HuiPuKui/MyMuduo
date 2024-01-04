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