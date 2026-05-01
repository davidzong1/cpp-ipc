#pragma once
#include <memory>
#include "libipc/ipc.h"
#include "libipc/sniffer.h"

namespace dzIPC {
constexpr ipc::sniffer::topology topic_sniffer_topology = ipc::sniffer::topology::route;
constexpr ipc::sniffer::topology service_sniffer_topology = ipc::sniffer::topology::server;

struct shm_sniffer_options
{
    std::string name;
    std::string pref;
    ipc::sniffer::topology topo = topic_sniffer_topology;
    bool hex = true;
    bool ascii = true;
    std::size_t max = 0;   // 0 = unlimited
    std::uint64_t timeout_ms = ipc::invalid_value;
    std::size_t width = 16;   // bytes per row in hex view
    bool quiet_meta = false;
    bool verbose = false;
};

struct shm_sniffer_info
{
    ipc::buff_t buf;
    ipc::sniffer::meta m;
};

class shm_sniffer
{
public:
    shm_sniffer() = default;
    ~shm_sniffer() = default;

    shm_sniffer(shm_sniffer const&) = delete;
    shm_sniffer& operator=(shm_sniffer const&) = delete;

    shm_sniffer(shm_sniffer_options& opt) { create_sniffer(opt); }

    void create_sniffer(shm_sniffer_options& opt);

    shm_sniffer_info try_recv() noexcept;

    shm_sniffer_info recv(std::uint64_t timeout_ms = 5'000) noexcept;

private:
    bool ready = false;
    std::unique_ptr<ipc::sniffer> s;
};
}   // namespace dzIPC