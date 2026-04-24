#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "dzIPC/common/topic_data.h"
#include "dzIPC/common/circularqueue.h"
#include "dzIPC/ipc_info_pool.h"
#include "libipc/udp.h"
#include <thread>
#include <atomic>
#include <semaphore.h>
#include <type_traits>
#include <memory>
#include <condition_variable>
#include <mutex>
namespace dzIPC
{
    namespace socket
    {
        template <typename T = ipc_msg_base,
                  typename = std::enable_if_t<std::is_base_of<ipc_msg_base, T>::value>>
        using MsgPtr = std::shared_ptr<T>;
        class socket_pub_ipc;
        class socket_sub_ipc;
        using TopicDataPtr = std::shared_ptr<TopicData>;
        using PublishPtr = std::shared_ptr<socket_pub_ipc>;
        using SubscribePtr = std::shared_ptr<socket_sub_ipc>;
        class socket_pub_ipc
        {
        public:
            explicit socket_pub_ipc(const TopicDataPtr &msg, const std::string &topic_name, size_t domain_id, bool verbose = false);
            ~socket_pub_ipc();
            void reset_message(const TopicDataPtr &msg);
            void InitChannel(std::string extra_info = "");
            bool publish(MsgPtr<> msg);
            bool has_subscribed() const { return subscribed_; }
            bool client_subscribed() const { return cli_cnt; }
            /* 禁用拷贝 */
            socket_pub_ipc(const socket_pub_ipc &) = delete;
            socket_pub_ipc &operator=(const socket_pub_ipc &) = delete;

        private:
            //     void sub_listener();

        private:
            int cli_cnt{0};
            std::atomic<bool> subscribed_{false};
            bool verbose_{false};
            std::atomic<bool> running{true};
            std::string topic_name_;
            std::shared_ptr<ipc::socket::UDPNode> publisher_;
            uint16_t port_hash_;
            std::string ipaddr_;
            size_t domain_id_;
            std::mutex sleep_mtx;
            std::condition_variable sleep_cv;
            dzIPC::info_pool::ScopedRegistration pool_reg_;
            TopicDataPtr topic_msg_;
        };

        class socket_sub_ipc
        {
        public:
            explicit socket_sub_ipc(const TopicDataPtr &msg, const std::string &topic_name, size_t domain_id, const size_t queue_size, bool verbose = false);
            ~socket_sub_ipc();
            void InitChannel(std::string extra_info = "");
            void reset_message(const TopicDataPtr &msg);
            void get(MsgPtr<> &msg);
            bool try_get(MsgPtr<> &msg);
            /* 禁用拷贝 */
            socket_sub_ipc(const socket_sub_ipc &) = delete;
            socket_sub_ipc &operator=(const socket_sub_ipc &) = delete;

        private:
            std::atomic<bool> subscribed_{false};
            std::atomic<bool> running{true};
            bool data_update_{false};
            bool verbose_{false};
            std::string topic_name_;
            uint16_t port_hash_;
            std::string ipaddr_;
            size_t domain_id_;
            std::shared_ptr<ipc::socket::UDPNode> subscriber_;
            TopicDataPtr topic_msg_;
            std::unique_ptr<CircularQueue<ipc_msg_base>> msg_queue_;
            std::thread *subscribe_thread_;
            dzIPC::info_pool::ScopedRegistration pool_reg_;
        };
    }
} // namespace dzIPC