// dzipc_topic_cat: a "ros2 topic echo"-style passive sniffer for cpp-ipc
// channels. Attaches to an existing channel by name and prints each message
// without disturbing the publisher (no receiver registration).

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <string>
#include "shm_sniffer.h"

namespace dzIPC {

void print_help(char const* prog)
{
    std::fprintf(stderr,
                 "Usage: %s -n NAME [shm_sniffer_options]\n"
                 "  -n, --name NAME         channel name (required)\n"
                 "  -p, --prefix PFX        channel prefix (default: empty)\n"
                 "  -t, --topology T        server | route   (default: route)\n"
                 "  -N, --count N           stop after N messages (default: unlimited)\n"
                 "  -T, --timeout MS        per-message timeout in ms (default: forever)\n"
                 "  -w, --width W           bytes per hex row (default: 16)\n"
                 "      --no-hex            do not print hex column\n"
                 "      --no-ascii          do not print ascii column\n"
                 "      --quiet             skip meta header per message\n"
                 "  -h, --help              this help\n"
                 "\n"
                 "Notes:\n"
                 "  - Passive observer: publisher is not slowed down, but a fast\n"
                 "    publisher may overwrite slots before they are read; the count\n"
                 "    of dropped messages is reported in the meta header.\n",
                 prog);
}

bool parse_topology(std::string const& s, ipc::sniffer::topology& out)
{
    if (s == "server")
    {
        out = ipc::sniffer::topology::server;
        return true;
    }
    if (s == "route")
    {
        out = ipc::sniffer::topology::route;
        return true;
    }
    if (s == "channel")
    {
        out = ipc::sniffer::topology::channel;
        return true;
    }
    return false;
}

bool parse_args(int argc, char** argv, shm_sniffer_options& opt)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        auto need = [&](char const* flag)
        {
            if (i + 1 >= argc)
            {
                std::fprintf(stderr, "missing argument for %s\n", flag);
                std::exit(2);
            }
            return std::string(argv[++i]);
        };

        if (a == "-h" || a == "--help")
        {
            print_help(argv[0]);
            return false;
        }
        else if (a == "-n" || a == "--name")
            opt.name = need("--name");
        else if (a == "-p" || a == "--prefix")
            opt.pref = need("--prefix");
        else if (a == "-t" || a == "--topology")
        {
            auto s = need("--topology");
            if (!parse_topology(s, opt.topo))
            {
                std::fprintf(stderr, "unknown topology: %s\n", s.c_str());
                return false;
            }
        }
        else if (a == "-N" || a == "--count")
            opt.max = static_cast<std::size_t>(std::stoull(need("--count")));
        else if (a == "-T" || a == "--timeout")
            opt.timeout_ms = static_cast<std::uint64_t>(std::stoull(need("--timeout")));
        else if (a == "-w" || a == "--width")
            opt.width = (std::max) (static_cast<std::size_t>(1), static_cast<std::size_t>(std::stoul(need("--width"))));
        else if (a == "--no-hex")
            opt.hex = false;
        else if (a == "--no-ascii")
            opt.ascii = false;
        else if (a == "--quiet")
            opt.quiet_meta = true;
        else
        {
            std::fprintf(stderr, "unknown arg: %s\n", a.c_str());
            print_help(argv[0]);
            return false;
        }
    }
    if (opt.name.empty())
    {
        std::fprintf(stderr, "error: --name is required\n\n");
        print_help(argv[0]);
        return false;
    }
    return true;
}

void hexdump(void const* data, std::size_t size, shm_sniffer_options const& opt)
{
    auto const* p = static_cast<unsigned char const*>(data);
    for (std::size_t off = 0; off < size; off += opt.width)
    {
        std::printf("  %08zx  ", off);
        if (opt.hex)
        {
            for (std::size_t j = 0; j < opt.width; ++j)
            {
                if (off + j < size)
                    std::printf("%02x ", p[off + j]);
                else
                    std::printf("   ");
            }
        }
        if (opt.ascii)
        {
            std::printf(" |");
            for (std::size_t j = 0; j < opt.width && off + j < size; ++j)
            {
                unsigned char c = p[off + j];
                std::putchar((c >= 0x20 && c < 0x7f) ? c : '.');
            }
            std::putchar('|');
        }
        std::putchar('\n');
    }
}

