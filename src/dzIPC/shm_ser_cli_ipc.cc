#include "dzIPC/shm_ser_cli_ipc.h"
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <memory>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include <typeinfo>
#include "libipc/semaphore.h"
namespace dzIPC
{
  namespace shm
  {
    using namespace ipc;
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/

    shm_ser_ipc::shm_ser_ipc(const std::string &topic_name_, const ServiceDataPtr &msg,
                             SerCliCallback callback, bool verbose)
        : topic_name_(topic_name_),
          callback_(std::move(callback)),
          verbose_(verbose)
    {
      message_.reset(msg->clone());
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_ser_ipc::reset_message(const ServiceDataPtr &msg)
    {
      message_.reset(msg->clone());
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_ser_ipc::reset_callback(SerCliCallback callback)
    {
      callback_ = std::move(callback);
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/

    shm_ser_ipc::~shm_ser_ipc()
    {
      running = false;
      if (response_thread_ != nullptr)
      {
        if (response_thread_->joinable())
        {
          response_thread_->join();
        }
      }
      if (ipc_r_ptr_ && ipc_r_ptr_->valid())
      {
        ipc_r_ptr_->clear();
      }
      if (ipc_w_ptr_ && ipc_w_ptr_->valid())
      {
        ipc_w_ptr_->clear();
      }
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_ser_ipc::InitChannel()
    {
      std::string r_name, w_name;
      r_name = "dz_ipc_" + topic_name_ + "_ser_r";
      w_name = "dz_ipc_" + topic_name_ + "_ser_w";
      // 清理残留的共享内存
      ipc::server::clear_storage(r_name.c_str());
      ipc::server::clear_storage(w_name.c_str());
      // 发送清理广播，等待客户端清理完成
      ipc_r_ptr_ =
          std::make_shared<ipc::server>(r_name.c_str(), ipc::receiver, verbose_);
      ipc_w_ptr_ =
          std::make_shared<ipc::server>(w_name.c_str(), ipc::sender, verbose_);
      send_cleanup_broadcast();
      pool_reg_.rebind({dzIPC::info_pool::EntryKind::ShmServer,
                        topic_name_,
                        message_ ? dzIPC::info_pool::demangle(typeid(*message_).name()) : std::string{},
                        /*domain_id=*/0,
                        "r=" + r_name + ",w=" + w_name});
      std::cerr << "\033[33m[" << topic_name_ << "_Info] Server channel created for topic: " << topic_name_
                << "\033[0m" << std::endl;
      response_thread_ = new std::thread(&shm_ser_ipc::response_thread_func, this);
      response_thread_->detach();
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/

    void shm_ser_ipc::send_cleanup_broadcast()
    {
      std::string sem_ser_name = "dz_ipc_" + topic_name_ + "_ser_post";
      std::string sem_cli_name = "dz_ipc_" + topic_name_ + "_cli_post";
      ipc::sync::semaphore sem_ser;
      if (!sem_ser.open(sem_ser_name.c_str(), 0))
      {
        throw std::runtime_error("sem_open failed");
      }
      ipc::sync::semaphore sem_cli;
      if (!sem_cli.open(sem_cli_name.c_str(), 0))
      {
        sem_ser.close();
        throw std::runtime_error("sem_open failed");
      }
      do
      {
        while (sem_ser.try_wait())
          ;                         // 清空信号量
        sem_ser.post();             // 发送清理开始信号
      } while (!sem_cli.wait(100)); // 每隔100ms等待客户端确认清理完成
      while (sem_ser.try_wait())
        ; // 清空信号量
      while (sem_cli.try_wait())
        ; // 清空信号量
      sem_ser.close();
      sem_cli.close();
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_ser_ipc::reset_sync()
    {
      std::string sem_ser_name = "dz_ipc_" + topic_name_ + "_ser_reset_sync";
      std::string sem_cli_name = "dz_ipc_" + topic_name_ + "_cli_reset_sync";
      ipc::sync::semaphore sem_ser;
      if (!sem_ser.open(sem_ser_name.c_str(), 0))
      {
        throw std::runtime_error("sem_open failed");
      }
      ipc::sync::semaphore sem_cli;
      if (!sem_cli.open(sem_cli_name.c_str(), 0))
      {
        sem_ser.close();
        throw std::runtime_error("sem_open failed");
      }
      if (verbose_)
        std::cerr << "[" << topic_name_ << "_Info] Server resetting sync." << std::endl;
      sem_ser.post(); // 发送清理开始信号
      sem_cli.wait();
      while (sem_ser.try_wait())
        ; // 清空信号量
      while (sem_cli.try_wait())
        ; // 清空信号量
      sem_ser.close();
      sem_cli.close();
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_ser_ipc::response_thread_func()
    {
      while (running)
      {
        /* 服务端等待请求 */
        ipc::buffer raw_data = ipc_r_ptr_->recv(50);
        if (raw_data.empty())
        {
          continue;
        }
        /* 身份判断 */
        if (!message_->check_msg_id(raw_data))
        {
          if (verbose_)
          {
            std::cerr << "\033[31m[Warning] Received message with invalid ID on topic: "
                      << topic_name_ << "\033[0m" << std::endl;
          }
          continue;
        }
        /* 反序列转换为msg数据 */
        message_->request()->deserialize(raw_data);
        callback_(message_);
        ipc::buffer response_data(std::move(message_->response()->serialize()));
        ipc_w_ptr_->send(response_data.data(), response_data.size());
      } // 客户段发送请求
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/

    shm_cli_ipc::shm_cli_ipc(const std::string &topic_name_, const ServiceDataPtr &msg,
                             bool verbose)
        : topic_name_(topic_name_), message_(msg), verbose_(verbose) {}

    shm_cli_ipc::~shm_cli_ipc()
    {
      if (ipc_r_ptr_ && ipc_r_ptr_->valid())
      {
        ipc_r_ptr_->release();
      }
      if (ipc_w_ptr_ && ipc_w_ptr_->valid())
      {
        ipc_w_ptr_->release();
      }
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/

    void shm_cli_ipc::InitChannel()
    {
      // 等待服务端创建通道
      rev_cleanup_broadcast();
      pool_reg_.rebind({dzIPC::info_pool::EntryKind::ShmClient,
                        topic_name_,
                        message_ ? dzIPC::info_pool::demangle(typeid(*message_).name()) : std::string{},
                        /*domain_id=*/0,
                        ""});
      if (verbose_)
      {
        std::cerr << "\033[33m[" << topic_name_
                  << "_Info] Client connected to server topic: " << topic_name_
                  << "\033[0m" << std::endl;
      }
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/

    void shm_cli_ipc::rev_cleanup_broadcast()
    {
      std::string sem_ser_name = "dz_ipc_" + topic_name_ + "_ser_post";
      std::string sem_cli_name = "dz_ipc_" + topic_name_ + "_cli_post";
      ipc::sync::semaphore sem_ser;
      if (!sem_ser.open(sem_ser_name.c_str(), 0))
      {
        throw std::runtime_error("sem_open failed");
      }
      ipc::sync::semaphore sem_cli;
      if (!sem_cli.open(sem_cli_name.c_str(), 0))
      {
        sem_ser.close();
        throw std::runtime_error("sem_open failed");
      }
      while (sem_cli.try_wait())
        ;             // 清空信号量
      sem_ser.wait(); // 等待服务端发送清理开始信号
      // 创建新的通道
      std::string r_name, w_name;
      r_name = "dz_ipc_" + topic_name_ + "_ser_w";
      w_name = "dz_ipc_" + topic_name_ + "_ser_r";
      ipc_r_ptr_ =
          std::make_shared<ipc::server>(r_name.c_str(), ipc::receiver, verbose_);
      ipc_w_ptr_ =
          std::make_shared<ipc::server>(w_name.c_str(), ipc::sender, verbose_);
      sem_cli.post(); // 通知服务端清理完成
      sem_ser.close();
      sem_cli.close();
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_cli_ipc::send_request(ServiceDataPtr &request)
    {
      ipc::buffer request_data(std::move(request->request()->serialize()));
      int retry_count = 0;
      while (ipc_w_ptr_->send(request_data.data(), request_data.size()) == false)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (retry_count >= 10)
        {
          return;
        }
        retry_count++;
      };

      ipc::buffer raw_response = ipc_r_ptr_->recv();
      if (!message_->check_msg_id(raw_response))
      {
        if (verbose_)
        {
          std::cerr << "\033[31m[Warning] Response message with invalid ID on topic: "
                    << topic_name_ << "\033[0m" << std::endl;
        }
      }
      else
      {
        request->response()->deserialize(raw_response);
      }
      /* 反序列转换为msg数据 */
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_cli_ipc::reset_message(const ServiceDataPtr &msg)
    {
      message_.reset(msg->clone());
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_cli_ipc::reset_sync()
    {
      std::string sem_ser_name = "dz_ipc_" + topic_name_ + "_ser_reset_sync";
      std::string sem_cli_name = "dz_ipc_" + topic_name_ + "_cli_reset_sync";
      ipc::sync::semaphore sem_ser;
      if (sem_ser.open(sem_ser_name.c_str(), 0))
      {
        throw std::runtime_error("sem_open failed");
      }
      ipc::sync::semaphore sem_cli;
      if (sem_cli.open(sem_cli_name.c_str(), 0))
      {
        sem_ser.close();
        throw std::runtime_error("sem_open failed");
      }
      if (verbose_)
        std::cerr << "[ShmIPCNode] Client resetting sync." << std::endl;
      sem_ser.wait(); // 等待服务端发送清理开始信号
      sem_cli.post(); // 通知服务端清理完成
      sem_ser.close();
      sem_cli.close();
    }
  }
} // namespace dzIPC