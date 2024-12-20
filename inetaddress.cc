#include "inetaddress.h"

#include <arpa/inet.h>
#include <cstring>

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    ::memset(&addr_, 0, sizeof addr_);

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const
{
    char buf[64] = { 0 };
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);

    return std::string(buf);
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = { 0 };
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    sprintf(buf + strlen(buf), ":%u", toPort());

    return std::string(buf);
}
