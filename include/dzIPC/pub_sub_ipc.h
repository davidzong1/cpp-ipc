#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "dzIPC/common/topic_data.h"
#include "dzIPC/common/circularqueue.h"
#include "libipc/ipc.h"
#include <thread>
#include <atomic>
#include <semaphore.h>

namespace dzIPC {
class pub_ipc;
class sub_ipc;
using TopicDataPtr = std::shared_ptr<TopicData>;
using MsgPtr = std::shared_ptr<ipc_msg_base>;
using PublishPtr = std::shared_ptr<pub_ipc>;
using SubscribePtr = std::shared_ptr<sub_ipc>;
class pub_ipc {
 public:
  explicit pub_ipc(const std::string& topic_name, bool verbose = false);
  ~pub_ipc();
  void InitChannel();
  void publish(MsgPtr msg);
  bool has_subscribed() const { return subscribed_; }
  /* 禁用拷贝 */
  pub_ipc(const pub_ipc&) = delete;
  pub_ipc& operator=(const pub_ipc&) = delete;

 private:
  void sub_listener();

 private:
  std::atomic<bool> subscribed_{false};
  bool verbose_{false};
  bool running{true};
  std::string topic_name_;
  std::shared_ptr<ipc::route> publisher_;
  std::thread publish_thread_;
};

class sub_ipc {
 public:
  explicit sub_ipc(const std::string& topic_name, const TopicDataPtr& msg,
                   const size_t queue_size, bool verbose = false);
  ~sub_ipc();
  void InitChannel();
  void reset_message(const TopicDataPtr& msg);
  void get(MsgPtr& msg);
  bool try_get(MsgPtr& msg);
  /* 禁用拷贝 */
  sub_ipc(const sub_ipc&) = delete;
  sub_ipc& operator=(const sub_ipc&) = delete;

 private:
  bool running{true};
  bool data_update_{false};
  bool verbose_{false};
  std::atomic<bool> sub_paused_{false};  // 线程暂停，用于切换msg
  std::string topic_name_;
  std::shared_ptr<ipc::route> subscriber_;
  TopicDataPtr topic_msg_;
  std::unique_ptr<CircularQueue<ipc_msg_base>> msg_queue_;
  std::thread subscribe_thread_;
  sem_t empty_queue_;
};
}  // namespace dzIPC