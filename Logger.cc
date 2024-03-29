#include "Logger.h"
#include "Timestamp.h"

#include <iostream>

// 获取日志唯一的实例对象
Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

// 设置日志级别
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}

// 写日志 [信息级别] time ： msg
void Logger::log(std::string msg)
{
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
    case FATAL:
        std::cout << "[FATAL]";
    case DEBUG:
        std::cout << "[DEBUG]";
    default:
        break;
    }

    // 打印时间 和 msg
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}