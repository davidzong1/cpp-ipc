#include "dzIPC/common/data_rev.h"
#include "ipc_msg/ipc_msg_base/udp_rtps_ack_msg.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>
#include <unordered_set>
#include <vector>
namespace dzIPC
{
    namespace socket
    {
#define UDP_MAX_SIZE 1472
        namespace
        {
            constexpr std::size_t TAIL_SIZE = 12;
            constexpr int SEND_RETRY_MAX = 10;
            constexpr int RTPS_MAX_NACK_ROUND = 3;

            struct chunk_meta
            {
                uint16_t page_cnt{0};
                uint32_t total_size{0};
                uint32_t msg_id{0};
            };

            uint32_t local_node_id()
            {
                static const uint32_t id = []
                {
                    const auto now = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
                    return static_cast<uint32_t>((now >> 32) ^ now ^ 0xA53C9E17u);
                }();
                return id;
            }

            bool parse_tail(const ipc::buffer &buf, ipc_tail_msg &tail)
            {
                if (buf.size() < TAIL_SIZE)
                {
                    return false;
                }

                const auto *data_ptr = static_cast<const uint8_t *>(buf.data());
                const size_t data_len = buf.size();
                tail.page_cnt = static_cast<uint16_t>(data_ptr[data_len - 12]) << 8 |
                                static_cast<uint16_t>(data_ptr[data_len - 11]);
                tail.now_page = static_cast<uint16_t>(data_ptr[data_len - 10]) << 8 |
                                static_cast<uint16_t>(data_ptr[data_len - 9]);
                tail.total_size = static_cast<uint32_t>(data_ptr[data_len - 8]) << 24 |
                                  static_cast<uint32_t>(data_ptr[data_len - 7]) << 16 |
                                  static_cast<uint32_t>(data_ptr[data_len - 6]) << 8 |
                                  static_cast<uint32_t>(data_ptr[data_len - 5]);
                tail.dz_ipc_msg_id = static_cast<uint32_t>(data_ptr[data_len - 4]) << 24 |
                                     static_cast<uint32_t>(data_ptr[data_len - 3]) << 16 |
                                     static_cast<uint32_t>(data_ptr[data_len - 2]) << 8 |
                                     static_cast<uint32_t>(data_ptr[data_len - 1]);
                return true;
            }

            bool copy_page_to_buffer(const ipc::buffer &page_buf,
                                     const chunk_meta &meta,
                                     std::vector<uint8_t> &assemble,
                                     std::vector<uint8_t> &received,
                                     std::size_t &received_cnt)
            {
                ipc_tail_msg tail;
                if (!parse_tail(page_buf, tail))
                {
                    return false;
                }

                if (tail.page_cnt != meta.page_cnt || tail.total_size != meta.total_size || tail.dz_ipc_msg_id != meta.msg_id)
                {
                    return false;
                }
                if (tail.now_page == 0 || tail.now_page > meta.page_cnt)
                {
                    return false;
                }
                const std::size_t payload_size = page_buf.size();
                const std::size_t offset = static_cast<std::size_t>(tail.now_page - 1) * UDP_MAX_SIZE;
                if (offset + payload_size > meta.total_size || payload_size > UDP_MAX_SIZE)
                {
                    return false;
                }

                if (received[tail.now_page] != 0)
                {
                    return true;
                }

                std::memcpy(assemble.data() + offset, page_buf.data(), payload_size);
                received[tail.now_page] = 1;
                ++received_cnt;
                return true;
            }

            uint64_t elapsed_ms(const std::chrono::steady_clock::time_point &begin)
            {
                return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count());
            }

