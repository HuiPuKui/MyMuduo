#pragma once

/**
 * �û�ʹ�� muduo ��д����������
 */

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

// ����ķ��������ʹ�õ���
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb);
    void setConnectionCallback(const ConnectionCallback &cb);
    void setMessageCallback(const MessageCallback &cb);
    void setWriteCompleteCallback(const WriteCompleteCallback &cb);

    // ���õײ� subLoop �ĸ���
    void setThreadNum(int numThreads);

    // ��������������
    void start();
private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_; // baseLoop �û������ loop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; // ������ mainLoop��������Ǽ����������¼�

    std::unique_ptr<EventLoopThreadPool> threadPool_; // one loop per thread

    ConnectionCallback connectionCallback_; // ��������ʱ�Ļص�
    MessageCallback messageCallback_; // �ж�д��Ϣʱ�Ļص�
    WriteCompleteCallback writeCompleteCallback_; // ��Ϣ��������Ժ�Ļص�

    ThreadInitCallback threadInitCallback_; // loop �̳߳�ʼ���Ļص�
    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_; // �������е�����
};  