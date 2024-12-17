#pragma once

#include <string>

#include "noncopyable.h"

// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(LogmsgFormat, ...) \
    do { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(LogLevel::INFO); \
        char buf[1024] = { 0 }; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(false)

#define LOG_ERROR(LogmsgFormat, ...) \
    do { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(LogLevel::ERROR); \
        char buf[1024] = { 0 }; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(false)

#define LOG_FATAL(LogmsgFormat, ...) \
    do { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(LogLevel::FATAL); \
        char buf[1024] = { 0 }; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(false)

#ifdef MUDEBUG
#define LOG_DEBUG(LogmsgFormat, ...) \
    do { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(LogLevel::DEBUG); \
        char buf[1024] = { 0 }; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(false)
#else
#define LOG_DEBUG(LogmsgFormat, ...)
#endif

// 定义日志级别 - INFO ERROR FATAL DEBUG
enum class LogLevel
{
    INFO,  // 流程信息
    ERROR, // 错误信息
    FATAL, // 严重错误
    DEBUG  // 调试信息
};

/**
    输出日志类
 */
class Logger : noncopyable
{
public:
    // 获取唯一的日志实例
    static Logger& instance();

    // 设置日志级别
    void setLogLevel(LogLevel level) { logLevel_ = level; }

    // 写日志
    void log(const std::string& msg);

private:
    Logger() = default;

    LogLevel logLevel_;
};
