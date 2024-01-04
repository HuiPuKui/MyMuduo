#include <stdlib.h>

#include "EPollPoller.h"
#include "Poller.h"

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // ���� poll ��ʵ��
    }
    else
    {
        return new EPollPoller(loop); // ���� epoll ��ʵ��
    }
}