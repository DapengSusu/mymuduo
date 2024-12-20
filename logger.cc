#include <iostream>

#include "logger.h"
#include "timestamp.h"

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::log(const std::string& msg)
{
    // [日志级别] time msg
    switch (logLevel_) {
    case LogLevel::INFO:
        std::cout << "[INFO]";
        break;
    case LogLevel::ERROR:
        std::cout << "[ERROR]";
        break;
    case LogLevel::FATAL:
        std::cout << "[FATAL]";
        break;
    case LogLevel::DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }

    std::cout << "[" << Timestamp::now().toString() << "] " << msg << std::endl;
}
