#pragma once

#include <cstdint>
#include <string>
#define UDP_DISCOVERY_BASE_PORT 11451
namespace dzIPC::common
{
    uint64_t fnv1a64(const std::string &data);
    uint16_t udp_discovery_port_calculate(const std::string &topic_name, int domain_id);
    std::string udp_discovery_addr_calculate(const std::string &topic_name);
}