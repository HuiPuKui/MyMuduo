#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

// 防止一个线程创建多个 EventLoop  thread_local
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的 Poller IO 复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建 wakeupfd，用来 notify 唤醒 subReactor 处理新来的 channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd err:%d \n", errno);
    }
    return evtfd;
}

Timestamp EventLoop::pollReturnTime() const
{
    return pollReturnTime_;
}

bool EventLoop::isInLoopThread() const
{
    return threadId_ == CurrentThread::tid();
}

EventLoop::EventLoop()
  : looping_(false)
  , quit_(false)
  , callingPendingFunctors_(false)
  , threadId_(CurrentThread::tid())
  , poller_(Poller::newDefaultPoller(this))
  , wakeupFd_(createEventfd())
  , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置 wakeupfd 的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个 EventLoop 都将监听 wakeupchannel 的 EPOLLIN 读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8", n);
    }
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_)
    {
        activeChannels_.clear();
        // 监听两类 fd 一种是 client 的 fd，一种 wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // Poller 监听哪些 channel 发生事件了，然后上报给 EventLoop，通知 channel 处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前 EventLoop 事件循环需要处理的回调操作
        /**
         * IO 线程 mainLoop accept fd 《= channel subloop
         * mainLoop 事先注册一个回调 cb（需要 subloop 来执行） wakeup  subloop 后执行下面的方法，执行之前 mainloop 注册的 cb 操作
         */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 退出事件循环：1. loop 在自己的线程中调用 quit 2. 在非 loop 的线程中，调用 loop 的 quit
void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopThread()) // 如果是在其他线程中，调用 quit   在一个 subloop（woker） 中，调用了 mainLoop（IO） 的 quit
    {
        wakeup();
    }
}

// 在当前 loop 中执行
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) // 在当前的 loop 线程中执行 cb
    {
        cb();
    }
    else // 在非 loop 线程中执行 cb，就需要唤醒 loop 所在线程，执行 cb
    {
        queueInLoop(cb);
    }
}

// 把 cb 放入队列中，唤醒 loop 所在的线程，执行 cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的，需要执行上面回调操作的 loop 的线程
    // callingPendingFunctors_ 的意思是：当前 loop 正在执行回调，但是 loop 又有了新的回调
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup(); // 唤醒 loop 所在线程
    }
}

// 用来唤醒 loop 所在的线程的 向 wakeupfd_ 写一个数据，wakeupChannel 就发生读事件，当前 loop 线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop 的方法 => Poller 的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前 loop 需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}