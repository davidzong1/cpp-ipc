#include "dzIPC/common/data_rev.h"
#include <thread>
namespace dzIPC
{
    namespace socket
    {
#define UDP_MAX_SIZE 1472
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        bool chunk_rev_topic(std::shared_ptr<ipc::socket::UDPNode> &node, std::shared_ptr<TopicData> &rev_msg, uint64_t tm)
        {
            ipc::buffer cache_buffer = node->receive(tm);
            std::shared_ptr<ipc_msg_base> msg_ptr = rev_msg->topic();
            if (!cache_buffer.empty())
            {
                if (msg_ptr->check_id(cache_buffer))
                {
                    const ipc_tail_msg data_vec = msg_ptr->get_tail_msg(cache_buffer);
                    if (data_vec.page_cnt > 1)
                    {
                        bool err_flag = false;
                        uint8_t data_cache[data_vec.total_size];
                        auto total_buffer_cache = ipc::buffer(data_cache, data_vec.total_size);
                        memcpy(static_cast<uint8_t *>(total_buffer_cache.data()) + (data_vec.now_page - 1) * UDP_MAX_SIZE, cache_buffer.data(), cache_buffer.size());
                        for (int i = 0; i < data_vec.page_cnt; i++)
                        {
                            ipc::buffer cache_buffer_i = node->receive(tm);
                            if (cache_buffer_i.empty())
                            {
                                return false; // 返回false表示没有接收到完整数据
                            }
                            if (msg_ptr->check_id(cache_buffer))
                            {
                                if (err_flag)
                                {
                                    continue; // 如果之前已经发生错误，继续接收剩余的分片但不进行处理
                                }
                                const ipc_tail_msg data_vec_i = msg_ptr->get_tail_msg(cache_buffer);
                                if (data_vec_i.page_cnt != data_vec.page_cnt || data_vec_i.total_size != data_vec.total_size || data_vec_i.now_page != (i + 1))
                                {
                                    err_flag = true;
                                    continue;
                                }
                                else
                                    memcpy(static_cast<uint8_t *>(total_buffer_cache.data()) + (data_vec_i.now_page - 1) * UDP_MAX_SIZE, cache_buffer_i.data(), cache_buffer_i.size());
                            }
                        }
                        if (err_flag)
                        {
                            return false; // 返回false表示没有接收到完整数据
                        }
                        else
                        {
                            msg_ptr->deserialize(total_buffer_cache);
                            return true; // 返回true表示成功接收到完整数据
                        }
                    }
                    else
                    {
                        msg_ptr->deserialize(cache_buffer);
                        return true; // 返回true表示成功接收到完整数据
                    }
                }
            }
            return false; // 返回false表示没有接收到有效数据
        }

        bool chunk_rev_server(std::shared_ptr<ipc::socket::UDPNode> &node, std::shared_ptr<ServiceData> &rev_msg, uint64_t tm, bool ser_or_cli)
        {
            ipc::buffer cache_buffer = node->receive(tm);
            std::shared_ptr<ipc_msg_base> msg_ptr = ser_or_cli ? rev_msg->request() : rev_msg->response();
            if (!cache_buffer.empty())
            {
                if (msg_ptr->check_id(cache_buffer))
                {
                    const ipc_tail_msg data_vec = msg_ptr->get_tail_msg(cache_buffer);
                    if (data_vec.page_cnt > 1)
                    {
                        bool err_flag = false;
                        uint8_t data_cache[data_vec.total_size];
                        auto total_buffer_cache = ipc::buffer(data_cache, data_vec.total_size);
                        memcpy(static_cast<uint8_t *>(total_buffer_cache.data()) + (data_vec.now_page - 1) * UDP_MAX_SIZE, cache_buffer.data(), cache_buffer.size());
                        for (int i = 0; i < data_vec.page_cnt; i++)
                        {
                            ipc::buffer cache_buffer_i = node->receive(tm);
                            if (cache_buffer_i.empty())
                            {
                                return false; // 返回false表示没有接收到完整数据
                            }
                            if (msg_ptr->check_id(cache_buffer))
                            {
                                if (err_flag)
                                {
                                    continue; // 如果之前已经发生错误，继续接收剩余的分片但不进行处理
                                }
                                const ipc_tail_msg data_vec_i = msg_ptr->get_tail_msg(cache_buffer);
                                if (data_vec_i.page_cnt != data_vec.page_cnt || data_vec_i.total_size != data_vec.total_size || data_vec_i.now_page != (i + 1))
                                {
                                    err_flag = true;
                                    continue;
                                }
                                else
                                    memcpy(static_cast<uint8_t *>(total_buffer_cache.data()) + (data_vec_i.now_page - 1) * UDP_MAX_SIZE, cache_buffer_i.data(), cache_buffer_i.size());
                            }
                        }
                        if (err_flag)
                        {
                            return false; // 返回false表示没有接收到完整数据
                        }
                        else
                        {
                            msg_ptr->deserialize(total_buffer_cache);
                            return true; // 返回true表示成功接收到完整数据
                        }
                    }
                    else
                    {
                        msg_ptr->deserialize(cache_buffer);
                        return true; // 返回true表示成功接收到完整数据
                    }
                }
            }
            return false; // 返回false表示没有接收到有效数据
        }

        bool chunk_send(std::shared_ptr<ipc::socket::UDPNode> &node, ipc::buffer &publish_data)
        {
            int retry_count = 0;
            if (publish_data.size() > UDP_MAX_SIZE)
            {
                // UDP报文超过1472字节可能会被分片，但增加丢包风险
                for (size_t offset = 0; offset < publish_data.size(); offset += UDP_MAX_SIZE)
                {
                    size_t chunk_size = std::min(static_cast<size_t>(UDP_MAX_SIZE), publish_data.size() - offset);
                    ipc::buffer chunk_buffer(static_cast<uint8_t *>(publish_data.data()) + offset, chunk_size);
                    while (!node->send(chunk_buffer))
                    {
                        retry_count++;
                        if (retry_count >= 10)
                        {
                            return false; // 发送失败，返回
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
            }
            else
            {
                while (!node->send(publish_data))
                {
                    retry_count++;
                    if (retry_count >= 10)
                    {
                        return false; // 发送失败，返回
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            return true; // 发送成功，返回
        }

#undef UDP_MAX_SIZE
    }
}