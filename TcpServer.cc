#include "TcpServer.h"
#include "Logger.h"

#include <functional>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameAge, Option option)
  : loop_(CheckLoopNotNull(loop))
  , ipPort_(listenAddr.toIpPort())
  , name_(nameAge)
  , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
  , threadPool_(new EventLoopThreadPool(loop, name_))
  , connectionCallback_()
  , messageCallback_()
  , nextConnId_(1)
{
    // �������û�����ʱ����ִ�� TcpServer::newConnection �ص�
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{

}

void TcpServer::setThreadInitCallback(const ThreadInitCallback &cb)
{
    threadInitCallback_ = cb;
}

void TcpServer::setConnectionCallback(const ConnectionCallback &cb)
{
    connectionCallback_ = cb;
}

void TcpServer::setMessageCallback(const MessageCallback &cb)
{
    messageCallback_ = cb;
}

void TcpServer::setWriteCompleteCallback(const WriteCompleteCallback &cb)
{
    writeCompleteCallback_ = cb;
}

// ���õײ� subLoop �ĸ���
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// ��������������       loop.loop()
void TcpServer::start()
{
    if (started_++ == 0) // ��ֹһ�� TcpServer ���� Start ���
    {
        threadPool_->start(threadInitCallback_); // �����ײ�� loop �̳߳�
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{

}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{

}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{

}