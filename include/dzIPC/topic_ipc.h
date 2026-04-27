#pragma once
#include "dzIPC/shm_pub_sub_ipc.h"
#include "dzIPC/socket_pub_sub_ipc.h"
#include "dzIPC/type.h"
namespace dzIPC
{
    namespace pimpl
    {
        class publisher_ipc_impl
        {
        public:
            explicit publisher_ipc_impl(const std::shared_ptr<TopicData> &msg, const std::string &topic_name, size_t domain_id, IPCType ipc_type, bool verbose = false);
            ~publisher_ipc_impl() = default;
            void InitChannel(std::string extra_info = "");
            void reset_message(const std::shared_ptr<TopicData> &msg);
            bool publish(std::shared_ptr<ipc_msg_base> msg);
            bool has_subscribed() const;

        private:
            class publisher_ipc_impl_;
            publisher_ipc_impl_ *p_;
        };

        class subscriber_ipc_impl
        {
        public:
            explicit subscriber_ipc_impl(const std::shared_ptr<TopicData> &msg, const std::string &topic_name, size_t domain_id, const size_t queue_size, IPCType ipc_type, bool verbose = false);
            ~subscriber_ipc_impl() = default;
            void InitChannel(std::string extra_info = "");
            void reset_message(const std::shared_ptr<TopicData> &msg);
            void get(std::shared_ptr<TopicData> &msg);
            bool try_get(std::shared_ptr<TopicData> &msg);

        private:
            class subscriber_ipc_impl_;
            subscriber_ipc_impl_ *p_;
        };
    }
}