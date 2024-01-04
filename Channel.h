#pragma once

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"

class EventLoop;

/**
 * �������EventLoop��Channel��Poller֮��Ĺ�ϵ <- Reactor ģ���϶�Ӧ Demultiplex
 * Channel ���Ϊͨ������װ�� sockfd �������Ȥ�� event���� EPOLLIN��EPOLLOUT �¼�
 * ������ poller ���صľ����¼� 
 */
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd �õ� poller ֪ͨ�Ժ󣬴����¼��ģ�������Ӧ�Ļص�����
    void handleEvent(Timestamp receiveTime);

    // ���ûص���������
    void setReadCallback(ReadEventCallback cb);
    void setWriteCallback(EventCallback cb);
    void setCloseCallback(EventCallback cb);
    void setErrorCallback(EventCallback cb);

    // ��ֹ�� channel ���ֶ� remove ����channel ����ִ�лص�����
    void tie(const std::shared_ptr<void>&);

    int fd();
    int events() const;
    void set_revents(int revt);
    
    // ���� fd ��Ӧ���¼�״̬
    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();
    
    // ���� fd ��ǰ���¼�״̬
    bool isNoneEvent() const;
    bool isWriting() const;
    bool isReading() const;

    int index();
    void set_index(int idx);

    // one loop per thread
    // ��ǰ Channel �����ĸ� EventLoop
    EventLoop *ownerLoop();
    void remove();
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    // ����Ȥ���¼���Ϣ��״̬������
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // �¼�ѭ��
    const int fd_;    // fd, Poller �����Ķ���
    int events_; // ע�� fd ����Ȥ���¼�
    int revents_; // poller ���صľ��巢�����¼�
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // ��Ϊ Channel ͨ�������ܹ���֪ fd ���շ����ľ�����¼� revents��������������þ����¼��Ļص�����
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};