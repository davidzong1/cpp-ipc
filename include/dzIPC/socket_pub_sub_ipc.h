// #pragma once
// #include <string>
// #include <vector>
// #include <memory>
// #include <functional>
// #include "dzIPC/common/topic_data.h"
// #include "dzIPC/common/circularqueue.h"
// #include "libipc/udp.h"
// #include <thread>
// #include <atomic>
// #include <semaphore.h>
// namespace dzIPC
// {
//     class socket_pub_ipc;
//     class socket_sub_ipc;
//     using TopicDataPtr = std::shared_ptr<TopicData>;
//     using MsgPtr = std::shared_ptr<ipc_msg_base>;
//     using PublishPtr = std::shared_ptr<socket_pub_ipc>;
//     using SubscribePtr = std::shared_ptr<socket_sub_ipc>;
//     class socket_pub_ipc
//     {
//     public:
//         explicit socket_pub_ipc(const std::string &topic_name, const std::string &ipaddr, uint64_t domain_id, bool verbose = false);
//         ~socket_pub_ipc();
//         void InitChannel();
//         void publish(MsgPtr msg);
//         bool has_subscribed() const { return subscribed_; }
//         /* 禁用拷贝 */
//         socket_pub_ipc(const socket_pub_ipc &) = delete;
//         socket_pub_ipc &operator=(const socket_pub_ipc &) = delete;

//     private:
//         void sub_listener();

//     private:
//         std::atomic<bool> subscribed_{false};
//         bool verbose_{false};
//         bool running{true};
//         std::string topic_name_;
//         std::shared_ptr<ipc::socket::UDPNode> publisher_;
//         std::thread publish_thread_;
//         uint16_t port_hash_;
//         std::string ipaddr_;
//         size_t domain_id_;
//     };

//     class socket_sub_ipc
//     {
//     public:
//         explicit socket_sub_ipc(const std::string &topic_name, const TopicDataPtr &msg,
//                                 const size_t queue_size, bool verbose = false);
//         ~socket_sub_ipc();
//         void InitChannel();
//         void reset_message(const TopicDataPtr &msg);
//         void get(MsgPtr &msg);
//         bool try_get(MsgPtr &msg);
//         /* 禁用拷贝 */
//         socket_sub_ipc(const socket_sub_ipc &) = delete;
//         socket_sub_ipc &operator=(const socket_sub_ipc &) = delete;

//     private:
//         bool running{true};
//         bool data_update_{false};
//         bool verbose_{false};
//         std::atomic<bool> sub_paused_{false}; // 线程暂停，用于切换msg
//         std::string topic_name_;
//         std::shared_ptr<ipc::socket::UDPNode> subscriber_;
//         TopicDataPtr topic_msg_;
//         std::unique_ptr<CircularQueue<ipc_msg_base>> msg_queue_;
//         std::thread subscribe_thread_;
//         sem_t empty_queue_;
//     };
// } // namespace dzIPC