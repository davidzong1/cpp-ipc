#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "dzIPC/common/topic_data.h"
#include "dzIPC/common/circularqueue.h"
#include "dzIPC/ipc_info_pool.h"
#include "libipc/ipc.h"
#include <thread>
#include <atomic>
#include "libipc/semaphore.h"
#include "libipc/count_sem.h"

namespace dzIPC
{
  namespace shm
  {
    class shm_pub_ipc;
    class shm_sub_ipc;
    using TopicDataPtr = std::shared_ptr<TopicData>;
    using MsgPtr = std::shared_ptr<ipc_msg_base>;
    using PublishPtr = std::shared_ptr<shm_pub_ipc>;
    using SubscribePtr = std::shared_ptr<shm_sub_ipc>;
    class shm_pub_ipc
    {
    public:
      explicit shm_pub_ipc(const std::string &topic_name, bool verbose = false);
      ~shm_pub_ipc();
      void InitChannel();
      void publish(MsgPtr msg);
      bool has_subscribed() const { return subscribed_; }
      /* 禁用拷贝 */
      shm_pub_ipc(const shm_pub_ipc &) = delete;
      shm_pub_ipc &operator=(const shm_pub_ipc &) = delete;

    private:
      void sub_listener();

    private:
      std::atomic<bool> subscribed_{false};
      bool verbose_{false};
      bool running{true};
      std::string topic_name_;
      std::string raw_topic_name_;
      std::shared_ptr<ipc::route> publisher_;
      std::thread publish_thread_;
      dzIPC::info_pool::ScopedRegistration pool_reg_;
    };

    class shm_sub_ipc
    {
    public:
      explicit shm_sub_ipc(const std::string &topic_name, const TopicDataPtr &msg,
                           const size_t queue_size, bool verbose = false);
      ~shm_sub_ipc();
      void InitChannel();
      void reset_message(const TopicDataPtr &msg);
      void get(MsgPtr &msg);
      bool try_get(MsgPtr &msg);
      /* 禁用拷贝 */
      shm_sub_ipc(const shm_sub_ipc &) = delete;
      shm_sub_ipc &operator=(const shm_sub_ipc &) = delete;

    private:
      bool running{true};
      bool data_update_{false};
      bool verbose_{false};
      std::string topic_name_;
      std::string raw_topic_name_;
      std::shared_ptr<ipc::route> subscriber_;
      TopicDataPtr topic_msg_;
      std::unique_ptr<CircularQueue<ipc_msg_base>> msg_queue_;
      std::thread subscribe_thread_;
      ipc::sync::count_sem *empty_queue_; // 用于通知订阅者消息队列中有新消息
      dzIPC::info_pool::ScopedRegistration pool_reg_;
    };
  }
} // namespace dzIPC