#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd) 
  : loop_(loop)
  , fd_(fd)
  , events_(0)
  , revents_(0)
  , index_(-1)
  , tied_(false)
{
}

Channel::~Channel()
{

}

// channel 的 tie 方法什么时候调用过？一个 TcpConnection 新连接创建的时候 TcpConnection => Channel
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

/**
 * 当改变 Channel 所表示 fd 的 events 事件后，update 负责在 poller 里面更改 fd 相应的事件 epoll_ctl
 */
void Channel::update()
{
    // 通过 Channel 所属的 EventLoop，调用 poller 的相应方法，注册 fd 的 events 事件
    loop_->updateChannel(this);
}

// 在 Channel 所属的 EventLoop 中，把当前的 Channel 删除掉
void Channel::remove()
{
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据 poller 通知的 Channel 发生的具体事件，由 Channel 负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("Channel handleEvent revents:%d\n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }

    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}

void Channel::setReadCallback(ReadEventCallback cb)
{
    readCallback_ = std::move(cb);
}

void Channel::setWriteCallback(EventCallback cb)
{
    writeCallback_ = std::move(cb);
}

void Channel::setCloseCallback(EventCallback cb)
{
    closeCallback_ = std::move(cb);
}

void Channel::setErrorCallback(EventCallback cb)
{
    errorCallback_ = std::move(cb);
}

int Channel::fd()
{
    return fd_;
}

int Channel::events() const
{
    return events_;
}

void Channel::set_revents(int revt)
{
    revents_ = revt;
}

void Channel::enableReading()
{
    events_ |= kReadEvent;
    update();
}

void Channel::disableReading()
{
    events_ &= ~kReadEvent;
    update();
}

void Channel::enableWriting()
{
    events_ |= kWriteEvent;
    update();
}

void Channel::disableWriting()
{
    events_ &= ~kWriteEvent;
    update();
}

void Channel::disableAll()
{
    events_ = kNoneEvent;
    update();
}

bool Channel::isNoneEvent() const
{
    return events_ == kNoneEvent;
}

bool Channel::isWriting() const
{
    return events_ & kWriteEvent;
}

bool Channel::isReading() const
{
    return events_ & kReadEvent;
}

int Channel::index()
{
    return index_;
}

void Channel::set_index(int idx)
{
    index_ = idx;
}

EventLoop *Channel::ownerLoop()
{
    return loop_;
}

