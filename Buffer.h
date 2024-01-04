#pragma once

#include <vector>

// 网络库底层的缓冲器类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 0;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize);

    size_t readableBytes() const;
    size_t writableBytes() const;
private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};