#include "timestamp.h"

#include <cstdio>

std::string Timestamp::toString() const
{
    char buf[128] = { 0 };
    tm* tmTime = localtime(&microSecondSinceEpoch_);

    snprintf(buf, 128, "%4d-%02d-%02d %02d:%02d:%02d",
        tmTime->tm_year + 1900, tmTime->tm_mon + 1, tmTime->tm_mday,
        tmTime->tm_hour, tmTime->tm_min, tmTime->tm_sec);

    return std::string(buf);
}

Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));
}
