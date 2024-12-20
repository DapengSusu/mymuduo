#include "timestamp.h"

#include <cstdio>
#include <chrono>
#include <ctime>

std::string Timestamp::toString() const
{
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    // 转换为time_t类型，用于获取秒级时间
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    // 获取毫秒部分
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // 将时间转换为本地时间
    std::tm tmTime;
#ifdef _WIN32
    localtime_s(&tmTime, &now_time_t); // Windows平台使用localtime_s
#else
    localtime_r(&now_time_t, &tmTime); // 非Windows平台使用localtime_r
#endif

    char buf[128] = { 0 };
    snprintf(buf, 128, "%4d-%02d-%02d %02d:%02d:%02d:%02ld",
        tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday,
        tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec, ms.count());

    return std::string(buf);
}

Timestamp Timestamp::now()
{
    return Timestamp();
}
