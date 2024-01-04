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

// ʱ��ѭ���ࣺ��Ҫ������������ģ�� Channel     Poller  (epoll �ĳ���)
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // �����¼�ѭ��
    void loop();
    // �˳��¼�ѭ��
    void quit();

    Timestamp pollReturnTime() const;

    // �ڵ�ǰ loop ��ִ��
    void runInLoop(Functor cb);
    // �� cb ��������У����� loop ���ڵ��̣߳�ִ�� cb
    void queueInLoop(Functor cb);

    // �������� loop ���ڵ��̵߳�
    void wakeup();

    // EventLoop �ķ��� => Poller �ķ���
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // �ж� EventLoop �����Ƿ��ڶ����Լ����߳�����
    bool isInLoopThread() const;
private:
    void handleRead(); // wake up
    void doPendingFunctors(); // ִ�лص�

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; // ԭ�Ӳ�����ͨ�� CAS ʵ�ֵ�
    std::atomic_bool quit_; // ��־�˳� loop ѭ��
    
    const pid_t threadId_; // ��¼��ǰ loop �����̵߳� id
    Timestamp pollReturnTime_; // poller ���ط����¼��� channels ��ʱ���
    std::unique_ptr<Poller> poller_;

    int wakeupFd_; // ��Ҫ���ã��� mainLoop ��ȡһ�����û��� channel��ͨ����ѯ�㷨ѡ��һ�� subLoop��ͨ���ó�Ա���� subLoop ���� channel
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // ��ʶ��ǰ loop �Ƿ�����Ҫִ�еĻص�����
    std::vector<Functor> pendingFunctors_; // �洢 loop ��Ҫִ�е����еĻص�����
    std::mutex mutex_; // �������������������� vector �������̰߳�ȫ����
};