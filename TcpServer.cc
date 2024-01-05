#include <functional>
#include <strings.h>

#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"
#include "InetAddress.h"

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
  , started_(0)
{
    // �������û�����ʱ����ִ�� TcpServer::newConnection �ص�
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second); // ����ֲ��� shared_ptr ����ָ����󣬳������ţ������Զ��ͷ� new ������ TcpConnection ������Դ��
        item.second.reset();

        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
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

// ��һ���ͻ��˵����ӣ�acceptor ��ִ������ص�����
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // ��ѯ�㷨��ѡ��һ�� subLoop�������� channel
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n", name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // ͨ�� sockfd ��ȡ��󶨵ı����� ip ��ַ�Ͷ˿���Ϣ
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("TcpServer::getLocalAddr");
    }
    InetAddress localAddr(local);

    // �������ӳɹ��� sockfd������ TcpConnection ���Ӷ���
    TcpConnectionPtr conn(new TcpConnection(
        ioLoop,
        connName,
        sockfd,
        localAddr,
        peerAddr
    ));
    connections_[connName] = conn;
    // ����Ļص������û����ø� TcpServer => TcpConnection => channel => poller => notify channel ���ûص�
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // ��������ιر����ӵĻص�
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );

    // ֱ�ӵ��� TcpConnection::connectEstablished
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn)
    );
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n", name_.c_str(), conn->name().c_str());

    size_t n = connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}