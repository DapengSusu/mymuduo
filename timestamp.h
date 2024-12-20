#pragma once

#include <string>

class Timestamp
{
public:
    Timestamp() = default;

    std::string toString() const;
    static Timestamp now();
};
