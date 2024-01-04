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
    // 当有新用户连接时，会执行 TcpServer::newConnection 回调
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

// 设置底层 subLoop 的个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// 开启服务器监听       loop.loop()
void TcpServer::start()
{
    if (started_++ == 0) // 防止一个 TcpServer 对象被 Start 多次
    {
        threadPool_->start(threadInitCallback_); // 启动底层的 loop 线程池
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