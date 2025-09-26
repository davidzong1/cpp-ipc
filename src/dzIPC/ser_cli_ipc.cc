#include "dzIPC/ser_cli_ipc.h"
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <memory>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include "semaphore.h"
namespace dzIPC {
using namespace ipc;
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/

ser_ipc::ser_ipc(const std::string& topic_name_, const ServiceDataPtr& msg,
                 SerCliCallback callback, bool verbose)
    : topic_name_(topic_name_),
      callback_(std::move(callback)),
      verbose_(verbose) {
  message_.reset(msg->clone());
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void ser_ipc::reset_message(const ServiceDataPtr& msg) {
  message_.reset(msg->clone());
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void ser_ipc::reset_callback(SerCliCallback callback) {
  callback_ = std::move(callback);
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/

ser_ipc::~ser_ipc() {
  running = false;
  if (response_thread_ != nullptr) {
    if (response_thread_->joinable()) {
      response_thread_->join();
    }
  }
  if (ipc_r_ptr_ && ipc_r_ptr_->valid()) {
    ipc_r_ptr_->clear();
  }
  if (ipc_w_ptr_ && ipc_w_ptr_->valid()) {
    ipc_w_ptr_->clear();
  }
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void ser_ipc::InitChannel() {
  std::string r_name, w_name;
  r_name = "dz_ipc_" + topic_name_ + "_ser_r";
  w_name = "dz_ipc_" + topic_name_ + "_ser_w";
  // 清理残留的共享内存
  ipc::server::clear_storage(r_name.c_str());
  ipc::server::clear_storage(w_name.c_str());
  // 发送清理广播，等待客户端清理完成
  send_cleanup_broadcast();
  ipc_r_ptr_ =
      std::make_shared<ipc::server>(r_name.c_str(), ipc::receiver, verbose_);
  ipc_w_ptr_ =
      std::make_shared<ipc::server>(w_name.c_str(), ipc::sender, verbose_);
  std::cerr << "\033[33m[" << topic_name_
            << "Info] Server channel created for topic: " << topic_name_
            << "\033[0m" << std::endl;
  response_thread_ = new std::thread(&ser_ipc::response_thread_func, this);
  response_thread_->detach();
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/

void ser_ipc::send_cleanup_broadcast() {
  std::string sem_ser_name = "dz_ipc_" + topic_name_ + "_ser_post";
  std::string sem_cli_name = "dz_ipc_" + topic_name_ + "_cli_post";
  sem_t* sem_ser = sem_open(sem_ser_name.c_str(), O_CREAT, 0666, 0);
  if (sem_ser == SEM_FAILED) {
    throw std::runtime_error("sem_open failed");
  }
  sem_t* sem_cli = sem_open(sem_cli_name.c_str(), O_CREAT, 0666, 0);
  if (sem_cli == SEM_FAILED) {
    sem_close(sem_ser);
    throw std::runtime_error("sem_open failed");
  }
  while (sem_trywait(sem_ser) == 0);  // 清空信号量
  sem_post(sem_ser);                  // 发送清理开始信号
  sem_wait(sem_cli);                  // 等待客户端确认清理完成
  sem_close(sem_ser);
  sem_close(sem_cli);
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void ser_ipc::reset_sync() {
  std::string sem_ser_name = "dz_ipc_" + topic_name_ + "_ser_reset_sync";
  std::string sem_cli_name = "dz_ipc_" + topic_name_ + "_cli_reset_sync";
  sem_t* sem_ser = sem_open(sem_ser_name.c_str(), O_CREAT, 0666, 0);
  if (sem_ser == SEM_FAILED) {
    throw std::runtime_error("sem_open failed");
  }
  sem_t* sem_cli = sem_open(sem_cli_name.c_str(), O_CREAT, 0666, 0);
  if (sem_cli == SEM_FAILED) {
    sem_close(sem_ser);
    throw std::runtime_error("sem_open failed");
  }
  std::cerr << "[UnitreeDDSNode] Server resetting sync." << std::endl;
  sem_post(sem_ser);  // 发送清理开始信号
  sem_wait(sem_cli);
  while (sem_trywait(sem_ser) == 0);  // 清空信号量
  while (sem_trywait(sem_cli) == 0);  // 清空信号量
  sem_close(sem_ser);
  sem_close(sem_cli);
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void ser_ipc::response_thread_func() {
  while (running) {
    /* 服务端等待请求 */
    buff_t raw_data_buff = ipc_r_ptr_->recv(10);
    if (raw_data_buff.empty()) {
      continue;
    }
    /* 原始数据映射 */
    std::vector<char> raw_data(
        static_cast<const char*>(raw_data_buff.data()),
        static_cast<const char*>(raw_data_buff.data()) + raw_data_buff.size());
    /* 反序列转换为msg数据 */
    message_->request()->deserialize(raw_data);
    callback_(message_);
    std::vector<char> response_data(
        std::move(message_->response()->serialize()));
    ipc_w_ptr_->send(response_data.data(), response_data.size());
  }  // 客户段发送请求
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/

cli_ipc::cli_ipc(const std::string& topic_name_, const ServiceDataPtr& msg,
                 bool verbose)
    : topic_name_(topic_name_), message_(msg), verbose_(verbose) {}

cli_ipc::~cli_ipc() {
  if (ipc_r_ptr_ && ipc_r_ptr_->valid()) {
    ipc_r_ptr_->release();
  }
  if (ipc_w_ptr_ && ipc_w_ptr_->valid()) {
    ipc_w_ptr_->release();
  }
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/

void cli_ipc::InitChannel() {
  // 等待服务端创建通道
  rev_cleanup_broadcast();
  std::cerr << "\033[33m[" << topic_name_
            << "Info] Client connected to server topic: " << topic_name_
            << "\033[0m" << std::endl;
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/

void cli_ipc::rev_cleanup_broadcast() {
  std::string sem_ser_name = "dz_ipc_" + topic_name_ + "_ser_post";
  std::string sem_cli_name = "dz_ipc_" + topic_name_ + "_cli_post";
  sem_t* sem_ser = sem_open(sem_ser_name.c_str(), O_CREAT, 0666, 0);
  if (sem_ser == SEM_FAILED) {
    throw std::runtime_error("sem_open failed");
  }
  sem_t* sem_cli = sem_open(sem_cli_name.c_str(), O_CREAT, 0666, 0);
  if (sem_cli == SEM_FAILED) {
    sem_close(sem_ser);
    throw std::runtime_error("sem_open failed");
  }
  while (sem_trywait(sem_cli) == 0);  // 清空信号量
  sem_wait(sem_ser);                  // 等待服务端发送清理开始信号
  std::cerr << "[UnitreeDDSNode] Client cleaning up old channels." << std::endl;
  // 创建新的通道
  std::string r_name, w_name;
  r_name = "dz_ipc_" + topic_name_ + "_ser_w";
  w_name = "dz_ipc_" + topic_name_ + "_ser_r";
  ipc_r_ptr_ =
      std::make_shared<ipc::server>(r_name.c_str(), ipc::receiver, verbose_);
  ipc_w_ptr_ =
      std::make_shared<ipc::server>(w_name.c_str(), ipc::sender, verbose_);
  sem_post(sem_cli);  // 通知服务端清理完成
  sem_close(sem_ser);
  sem_close(sem_cli);
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void cli_ipc::send_request(ServiceDataPtr& request) {
  std::vector<char> request_data(std::move(request->request()->serialize()));
  int retry_count = 0;
  while (ipc_w_ptr_->send(request_data.data(), request_data.size()) == false) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (retry_count >= 10) {
      return;
    }
    retry_count++;
  };
  buff_t raw_response_buff = ipc_r_ptr_->recv();
  if (raw_response_buff.empty()) {
    throw std::runtime_error("ipc_r_ptr_ recv error");
  }
  /* 原始数据映射 */
  std::vector<char> raw_response(
      static_cast<const char*>(raw_response_buff.data()),
      static_cast<const char*>(raw_response_buff.data()) +
          raw_response_buff.size());
  /* 反序列转换为msg数据 */
  request->response()->deserialize(raw_response);
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void cli_ipc::reset_message(const ServiceDataPtr& msg) {
  message_.reset(msg->clone());
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void cli_ipc::reset_sync() {
  std::string sem_ser_name = "dz_ipc_" + topic_name_ + "_ser_reset_sync";
  std::string sem_cli_name = "dz_ipc_" + topic_name_ + "_cli_reset_sync";
  sem_t* sem_ser = sem_open(sem_ser_name.c_str(), O_CREAT, 0666, 0);
  if (sem_ser == SEM_FAILED) {
    throw std::runtime_error("sem_open failed");
  }
  sem_t* sem_cli = sem_open(sem_cli_name.c_str(), O_CREAT, 0666, 0);
  if (sem_cli == SEM_FAILED) {
    sem_close(sem_ser);
    throw std::runtime_error("sem_open failed");
  }
  std::cerr << "[UnitreeDDSNode] Client resetting sync." << std::endl;
  sem_wait(sem_ser);  // 等待服务端发送清理开始信号
  sem_post(sem_cli);  // 通知服务端清理完成
  sem_close(sem_ser);
  sem_close(sem_cli);
}
}  // namespace dzIPC