std::atomic<bool> g_stop{false};

void on_sigint(int)
{
    g_stop.store(true);
}

void shm_sniffer::create_sniffer(shm_sniffer_options& opt)

{
    std::unique_ptr<ipc::sniffer> s = std::make_unique<ipc::sniffer>();
    bool ok = opt.pref.empty() ? s->open(opt.name.c_str(), opt.topo)
                               : s->open(ipc::prefix{opt.pref.c_str()}, opt.name.c_str(), opt.topo);
    if (!ok)
    {
        std::fprintf(stderr, "error: failed to open channel '%s' (prefix='%s', topology=%s)\n", opt.name.c_str(),
                     opt.pref.c_str(),
                     opt.topo == ipc::sniffer::topology::server  ? "server"
                     : opt.topo == ipc::sniffer::topology::route ? "route"
                                                                 : "channel");

        std::exit(1);
    }
    if (opt.verbose)
    {
        std::fprintf(stderr,
                     "[topic_cat] listening on '%s' (prefix='%s', topology=%s, "
                     "active receivers=%zu)\n",
                     s->name(), s->prefix_str(), opt.topo == ipc::sniffer::topology::server ? "server" : "route",
                     s->receiver_connections());
    }
    ready = true;
}

shm_sniffer_info shm_sniffer::try_recv() noexcept
{
    ipc::sniffer::meta m;
    if (!ready)
        return shm_sniffer_info{ipc::buff_t{}, ipc::sniffer::meta{}};
    return shm_sniffer_info{s->try_recv(&m), m};
}

shm_sniffer_info shm_sniffer::recv(std::uint64_t timeout_ms) noexcept
{
    ipc::sniffer::meta m;
    if (!ready)
        return shm_sniffer_info{ipc::buff_t{}, ipc::sniffer::meta{}};
    return shm_sniffer_info{s->recv(timeout_ms, &m), m};
}
}   // namespace dzIPC

// int main(int argc, char** argv)

// {
//     shm_sniffer_options opt;
//     if (!parse_args(argc, argv, opt))
//         return 1;

//     std::signal(SIGINT, on_sigint);
//     std::signal(SIGTERM, on_sigint);

//     ipc::sniffer s;
//     bool ok = opt.pref.empty() ? s.open(opt.name.c_str(), opt.topo)
//                                : s.open(ipc::prefix{opt.pref.c_str()}, opt.name.c_str(), opt.topo);
//     if (!ok)
//     {
//         std::fprintf(stderr, "error: failed to open channel '%s' (prefix='%s', topology=%s)\n", opt.name.c_str(),
//                      opt.pref.c_str(),
//                      opt.topo == ipc::sniffer::topology::server  ? "server"
//                      : opt.topo == ipc::sniffer::topology::route ? "route"
//                                                                  : "channel");
//         return 1;
//     }

//     std::fprintf(stderr,
//                  "[topic_cat] listening on '%s' (prefix='%s', topology=%s, "
//                  "active receivers=%zu)\n",
//                  s.name(), s.prefix_str(), opt.topo == ipc::sniffer::topology::server ? "server" : "route",
//                  s.receiver_connections());

//     std::size_t got = 0;
//     while (!g_stop.load())
//     {
//         ipc::sniffer::meta m;
//         ipc::buff_t buf = s.recv(opt.timeout_ms, &m);
//         if (buf.empty())
//         {
//             if (opt.timeout_ms != ipc::invalid_value)
//             {
//                 std::fprintf(stderr, "[topic_cat] timeout\n");
//                 break;
//             }
//             continue;
//         }

//         if (!opt.quiet_meta)
//         {
//             auto now = std::chrono::system_clock::now().time_since_epoch();
//             auto us = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
//             std::printf("---- msg #%u  cc_id=%u  size=%zu  dropped=%llu  ts=%lld.%06lld\n", m.msg_id, m.cc_id,
//                         buf.size(), static_cast<unsigned long long>(m.dropped), static_cast<long long>(us / 1'000'000),
//                         static_cast<long long>(us % 1'000'000));
//         }
//         if (opt.hex || opt.ascii)
//         {
//             hexdump(buf.data(), buf.size(), opt);
//         }
//         std::fflush(stdout);

//         if (opt.max != 0 && ++got >= opt.max)
//             break;
//     }

//     return 0;
// }
