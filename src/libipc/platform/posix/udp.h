
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "libipc/buffer.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
namespace ipc
{
    namespace detail
    {
        namespace socket
        {

            class UDPNode
            {
                char name[256]{};
                char ip[16]{};
                uint16_t port{};
                int server_fd{-1};
                std::vector<char> temp_buffer;

            public:
                UDPNode() {}
                /* Instantiation */
                UDPNode(const char *name, const char *ip, uint16_t port) { create(name, ip, port); }
                void create(const char *name, const char *ip, uint16_t port)
                {
                    strncpy(this->name, name, sizeof(this->name) - 1);
                    strncpy(this->ip, ip, sizeof(this->ip) - 1);
                    this->port = port;
                    temp_buffer.resize(1472); // 预分配最大UDP报文长度
                }

                bool connect()
                {
                    if (server_fd >= 0)
                    {
                        close();
                    }

                    server_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
                    if (server_fd < 0)
                    {
                        return false;
                    }

                    int reuse = 1;
                    ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#ifdef SO_REUSEPORT
                    ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
#endif

                    sockaddr_in local_addr{};
                    local_addr.sin_family = AF_INET;
                    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                    local_addr.sin_port = htons(port);

                    if (::bind(server_fd, reinterpret_cast<sockaddr *>(&local_addr), sizeof(local_addr)) < 0)
                    {
                        ::close(server_fd);
                        server_fd = -1;
                        return false;
                    }

                    ip_mreq mreq{};
                    if (::inet_pton(AF_INET, ip, &mreq.imr_multiaddr) != 1)
                    {
                        ::close(server_fd);
                        server_fd = -1;
                        return false;
                    }
                    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

                    if (::setsockopt(server_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                    {
                        ::close(server_fd);
                        server_fd = -1;
                        return false;
                    }

                    unsigned char ttl = 1;
                    ::setsockopt(server_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
                    unsigned char loop = 1;
                    ::setsockopt(server_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
                    return true;
                }

                bool send(std::vector<char> &data)
                {
                    if (server_fd < 0 || data.size() == 0)
                    {
                        return false;
                    }

                    if (!data.data())
                    {
                        return false;
                    }

                    sockaddr_in dst_addr{};
                    dst_addr.sin_family = AF_INET;
                    dst_addr.sin_port = htons(port);
                    if (::inet_pton(AF_INET, ip, &dst_addr.sin_addr) != 1)
                    {
                        return false;
                    }
                    if (data.size() > 1472) // UDP最大有效载荷限制
                    {
                        return false;
                    }
                    else
                    {
                        ssize_t sent = ::sendto(server_fd, data.data(), data.size(), 0,
                                                reinterpret_cast<sockaddr *>(&dst_addr),
                                                sizeof(dst_addr));
                        if (sent < 0)
                            return false;
                    }
                    return true;
                }

                bool receive(std::vector<char> &buffer, uint64_t tm)
                {
                    if (server_fd < 0)
                        return false;
                    int err_cnt = 0;
                    if (tm == ipc::invalid_value)
                    {
                        for (;;)
                        {
                            ssize_t received = ::recvfrom(server_fd, temp_buffer.data(), temp_buffer.size(), 0, nullptr, nullptr);
                            if (received >= 0)
                            {
                                buffer.assign(temp_buffer.data(), temp_buffer.data() + received);
                                return true;
                            }
                            if (errno == EINTR)
                            {
                                err_cnt++;
                                if (err_cnt >= 100)
                                {
                                    return false; // 避免无限重试
                                }
                                continue;
                            }
                            return false;
                        }
                    }

                    // 2. 处理定时等待逻辑
                    auto start_time = std::chrono::steady_clock::now();
                    uint64_t remaining_ms = tm;
                    int err_cnt = 0;

                    while (remaining_ms > 0)
                    {
                        fd_set read_fds;
                        FD_ZERO(&read_fds);
                        FD_SET(server_fd, &read_fds);

                        timeval timeout;
                        timeout.tv_sec = static_cast<long>(remaining_ms / 1000);
                        timeout.tv_usec = static_cast<long>((remaining_ms % 1000) * 1000);

                        int ret = ::select(server_fd + 1, &read_fds, nullptr, nullptr, &timeout);

                        if (ret > 0)
                        {
                            ssize_t received = ::recvfrom(server_fd, temp_buffer.data(), temp_buffer.size(), MSG_DONTWAIT, nullptr, nullptr);
                            if (received >= 0)
                            {
                                buffer.assign(temp_buffer.data(), temp_buffer.data() + received);
                                return true;
                            }
                            if ((errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) && ++err_cnt < 10)
                            {
                                goto refresh_time;
                                continue;
                            }
                            return false;
                        }
                        else if (ret == 0)
                        {
                            return false; // 真正超时
                        }
                        else
                        {
                            if (errno == EINTR && ++err_cnt < 5)
                            {
                                goto refresh_time;
                                continue;
                            }
                            return false;
                        }

                    refresh_time:
                        auto now = std::chrono::steady_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
                        if (static_cast<uint64_t>(elapsed) >= tm)
                            return false;
                        remaining_ms = tm - static_cast<uint64_t>(elapsed);
                    }

                    return false;
                }

                bool close()
                {
                    if (server_fd < 0)
                    {
                        return true;
                    }

                    ip_mreq mreq{};
                    if (::inet_pton(AF_INET, ip, &mreq.imr_multiaddr) == 1)
                    {
                        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
                        ::setsockopt(server_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
                    }

                    ::close(server_fd);
                    server_fd = -1;
                    return true;
                }
            };
        }
    }
}