            bool recv_chunk_common(std::shared_ptr<ipc::socket::UDPNode> &node, std::shared_ptr<ipc_msg_base> &msg_ptr, uint64_t tm)
            {
                const auto begin = std::chrono::steady_clock::now();
                auto remain_ms = [&]() -> uint64_t
                {
                    if (tm == ipc::invalid_value)
                    {
                        return tm;
                    }
                    const uint64_t used = elapsed_ms(begin);
                    return used >= tm ? 0 : (tm - used);
                };

                chunk_meta meta{};
                ipc::buffer first_page;
                ipc_rtps_nack_msg nack_msg;

                while (true)
                {
                    const uint64_t rem = remain_ms();
                    if (tm != ipc::invalid_value && rem == 0)
                    {
                        return false;
                    }

                    ipc::buffer cache_buffer = node->receive(rem);
                    if (cache_buffer.empty())
                    {
                        return false;
                    }

                    if (!msg_ptr->check_id(cache_buffer))
                    {
                        continue;
                    }

                    ipc_tail_msg tail;
                    if (!parse_tail(cache_buffer, tail) || tail.page_cnt == 0 || tail.total_size == 0)
                    {
                        continue;
                    }

                    first_page = std::move(cache_buffer);
                    meta.page_cnt = tail.page_cnt;
                    meta.total_size = tail.total_size;
                    meta.msg_id = tail.dz_ipc_msg_id;
                    break;
                }

                if (meta.page_cnt == 1)
                {
                    msg_ptr->deserialize(first_page);
                    return true;
                }

                std::vector<uint8_t> assembled(meta.total_size);
                std::vector<uint8_t> received(static_cast<std::size_t>(meta.page_cnt) + 1, 0);
                std::size_t received_cnt = 0;
                copy_page_to_buffer(first_page, meta, assembled, received, received_cnt);

                const uint64_t round_wait_ms = tm == ipc::invalid_value ? 50 : std::max<uint64_t>(5, tm);
                for (int round = 0; round < RTPS_MAX_NACK_ROUND && received_cnt < meta.page_cnt; ++round)
                {
                    const auto round_begin = std::chrono::steady_clock::now();
                    while (received_cnt < meta.page_cnt)
                    {
                        const uint64_t round_elapsed = elapsed_ms(round_begin);
                        if (round_elapsed >= round_wait_ms)
                        {
                            break;
                        }

                        ipc::buffer page = node->receive(round_wait_ms - round_elapsed);
                        if (page.empty())
                        {
                            break;
                        }
                        if (!msg_ptr->check_id(page))
                        {
                            continue;
                        }

                        copy_page_to_buffer(page, meta, assembled, received, received_cnt);
                    }

                    if (received_cnt >= meta.page_cnt)
                    {
                        break;
                    }

                    nack_msg.page_cnt = meta.page_cnt;
                    nack_msg.total_size = meta.total_size;
                    nack_msg.data_msg_id = meta.msg_id;
                    nack_msg.receiver_id = local_node_id();
                    nack_msg.missing_pages.clear();

                    for (uint16_t i = 1; i <= meta.page_cnt; ++i)
                    {
                        if (received[i] == 0)
                        {
                            nack_msg.missing_pages.push_back(i);
                            if (nack_msg.missing_pages.size() >= ipc_rtps_nack_msg::kMaxMissingPages)
                            {
                                break;
                            }
                        }
                    }

                    if (nack_msg.missing_pages.empty())
                    {
                        break;
                    }

                    ipc::buffer nack_buf = nack_msg.serialize();
                    int send_retry = 0;
                    while (!node->send(nack_buf) && send_retry < SEND_RETRY_MAX)
                    {
                        ++send_retry;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }

                if (received_cnt < meta.page_cnt)
                {
                    return false;
                }

                ipc::buffer total_buffer_cache(assembled.data(), meta.total_size);
                msg_ptr->deserialize(total_buffer_cache);
                return true;
            }
        }

        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        bool chunk_rev_topic(std::shared_ptr<ipc::socket::UDPNode> &node, std::shared_ptr<TopicData> &rev_msg, uint64_t tm)
        {
            std::shared_ptr<ipc_msg_base> msg_ptr = rev_msg->topic();
            return recv_chunk_common(node, msg_ptr, tm);
        }

        bool chunk_rev_server(std::shared_ptr<ipc::socket::UDPNode> &node, std::shared_ptr<ServiceData> &rev_msg, uint64_t tm, bool ser_or_cli)
        {
            std::shared_ptr<ipc_msg_base> msg_ptr = ser_or_cli ? rev_msg->request() : rev_msg->response();
            return recv_chunk_common(node, msg_ptr, tm);
        }

        bool chunk_send(std::shared_ptr<ipc::socket::UDPNode> &node, ipc::buffer &publish_data)
        {
            std::vector<ipc::buffer> chunks;
            chunks.reserve((publish_data.size() + UDP_MAX_SIZE - 1) / UDP_MAX_SIZE);
            for (std::size_t offset = 0; offset < publish_data.size(); offset += UDP_MAX_SIZE)
            {
                std::size_t chunk_size = std::min(static_cast<std::size_t>(UDP_MAX_SIZE), publish_data.size() - offset);
                chunks.emplace_back(static_cast<uint8_t *>(publish_data.data()) + offset, chunk_size);
            }

            for (auto &chunk : chunks)
            {
                int retry_count = 0;
                while (!node->send(chunk))
                {
                    ++retry_count;
                    if (retry_count >= SEND_RETRY_MAX)
                    {
                        return false;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            if (chunks.size() <= 1)
            {
                return true;
            }

            ipc_tail_msg first_tail;
            if (!parse_tail(chunks.front(), first_tail))
            {
                return true;
            }

            chunk_meta meta{first_tail.page_cnt, first_tail.total_size, first_tail.dz_ipc_msg_id};
            ipc_rtps_nack_msg nack_msg;
            const uint64_t nack_wait_ms = std::max<uint64_t>(6, std::min<uint64_t>(40, static_cast<uint64_t>(meta.page_cnt) * 2));

            for (int round = 0; round < RTPS_MAX_NACK_ROUND; ++round)
            {
                bool has_valid_nack = false;
                std::unordered_set<uint16_t> missing_union;
                const auto round_begin = std::chrono::steady_clock::now();

                while (elapsed_ms(round_begin) < nack_wait_ms)
                {
                    ipc::buffer recv_buf = node->receive(nack_wait_ms - elapsed_ms(round_begin));
                    if (recv_buf.empty())
                    {
                        break;
                    }
                    if (!nack_msg.check_id(recv_buf))
                    {
                        continue;
                    }

                    nack_msg.deserialize(recv_buf);
                    if (nack_msg.receiver_id == local_node_id())
                    {
                        continue;
                    }
                    if (nack_msg.page_cnt != meta.page_cnt ||
                        nack_msg.total_size != meta.total_size ||
                        nack_msg.data_msg_id != meta.msg_id)
                    {
                        continue;
                    }

                    has_valid_nack = true;
                    for (uint16_t page_id : nack_msg.missing_pages)
                    {
                        if (page_id > 0 && page_id <= chunks.size())
                        {
                            missing_union.insert(page_id);
                        }
                    }
                }

                if (!has_valid_nack || missing_union.empty())
                {
                    break;
                }

                for (uint16_t miss_page : missing_union)
                {
                    int retry_count = 0;
                    while (!node->send(chunks[miss_page - 1]))
                    {
                        ++retry_count;
                        if (retry_count >= SEND_RETRY_MAX)
                        {
                            return false;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
            }

            return true;
        }

#undef UDP_MAX_SIZE
    }
}