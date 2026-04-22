#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "dzIPC/common/srv_data.h"
#include "dzIPC/define.h"
#include "libipc/udp.h"
#include <thread>
#include <atomic>
#include "dzIPC/ipc_info_pool.h"
namespace dzIPC
{
    namespace socket
    {
        class socket_ser_ipc;
        class socket_cli_ipc;

        /**
         * @brief 派生类智能指针转换
         * @tparam T 目标派生类类型
         * @tparam U 源类型
         */
        template <typename T, typename U>
        auto msgcast(const std::shared_ptr<U> &ptr)
        {
            return std::static_pointer_cast<T>(ptr);
        }
        using ServiceDataPtr = std::shared_ptr<ServiceData>;
        template <typename T = ipc_msg_base,
                  typename = std::enable_if_t<std::is_base_of<ipc_msg_base, T>::value>>
        using MsgPtr = std::shared_ptr<T>;
        using dzIpcSerPtr = std::shared_ptr<dzIPC::socket::socket_ser_ipc>;
        using dzIpcCliPtr = std::shared_ptr<dzIPC::socket::socket_cli_ipc>;
        using SerCliCallback = std::function<void(ServiceDataPtr &)>;

        class socket_ser_ipc
        {
        public:
            explicit socket_ser_ipc(const std::string &topic_name_, const ServiceDataPtr &msg,
                                    SerCliCallback callback, size_t domain_id, bool verbose = false);
            ~socket_ser_ipc();
            void reset_message(const ServiceDataPtr &msg);
            void reset_callback(SerCliCallback callback);
            void InitChannel(std::string extra_info = "");
            /* 禁用拷贝 */
            socket_ser_ipc(const socket_ser_ipc &) = delete;
            socket_ser_ipc &operator=(const socket_ser_ipc &) = delete;

        protected:
            void response_thread_func();
            void server_handshake();

        private:
            std::atomic<bool> running{true};
            std::atomic<bool> handshake_completed{false};
            size_t domain_id_;
            uint64_t port_hash_;
            std::string ipaddr_;
            bool verbose_{true};
            SerCliCallback callback_;
            std::string topic_name_;
            ServiceDataPtr message_;
            std::thread *response_thread_{nullptr};
            std::thread *handshake_thread_{nullptr};
            std::shared_ptr<ipc::socket::UDPNode> ipc_r_ptr_;
            std::shared_ptr<ipc::socket::UDPNode> ipc_w_ptr_;
            std::vector<char> buf_;
            std::vector<char> response_buf_;
            dzIPC::info_pool::ScopedRegistration pool_reg_;
        };

        class socket_cli_ipc
        {
        public:
            explicit socket_cli_ipc(const std::string &topic_name_, const ServiceDataPtr &msg, size_t domain_id,
                                    bool verbose = false);
            ~socket_cli_ipc();
            void InitChannel(std::string extra_info = "");
            void reset_message(const ServiceDataPtr &msg);
            bool send_request(ServiceDataPtr &request);
            /* 禁用拷贝 */
            socket_cli_ipc(const socket_cli_ipc &) = delete;
            socket_cli_ipc &operator=(const socket_cli_ipc &) = delete;

        protected:
            void client_handshake();

        private:
            std::atomic<bool> running{true};
            std::atomic<bool> handshake_completed{false};
            size_t domain_id_;
            uint64_t port_hash_;
            std::string ipaddr_;
            bool verbose_{true};
            ServiceDataPtr message_;
            std::string topic_name_;
            std::shared_ptr<ipc::socket::UDPNode> ipc_r_ptr_;
            std::shared_ptr<ipc::socket::UDPNode> ipc_w_ptr_;
            std::vector<char> buf_;
            std::vector<char> response_buf_;
            std::thread *handshake_thread_{nullptr};
            dzIPC::info_pool::ScopedRegistration pool_reg_;
        };
    }
} // namespace dzIPC