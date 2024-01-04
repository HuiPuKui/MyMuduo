#pragma once

#include <vector>
#include <string>
#include <algorithm>

// �����ײ�Ļ��������Ͷ���
class Buffer
{
public:
    static const size_t kCheapPrepend = 0;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize);

    size_t readableBytes() const;
    size_t writableBytes() const;

    size_t prependableBytes() const;

    const char* peek() const; // ���ػ������пɶ����ݵ���ʼ��ַ

    void retrieve(size_t len);
    void retrieveAll();
    std::string retrieveAllAsString();
    std::string retrieveAsString(size_t len);
    
    void ensureWritableBytes(size_t len);

    void append(const char *data, size_t len);

    char* beginWrite();
    const char* beginWrite() const;

    ssize_t readFd(int fd, int *saveErrno);
    ssize_t writeFd(int fd, int *saveErrno);
private:
    char* begin(); // vector �ײ�������Ԫ�صĵ�ַ��Ҳ�����������ʼ��ַ
    const char* begin() const;

    void makeSpace(size_t len);



    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};