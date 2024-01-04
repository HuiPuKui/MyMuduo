#pragma once

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"

class EventLoop;

/**
 * 理清楚：EventLoop、Channel、Poller之间的关系 <- Reactor 模型上对应 Demultiplex
 * Channel 理解为通道，封装了 sockfd 和其感兴趣的 event，如 EPOLLIN、EPOLLOUT 事件
 * 还绑定了 poller 返回的具体事件 
 */
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd 得到 poller 通知以后，处理事件的，调用相应的回调方法
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb);
    void setWriteCallback(EventCallback cb);
    void setCloseCallback(EventCallback cb);
    void setErrorCallback(EventCallback cb);

    // 防止当 channel 被手动 remove 掉，channel 还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd();
    int events() const;
    void set_revents(int revt);
    
    // 设置 fd 相应的事件状态
    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();
    
    // 返回 fd 当前的事件状态
    bool isNoneEvent() const;
    bool isWriting() const;
    bool isReading() const;

    int index();
    void set_index(int idx);

    // one loop per thread
    // 当前 Channel 属于哪个 EventLoop
    EventLoop *ownerLoop();
    void remove();
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    // 感兴趣的事件信息的状态的描述
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd, Poller 监听的对象
    int events_; // 注册 fd 感兴趣的事件
    int revents_; // poller 返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为 Channel 通道里面能够获知 fd 最终发生的具体的事件 revents，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};