// #include "dzIPC/socket_pub_sub_ipc.h"
// #include "ipc_msg/ipc_msg_base/udp_id_init_msg.hpp"
// #include "libipc/platform/detail.h"
// #include "dzIPC/common/hash.h"
// #include <memory>
// #include <iostream>
// #define UDP_DISCOVERY_BASE_PORT 11451

// namespace dzIPC
// {
//     uint16_t udp_discovery_port_calculate(const std::string &topic_name, int domain_id)
//     {
//         uint64_t hash_value = static_cast<uint16_t>(dzIPC::common::fnv1a64(topic_name) % 10000);
//         uint64_t limited_port = UDP_DISCOVERY_BASE_PORT + domain_id * hash_value;
//         if (limited_port > 65535)
//         {
//             throw std::runtime_error("Calculated port number exceeds the maximum allowed value of 65535. Please choose a different topic name or domain ID.");
//         }
//         return static_cast<uint16_t>(limited_port); // 确保端口号在有效范围内
//     }

//     socket_pub_ipc::socket_pub_ipc(const std::string &topic_name, const std::string &ipaddr, size_t domain_id, bool verbose = false)
//         : topic_name_(topic_name), ipaddr_(ipaddr), domain_id_(domain_id), verbose_(verbose)
//     {
//         this->port_hash_ = udp_discovery_port_calculate(topic_name, domain_id);
//     }
//     /******************************************************************************************************/
//     /******************************************************************************************************/
//     /******************************************************************************************************/
//     socket_pub_ipc::~socket_pub_ipc()
//     {
//         running = false;
//         if (publish_thread_.joinable())
//         {
//             publish_thread_.join();
//         }
//         if (publisher_)
//         {
//             publisher_->close();
//         }
//     }
//     /******************************************************************************************************/
//     /******************************************************************************************************/
//     /******************************************************************************************************/
//     void socket_pub_ipc::InitChannel()
//     {
//         try
//         {
//             publisher_ = std::make_shared<ipc::socket::UDPNode>(this->topic_name_, this->ipaddr_, this->port_hash_);
//             while (!publisher_->connect())
//             {
//                 std::cerr << "\033[31mFailed to connect publisher,reconnect affter 1 second...\033[0m" << std::endl;
//                 std::this_thread::sleep_for(std::chrono::seconds(1));
//             }
//             publish_thread_ = std::thread(&socket_pub_ipc::sub_listener, this);
//         }
//         catch (const std::exception &e)
//         {
//             std::cerr << "\033[31mError initializing channel: " << e.what() << "\033[0m"
//                       << std::endl;
//         }
//     }
//     /******************************************************************************************************/
//     /******************************************************************************************************/
//     /******************************************************************************************************/
//     void socket_pub_ipc::sub_listener()
//     {
//         uint16_t sub_cnt = 0;
//         std::shared_ptr<ipc::socket::UDPNode> sub_listener_node = std::make_shared<ipc::socket::UDPNode>(this->topic_name_, this->ipaddr_, this->port_hash_ + 1);
//         while (!sub_listener_node->connect())
//         {
//             std::cerr << "\033[31mFailed to connect subscriber listener,reconnect affter 1 second...\033[0m" << std::endl;
//             std::this_thread::sleep_for(std::chrono::seconds(1));
//         }
//         uint64_t tm = 100; // per second check subscriber count
//         MsgPtr send_msg = std::make_shared<ipc_pub_sub_id_init_msg>(domain_id_);
//         MsgPtr rev_msg = std::make_shared<ipc_pub_sub_id_init_msg>(domain_id_);
//         send_msg->set_msg_id(domain_id_);
//         rev_msg->set_msg_id(domain_id_);
//         send_msg->host_flag = true;
//         while (running)
//         {
//             std::vector<char> msg_raw_data(std::move(send_msg->serialize()));
//             sub_listener_node->send(msg_raw_data);
//             if (sub_listener_node->receive(buffer, tm))
//             {
//                 if (send_msg->check_id(msg_raw_data))
//                 {
//                     rev_msg->deserialize(msg_raw_data);
//                     if (!rev_msg->host_flag)
//                     {
//                         sub_cnt++;
//                         if ((verbose_))
//                         {
//                             std::cout << "\033[32mSubscriber connected, total subscribers: " << sub_cnt << "\033[0m" << std::endl;
//                         }
//                     }
//                 }
//             }
//         }
//     }
//     /******************************************************************************************************/
//     /******************************************************************************************************/
//     /******************************************************************************************************/
//     void socket_pub_ipc::publish(MsgPtr msg)
//     {
//         try
//         {
//             if (subscribed_.load(std::memory_order_acquire))
//             {
//                 std::vector<char> response_data(std::move(msg->serialize()));
//                 if (response_data.size() > 1472)
//                 {
//                     // UDP报文超过1472字节可能会被分片，但增加丢包风险
//                                 }
//                 else
//                     publisher_->send(response_data);
//             }
//         }
//         catch (const std::exception &e)
//         {
//             std::cerr << "\033[31mError publishing message: " << e.what() << "\033[0m"
//                       << std::endl;
//         }
//     }
// }