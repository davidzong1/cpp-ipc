#pragma once
#include <memory>
#include "libipc/udp.h"

namespace dzIPC {
class socket_sniffer
{
public:
    socket_sniffer() = default;
    ~socket_sniffer() = default;
    void create_sniffer(const std::string& topic_name, int domain_id);
    ipc::buffer try_recv() noexcept;
    ipc::buffer recv(std::uint64_t timeout_ms = 5'000) noexcept;

private:
    bool ready = false;
    std::unique_ptr<ipc::socket::UDPNode> node;
};
}   // namespace dzIPC