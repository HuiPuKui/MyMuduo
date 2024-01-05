#include <functional>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <string>

#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
  : loop_(CheckLoopNotNull(loop))
  , name_(nameArg)
  , state_(kConnecting)
  , reading_(true)
  , socket_(new Socket(sockfd))
  , channel_(new Channel(loop, sockfd))
  , localAddr_(localAddr)
  , peerAddr_(peerAddr)
  , highWaterMark_(64 * 1024 * 1024) // 64M
{
    // ����� channel ������Ӧ�Ļص�������poller �� channel ֪ͨ����Ȥ���¼������ˣ�channel ��ص���Ӧ�Ĳ�������
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
    );
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this)
    );
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this)
    );
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this)
    );

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), (int)state_);
}

EventLoop* TcpConnection::getLoop() const
{
    return loop_;
}

const std::string& TcpConnection::name() const
{
    return name_;
}

const InetAddress& TcpConnection::localAddress() const
{
    return localAddr_;
}

const InetAddress& TcpConnection::peerAddress() const
{
    return peerAddr_;
}

bool TcpConnection::connected() const
{
    return state_ == kConnected;
}

// ��������
void TcpConnection::send(const void *message, int len)
{

}

// �ر�����
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this)
        );
    }
}

void TcpConnection::setConnectionCallback(const ConnectionCallback& cb)
{
    connectionCallback_ = cb;
}

void TcpConnection::setMessageCallback(const MessageCallback& cb)
{
    messageCallback_ = cb;
}

void TcpConnection::setWriteCompleteCallback(const WriteCompleteCallback& cb)
{
    writeCompleteCallback_ = cb;
}

void TcpConnection::setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
{
    highWaterMarkCallback_ = cb;
    highWaterMark_ = highWaterMark;
}

void TcpConnection::setCloseCallback(const CloseCallback& cb)
{
    closeCallback_ = cb;
}

// ���ӽ���
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // �� poller ע�� channel �� epollin �¼�

    // �����ӽ�����ִ�лص�
    connectionCallback_(shared_from_this());
}

// ��������
void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // �� channel �����и���Ȥ���¼����� poller �� del ��
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // �� channel �� poller ��ɾ����
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if (n > 0)
    {
        // �ѽ������ӵ��û����пɶ��¼������ˣ������û�����Ļص����� onMessage
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    // ���� loop_ ��Ӧ�� thread �̣߳�ִ�лص�
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
                    );
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}

// poller => channel::closeCallback => TcpConnection::handleClose
void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // ִ�����ӹرյĻص�
    closeCallback_(connPtr); // �ر����ӵĻص���ִ�е��� TcpServer::removeConnection �ص�����
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen))
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(
                std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size())
            );
        }
    }
}

/**
 * ��������  Ӧ��д�Ŀ죬���ں˷�������������Ҫ�Ѵ���������д�뻺����������������ˮλ�ص�
 */

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // ֮ǰ���ù��� connection �� shutdown�������ٽ��з�����
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing");
        return ;
    }

    // ��ʾ channel_ ��һ�ο�ʼд���ݣ����һ�����û�д���������
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // ��Ȼ����������ȫ��������ɣ��Ͳ����ٸ� channel ���� epollout �¼���
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
                );
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE   RESET
                {
                    faultError = true;
                }
            }
        }
    }
    // ˵����ǰ��һ�� write����û�а�����ȫ�����ͳ�ȥ��ʣ���������Ҫ���浽�������У�
    // Ȼ��� channel ע�� epollout �¼���poller ���� tcp �ķ��ͻ������пռ䣬��֪ͨ��Ӧ�� sock-channel ������ handleWrite �ص�����
    // Ҳ���ǵ��� TcpConnection::handlewrite �������ѷ��ͻ������е�����ȫ���������
    if (!faultError && remaining > 0)
    {
        // Ŀǰ���ͻ�����ʣ��Ĵ��������ݵĳ���
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining)
            );
        }
        outputBuffer_.append((char*)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting(); // ����һ��Ҫע�� channel ��д�¼������� poller ����� channel ֪ͨ epollout
        }
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) // ˵����ǰ outputBuffer �е�����ȫ���������
    {
        socket_->shutdownWrite(); // �ر�д��
    }
}

void TcpConnection::setState(StateE state)
{
    state_ = state;
}

