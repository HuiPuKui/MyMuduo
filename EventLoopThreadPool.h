#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *baseloop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads);

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // ��������ڶ��߳��У�baseloop_ Ĭ������ѯ�ķ�ʽ���� channel �� subloop
    EventLoop *getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const;
    const std::string name() const;
private:

    EventLoop *baseLoop_; // EventLoop loop;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};