#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// 时间循环类：主要包含了两个大模块 Channel     Poller  (epoll 的抽象)
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const;

    // 在当前 loop 中执行
    void runInLoop(Functor cb);
    // 把 cb 放入队列中，唤醒 loop 所在的线程，执行 cb
    void queueInLoop(Functor cb);

    // 用来唤醒 loop 所在的线程的
    void wakeup();

    // EventLoop 的方法 => Poller 的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断 EventLoop 对象是否在对象自己的线程里面
    bool isInLoopThread() const;
private:
    void handleRead(); // wake up
    void doPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; // 原子操作，通过 CAS 实现的
    std::atomic_bool quit_; // 标志退出 loop 循环
    
    const pid_t threadId_; // 记录当前 loop 所在线程的 id
    Timestamp pollReturnTime_; // poller 返回发生事件的 channels 的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_; // 主要作用：当 mainLoop 获取一个新用户的 channel，通过轮询算法选择一个 subLoop，通过该成员唤醒 subLoop 处理 channel
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // 标识当前 loop 是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; // 存储 loop 需要执行的所有的回调操作
    std::mutex mutex_; // 互斥锁，用来保护上面 vector 容器的线程安全操作
};