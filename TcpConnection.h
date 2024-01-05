#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer => Acceptor => ��һ�����û����ӣ�ͨ�� accept �����õ� connfd
 * 
 * => TcpConnection ���ûص� => Channel => Poller => Channel �ص�����
 */

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop, const std::string &nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const;
    const std::string& name() const;
    const InetAddress& localAddress() const;
    const InetAddress& peerAddress() const;

    bool connected() const;

    // ��������
    void send(const std::string &buf);
    
    // �ر�����
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb);
    void setMessageCallback(const MessageCallback& cb);
    void setWriteCompleteCallback(const WriteCompleteCallback& cb);
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark);
    void setCloseCallback(const CloseCallback& cb);

    // ���ӽ���
    void connectEstablished();
    // ��������
    void connectDestroyed();
private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void setState(StateE state);

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void send(const void *message, int len);
    void sendInLoop(const void* message, size_t len);
    
    void shutdownInLoop();

    EventLoop *loop_; // ������Բ��� baseLoop����Ϊ TcpConnection ������ subLoop ��������
    const std::string name_;    
    std::atomic_int state_;
    bool reading_;

    // ����� Acceptor ����  Acceptor => mainLoop     TcpConnection => subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_; // ��������ʱ�Ļص�
    MessageCallback messageCallback_; // �ж�д��Ϣʱ�Ļص�
    WriteCompleteCallback writeCompleteCallback_; // ��Ϣ��������Ժ�Ļص�
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_;

    Buffer inputBuffer_; // �������ݵĻ�����
    Buffer outputBuffer_; // �������ݵĻ�����
};