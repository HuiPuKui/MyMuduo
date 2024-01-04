#pragma once

#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "Timestamp.h"

class Channel;
class EventLoop;

// muduo ���ж�·�¼��ַ����ĺ��� IO ����ģ��
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // ������ IO ���ñ���ͬ��Ľӿ�
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // �жϲ��� channel �Ƿ��ڵ�ǰ Poller ����
    bool hasChannel(Channel *channel) const;

    // EventLoop ����ͨ���ýӿڻ�ȡĬ�ϵ� IO ���þ���ʵ��
    static Poller *newDefaultPoller(EventLoop *loop);
protected:
    // map �� key��sockfd   value��sockfd ������ channel ͨ������
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_; // ���� Poller �������¼�ѭ�� EventLoop
};