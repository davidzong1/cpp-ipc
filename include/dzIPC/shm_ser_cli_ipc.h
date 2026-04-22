#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "dzIPC/common/srv_data.h"
#include "dzIPC/define.h"
#include "dzIPC/ipc_info_pool.h"
#include "libipc/ipc.h"
#include <thread>
#include <atomic>
namespace dzIPC
{
  namespace shm
  {
    class shm_ser_ipc;
    class shm_cli_ipc;

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
    using MsgPtr = std::shared_ptr<ipc_msg_base>;
    using dzIpcSerPtr = std::shared_ptr<dzIPC::shm::shm_ser_ipc>;
    using dzIpcCliPtr = std::shared_ptr<dzIPC::shm::shm_cli_ipc>;
    using SerCliCallback = std::function<void(ServiceDataPtr &)>;

    class shm_ser_ipc
    {
    public:
      explicit shm_ser_ipc(const std::string &topic_name_, const ServiceDataPtr &msg,
                           SerCliCallback callback, bool verbose = false);
      ~shm_ser_ipc();
      void reset_message(const ServiceDataPtr &msg);
      void reset_callback(SerCliCallback callback);
      void InitChannel(std::string extra_info = "");

      /* 禁用拷贝 */
      shm_ser_ipc(const shm_ser_ipc &) = delete;
      shm_ser_ipc &operator=(const shm_ser_ipc &) = delete;

    protected:
      void response_thread_func();
      void ser_handshake();

    private:
      std::atomic<bool> running{true};
      std::atomic<bool> handshake_completed{false};
      bool verbose_{true};
      SerCliCallback callback_;
      std::string topic_name_;
      ServiceDataPtr message_;
      std::thread *response_thread_{nullptr};
      std::thread *handshake_thread_{nullptr};
      std::shared_ptr<ipc::server> ipc_r_ptr_;
      std::shared_ptr<ipc::server> ipc_w_ptr_;
      std::vector<char> buf_;
      std::vector<char> response_buf_;
      dzIPC::info_pool::ScopedRegistration pool_reg_;
    };

    class shm_cli_ipc
    {
    public:
      explicit shm_cli_ipc(const std::string &topic_name_, const ServiceDataPtr &msg,
                           bool verbose = false);
      ~shm_cli_ipc();
      void InitChannel(std::string extra_info = "");
      void reset_message(const ServiceDataPtr &msg);
      bool send_request(ServiceDataPtr &request, uint64_t rev_tm = std::numeric_limits<uint32_t>::max());
      /* 禁用拷贝 */
      shm_cli_ipc(const shm_cli_ipc &) = delete;
      shm_cli_ipc &operator=(const shm_cli_ipc &) = delete;

    protected:
      void cli_handshake();

    private:
      std::atomic<bool> running{true};
      std::atomic<bool> handshake_completed{false};
      bool verbose_{true};
      ServiceDataPtr message_;
      std::string topic_name_;
      std::shared_ptr<ipc::server> ipc_r_ptr_;
      std::shared_ptr<ipc::server> ipc_w_ptr_;
      std::vector<char> buf_;
      std::vector<char> response_buf_;
      std::thread *handshake_thread_{nullptr};
      dzIPC::info_pool::ScopedRegistration pool_reg_;
    };
  }
} // namespace dzIPC