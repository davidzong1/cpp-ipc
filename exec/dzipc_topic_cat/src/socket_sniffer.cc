#include "socket_sniffer.h"
#include <string>
#include "dzIPC/common/hash.h"

namespace dzIPC {
void socket_sniffer::create_sniffer(const std::string& topic_name, int domain_id)
{
    std::string ip_hash = dzIPC::common::udp_discovery_addr_calculate(topic_name);
    uint16_t port_hash = dzIPC::common::udp_discovery_port_calculate(topic_name, domain_id);
    node = std::make_unique<ipc::socket::UDPNode>();
    node->create(topic_name.c_str(), ip_hash.c_str(), port_hash);
    ready = true;
}

ipc::buffer socket_sniffer::try_recv() noexcept
{
    if (!ready)
        return ipc::buffer{};
    try
    {
        return node->receive(0);
    }
    catch (const std::exception& e)
    {
        // std::fprintf(stderr, "socket_sniffer try_recv error: %s\n", e.what());
        return ipc::buffer{};
    }
}

ipc::buffer socket_sniffer::recv(std::uint64_t timeout_ms) noexcept
{
    if (!ready)
        return ipc::buffer{};
    try
    {
        return node->receive(timeout_ms);
    }
    catch (const std::exception& e)
    {
        // std::fprintf(stderr, "socket_sniffer recv error: %s\n", e.what());
        return ipc::buffer{};
    }
}
}   // namespace dzIPC
