#pragma once

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "Timestamp.h"

/**
 *  epoll �1�7�1�7?�1�7�1�7
 *  epoll_create
 *  epoll_ctl   add/modify/del
 *  epoll_wait
 */

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // �1�7�1�7?�1�7�1�7�1�7�1�7 Poller �1�7?�1�7�1�7??�1�7
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;
private:
    static const int kInitEventListSize = 16;

    // �1�7�1�7?�1�7�1�7?�1�7�1�7�1�7�1�7�1�7�1�7
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // �1�7�1�7�1�7�1�7 channel ?�1�7�1�7
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};