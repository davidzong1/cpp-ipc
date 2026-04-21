#include "dzIPC/socket_pub_sub_ipc.h"
#include "ipc_msg/ipc_msg_base/udp_id_init_msg.hpp"
#include "libipc/platform/detail.h"
#include "dzIPC/common/hash.h"
#include <memory>
#include <iostream>
#include <typeinfo>

#define UDP_DISCOVERY_BASE_PORT 11451
#define ListenerWaitTime 1000 // 1 second
namespace dzIPC
{
    namespace socket
    {
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        bool chunk_rev(std::shared_ptr<ipc::socket::UDPNode> &node, TopicDataPtr &rev_msg, uint64_t tm)
        {
#define UDP_MAX_SIZE 1472
            ipc::buffer cache_buffer = node->receive(tm);
            if (!cache_buffer.empty())
            {
                if (rev_msg->topic()->check_id(cache_buffer))
                {
                    const ipc_tail_msg data_vec = rev_msg->topic()->get_tail_msg(cache_buffer);
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
                            if (rev_msg->topic()->check_id(cache_buffer))
                            {
                                if (err_flag)
                                {
                                    continue; // 如果之前已经发生错误，继续接收剩余的分片但不进行处理
                                }
                                const ipc_tail_msg data_vec_i = rev_msg->topic()->get_tail_msg(cache_buffer);
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
                            rev_msg->topic()->deserialize(total_buffer_cache);
                            return true; // 返回true表示成功接收到完整数据
                        }
                    }
                    else
                    {
                        rev_msg->topic()->deserialize(cache_buffer);
                        return true; // 返回true表示成功接收到完整数据
                    }
                }
            }
            return false; // 返回false表示没有接收到有效数据
#undef UDP_MAX_SIZE
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        uint16_t udp_discovery_port_calculate(const std::string &topic_name, int domain_id)
        {
            uint64_t hash_value = static_cast<uint16_t>(dzIPC::common::fnv1a64(topic_name) % 10000);
            uint64_t limited_port = UDP_DISCOVERY_BASE_PORT + domain_id * hash_value;
            if (limited_port > 65535)
            {
                throw std::runtime_error("Calculated port number exceeds the maximum allowed value of 65535. Please choose a different topic name or domain ID.");
            }
            return static_cast<uint16_t>(limited_port); // 确保端口号在有效范围内
        }
        std::string udp_discovery_addr_calculate(const std::string &topic_name)
        {
            std::string ip_prefix = "239.255.";
            std::string ip_mid = topic_name + "_mid";
            std::string ip_end = topic_name + "_end";
            uint64_t mid_hash_value = dzIPC::common::fnv1a64(ip_mid) % 254;
            uint64_t end_hash_value = dzIPC::common::fnv1a64(ip_end) % 254;
            return ip_prefix + std::to_string(static_cast<uint64_t>(mid_hash_value)) + "." + std::to_string(static_cast<uint64_t>(end_hash_value));
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        socket_pub_ipc::socket_pub_ipc(const std::string &topic_name, size_t domain_id, bool verbose)
            : topic_name_(topic_name), domain_id_(domain_id), verbose_(verbose)
        {
            this->port_hash_ = udp_discovery_port_calculate(topic_name, domain_id);
            ipaddr_ = udp_discovery_addr_calculate(topic_name);
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        socket_pub_ipc::~socket_pub_ipc()
        {
            {
                std::lock_guard<std::mutex> lock(sleep_mtx);
                running.store(false, std::memory_order_release);
            }
            sleep_cv.notify_all(); // 立即唤醒正在 wait_for 的线程
            if (publish_thread_.joinable())
            {
                publish_thread_.join();
            }
            if (publisher_)
            {
                publisher_->close();
            }
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        void socket_pub_ipc::InitChannel()
        {
            try
            {
                publisher_ = std::make_shared<ipc::socket::UDPNode>(this->topic_name_.c_str(), this->ipaddr_.c_str(), this->port_hash_);
                std::cerr << "\033[32m[PublisherInfo] Publisher initialized on IP: " << this->ipaddr_ << " Port: " << this->port_hash_ << " for topic: " << topic_name_ << "\033[0m" << std::endl;
                while (!publisher_->connect())
                {
                    std::cerr << "\033[31m[PublisherInfo] Failed to connect publisher,reconnect affter 1 second...\033[0m" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                pool_reg_.rebind({dzIPC::info_pool::EntryKind::SocketPub,
                                  topic_name_,
                                  /*type_name=*/"",
                                  static_cast<int32_t>(domain_id_),
                                  "ip=" + ipaddr_ + ",port=" + std::to_string(port_hash_)});
                publish_thread_ = std::thread(&socket_pub_ipc::sub_listener, this);
                publish_thread_.detach();
            }
            catch (const std::exception &e)
            {
                std::cerr << "\033[31m[PublisherInfo] Error initializing channel: " << e.what() << "\033[0m"
                          << std::endl;
            }
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        void socket_pub_ipc::sub_listener()
        {
            ipc::buffer total_buffer_cache; // Used for storing and concatenating UDP messages that may be fragmented
            uint16_t sub_cnt = 0;
            uint32_t print_cnt = 0;
            std::shared_ptr<ipc::socket::UDPNode> sub_listener_node = std::make_shared<ipc::socket::UDPNode>(this->topic_name_.c_str(), this->ipaddr_.c_str(), this->port_hash_ + 1);
            while (!sub_listener_node->connect())
            {
                std::cerr << "\033[31mFailed to connect subscriber listener,reconnect affter 1 second...\033[0m" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            uint64_t tm = 100; // per second check subscriber count
            MsgPtr<ipc_pub_sub_id_init_msg> send_msg = std::make_shared<ipc_pub_sub_id_init_msg>();
            MsgPtr<ipc_pub_sub_id_init_msg> rev_msg = std::make_shared<ipc_pub_sub_id_init_msg>();
            send_msg->set_msg_id(domain_id_);
            rev_msg->set_msg_id(domain_id_);
            send_msg->host_flag = true;
            while (running.load(std::memory_order_acquire))
            {
                ipc::buffer msg_raw_data(std::move(send_msg->serialize()));
                sub_listener_node->send(msg_raw_data);
                int client_cnt_cache = 0;
                while (running.load(std::memory_order_acquire))
                {
                    ipc::buffer cache_buffer = sub_listener_node->receive(tm);
                    if (!cache_buffer.empty())
                    {
                        if (rev_msg->check_cli(cache_buffer) && rev_msg->check_id(cache_buffer))
                        {
                            client_cnt_cache++;
                        }
                    }
                    else // No more messages received within the timeout period, break to send a new discovery message
                    {
                        if (client_cnt_cache > 0) // has subscriber response
                        {
                            cli_cnt = client_cnt_cache;
                            subscribed_.store(true, std::memory_order_release);
                        }
                        else // no subscriber response
                        {
                            cli_cnt = 0;
                            subscribed_.store(false, std::memory_order_release);
                        }
                        break;
                    }
                }
                if ((verbose_) && cli_cnt > 0 && print_cnt > 1)
                {
                    std::cout << "\033[32m[PublisherInfo] Subscriber connected, total subscribers: " << cli_cnt << "\033[0m" << std::endl;
                    print_cnt = 0;
                }
                else
                {
                    print_cnt++;
                }
                std::unique_lock<std::mutex> lock(sleep_mtx);
                if (sleep_cv.wait_for(lock, std::chrono::milliseconds(ListenerWaitTime), [this]
                                      { return !running.load(std::memory_order_acquire); }))
                {
                    if (verbose_)
                    {
                        std::cerr << "\033[33m[PublisherInfo] Subscriber listener has notified to exit for topic: " << topic_name_
                                  << "\033[0m" << std::endl;
                    }
                    break;
                }
            }
            sub_listener_node->close();
            std::cerr << "\033[33m[PublisherInfo] Subscriber listener thread exited for topic: " << topic_name_ << "\033[0m" << std::endl;
        }

        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        void socket_pub_ipc::publish(MsgPtr<> msg)
        {
            try
            {
                if (subscribed_.load(std::memory_order_acquire))
                {
                    ipc::buffer response_data(std::move(msg->serialize()));
                    if (response_data.size() > 1472)
                    {
                        // UDP报文超过1472字节可能会被分片，但增加丢包风险
                        for (size_t offset = 0; offset < response_data.size(); offset += 1472)
                        {
                            size_t chunk_size = std::min(static_cast<size_t>(1472), response_data.size() - offset);
                            ipc::buffer chunk_buffer(static_cast<uint8_t *>(response_data.data()) + offset, chunk_size);
                            publisher_->send(chunk_buffer);
                        }
                    }
                    else
                        publisher_->send(response_data);
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "\033[31m[PublisherInfo] Error publishing message: " << e.what() << "\033[0m"
                          << std::endl;
            }
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        socket_sub_ipc::socket_sub_ipc(const TopicDataPtr &msg, const std::string &topic_name, size_t domain_id, const size_t queue_size, bool verbose)
            : topic_name_(topic_name), domain_id_(domain_id), verbose_(verbose)
        {
            topic_msg_.reset(msg->clone());
            this->port_hash_ = udp_discovery_port_calculate(topic_name, domain_id);
            ipaddr_ = udp_discovery_addr_calculate(topic_name);
            msg_queue_ = std::make_unique<CircularQueue<ipc_msg_base>>(queue_size);
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        socket_sub_ipc::~socket_sub_ipc()
        {
            {
                std::lock_guard<std::mutex> lock(sub_no_mem_sleep_mtx);
                running.store(false, std::memory_order_release);
            }
            sub_no_mem_sleep_cv.notify_all();
            if (subscribe_actor_thread_.joinable())
                subscribe_actor_thread_.join();
            if (subscribe_thread_.joinable())
                subscribe_thread_.join();
            if (subscriber_)
                subscriber_->close();
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        void socket_sub_ipc::reset_message(const TopicDataPtr &msg)
        {
            if (!msg)
            {
                return;
            }
            topic_msg_.reset(msg->clone());
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        void socket_sub_ipc::InitChannel()
        {
            try
            {
                subscriber_ = std::make_shared<ipc::socket::UDPNode>(this->topic_name_.c_str(), this->ipaddr_.c_str(), this->port_hash_);
                std::cerr << "\033[32m[SubscriberInfo] Subscriber initialized on IP: " << this->ipaddr_ << " Port: " << this->port_hash_ << " for topic: " << topic_name_ << "\033[0m" << std::endl;
                while (!subscriber_->connect())
                {
                    std::cerr << "\033[31m[SubscriberInfo] Failed to connect subscriber,reconnect affter 1 second...\033[0m" << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                pool_reg_.rebind({dzIPC::info_pool::EntryKind::SocketSub,
                                  topic_name_,
                                  topic_msg_ && topic_msg_->topic()
                                      ? dzIPC::info_pool::demangle(typeid(*topic_msg_->topic()).name())
                                      : std::string{},
                                  static_cast<int32_t>(domain_id_),
                                  "ip=" + ipaddr_ + ",port=" + std::to_string(port_hash_)});
                subscribe_actor_thread_ = std::thread(&socket_sub_ipc::sub_actor, this);
                subscribe_actor_thread_.detach();
                subscribe_thread_ = std::thread([this]()
                                                {
                    while (running.load(std::memory_order_acquire))
                    {
                        if (subscribed_.load(std::memory_order_acquire) && topic_msg_)
                        {
                            if (chunk_rev(subscriber_, topic_msg_, 50)) // rev timeput 50ms
                            {
                                MsgPtr<> ptr_cache;
                                topic_msg_->swap(ptr_cache);
                                msg_queue_->push(std::move(ptr_cache));
                            }
                            else
                            {
                                continue;
                            }
                        }
                        else
                        {
                            std::unique_lock<std::mutex> lock(sub_no_mem_sleep_mtx);
                            sub_no_mem_sleep_cv.wait_for(lock, std::chrono::milliseconds(ListenerWaitTime));
                        }
                    } }); // 占位线程，保持对象存活直到析构
            }
            catch (const std::exception &e)
            {
                std::cerr << "\033[31m[SubscriberInfo] Error initializing channel: " << e.what() << "\033[0m"
                          << std::endl;
            }
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        void socket_sub_ipc::sub_actor()
        {
            uint64_t tm = ListenerWaitTime * 2;
            std::shared_ptr<ipc::socket::UDPNode> sub_listener_node = std::make_shared<ipc::socket::UDPNode>(this->topic_name_.c_str(), this->ipaddr_.c_str(), this->port_hash_ + 1);
            while (!sub_listener_node->connect())
            {
                std::cerr << "\033[31m[SubscriberInfo] Failed to connect subscriber listener,reconnect affter 1 second...\033[0m" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            MsgPtr<ipc_pub_sub_id_init_msg> send_msg = std::make_shared<ipc_pub_sub_id_init_msg>();
            MsgPtr<ipc_pub_sub_id_init_msg> rev_msg = std::make_shared<ipc_pub_sub_id_init_msg>();
            send_msg->set_msg_id(domain_id_);
            rev_msg->set_msg_id(domain_id_);
            send_msg->cli_flag = true;
            while (running.load(std::memory_order_acquire))
            {
                ipc::buffer cache_buffer = sub_listener_node->receive(tm);
                if (!cache_buffer.empty())
                {
                    if (rev_msg->check_id(cache_buffer) && rev_msg->check_host(cache_buffer))
                    {
                        ipc::buffer reply_data(std::move(send_msg->serialize()));
                        sub_listener_node->send(reply_data);
                        subscribed_.store(true, std::memory_order_release);
                        sub_no_mem_sleep_cv.notify_all(); // 唤醒可能正在等待消息的线程
                        if (verbose_)
                        {
                            std::cerr << "\033[32m[SubscriberInfo] Subscriber connected to publisher for topic: " << topic_name_
                                      << "\033[0m" << std::endl;
                        }
                    }
                }
                else
                {
                    // No more messages received within the timeout period, continue to wait for new messages
                    if (verbose_)
                    {
                        std::cerr << "\033[33m[SubscriberInfo] No discovery message received, waiting for publisher for topic: " << topic_name_
                                  << "\033[0m" << std::endl;
                    }
                    subscribed_.store(false, std::memory_order_release);
                    continue;
                }
            }
            sub_listener_node->close();
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        void socket_sub_ipc::get(MsgPtr<> &msg)
        {
            msg_queue_->pop(msg);
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/
        bool socket_sub_ipc::try_get(MsgPtr<> &msg)
        {
            return msg_queue_->try_pop(msg);
        }
        /******************************************************************************************************/
        /******************************************************************************************************/
        /******************************************************************************************************/

    }
}