#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "dzIPC/common/srv_data.h"
#include "libipc/ipc.h"
#include <thread>
namespace dzIPC {
#define IPC_SERVER_SELECT true
#define IPC_CLIENT_SELECT false
class ser_ipc;
class cli_ipc;

/**
 * @brief 派生类智能指针转换
 * @tparam T 目标派生类类型
 * @tparam U 源类型
 */
template <typename T, typename U>
auto msgcast(const std::shared_ptr<U>& ptr) {
  return std::static_pointer_cast<T>(ptr);
}
using ServiceDataPtr = std::shared_ptr<ServiceData>;
using MsgPtr = std::shared_ptr<ipc_msg_base>;
using dzIpcSerPtr = std::shared_ptr<dzIPC::ser_ipc>;
using dzIpcCliPtr = std::shared_ptr<dzIPC::cli_ipc>;
using SerCliCallback = std::function<void(ServiceDataPtr&)>;

class ser_ipc {
 public:
  explicit ser_ipc(const std::string& topic_name_, const ServiceDataPtr& msg,
                   SerCliCallback callback, bool verbose = true);
  ~ser_ipc();
  void reset_message(const ServiceDataPtr& msg);
  void reset_callback(SerCliCallback callback);
  void InitChannel();
  void reset_sync();
  /* 禁用拷贝 */
  ser_ipc(const ser_ipc&) = delete;
  ser_ipc& operator=(const ser_ipc&) = delete;

 protected:
  void response_thread_func();
  void send_cleanup_broadcast();

 private:
  bool running{true};
  bool verbose_{true};
  SerCliCallback callback_;
  std::string topic_name_;
  ServiceDataPtr message_;
  std::thread* response_thread_ = nullptr;
  std::shared_ptr<ipc::server> ipc_r_ptr_;
  std::shared_ptr<ipc::server> ipc_w_ptr_;
  std::vector<char> buf_;
  std::vector<char> response_buf_;
};

class cli_ipc {
 public:
  explicit cli_ipc(const std::string& topic_name_, const ServiceDataPtr& msg,
                   bool verbose = true);
  ~cli_ipc();
  void InitChannel();
  void reset_message(const ServiceDataPtr& msg);
  void send_request(ServiceDataPtr& request);
  void reset_sync();
  /* 禁用拷贝 */
  cli_ipc(const cli_ipc&) = delete;
  cli_ipc& operator=(const cli_ipc&) = delete;

 protected:
  void rev_cleanup_broadcast();

 private:
  bool verbose_{true};
  ServiceDataPtr message_;
  std::string topic_name_;
  std::shared_ptr<ipc::server> ipc_r_ptr_;
  std::shared_ptr<ipc::server> ipc_w_ptr_;
  std::vector<char> buf_;
  std::vector<char> response_buf_;
};

}  // namespace dzIPC