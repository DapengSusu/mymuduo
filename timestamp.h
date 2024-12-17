#pragma once

#include <cstdint>
#include <string>

class Timestamp
{
public:
    Timestamp() : microSecondSinceEpoch_(0) {}
    explicit Timestamp(int64_t microSecondSinceEpoch)
        : microSecondSinceEpoch_(microSecondSinceEpoch) {}

    std::string toString() const;
    static Timestamp now();

private:
    int64_t microSecondSinceEpoch_;
};
