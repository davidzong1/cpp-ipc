#include <dzIPC/pub_sub_ipc.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <fcntl.h>
namespace dzIPC {
using namespace ipc;
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
pub_ipc::pub_ipc(const std::string& topic_name, bool verbose)
    : topic_name_("dz_ipc_" + topic_name + "_topic"), verbose_(verbose) {}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
pub_ipc::~pub_ipc() {
  running = false;
  if (publish_thread_.joinable()) {
    publish_thread_.join();
  }
  if (publisher_ && publisher_->valid()) {
    publisher_->clear();
  }
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void pub_ipc::InitChannel() {
  try {
    ipc::route::clear_storage(topic_name_.c_str());
    publisher_ = std::make_shared<ipc::route>(topic_name_.c_str(), ipc::sender,
                                              verbose_);
    publish_thread_ = std::thread(&pub_ipc::sub_listener, this);
  } catch (const std::exception& e) {
    std::cerr << "\033[31mError initializing channel: " << e.what() << "\033[0m"
              << std::endl;
  }
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void pub_ipc::sub_listener() {
  std::string pub_sem_name = "/" + topic_name_ + "_pub_node";
  std::string sub_sem_name = "/" + topic_name_ + "_sub_node";
  sem_t* pub_node = sem_open(pub_sem_name.c_str(), O_CREAT, 0644, 0);
  sem_t* sub_node = sem_open(sub_sem_name.c_str(), O_CREAT, 0644, 0);
  if (pub_node == SEM_FAILED || sub_node == SEM_FAILED) {
    perror("sem_open");
    if (pub_node != SEM_FAILED) {
      sem_close(pub_node);
    }
    if (sub_node != SEM_FAILED) {
      sem_close(sub_node);
    }
    std::cerr << "\033[31mError opening semaphores for topic: " << topic_name_
              << "\033[0m" << std::endl;
    throw std::runtime_error("Fatal error: sem_open failed");
  }
  while (sem_trywait(sub_node) == 0);
  std::cerr << "\033[33m[PublishSubscribeInfo] Publisher has created topic: "
            << topic_name_ << "\033[0m" << std::endl;
  while (running) {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts) != 0;
    ts.tv_nsec += 20 * 1000000;  // 20 ms
    if (ts.tv_nsec >= 1000000000) {
      ts.tv_sec += ts.tv_nsec / 1000000000;
      ts.tv_nsec = ts.tv_nsec % 1000000000;
    }
    if (sem_timedwait(sub_node, &ts) == 0) {
      sem_post(pub_node);
      subscribed_.store(true, std::memory_order_relaxed);
      std::cerr << "\033[33m[PublishSubscribeInfo] Publisher detected a "
                   "subscriber on topic: "
                << topic_name_ << "\033[0m" << std::endl;
    }
  }
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void pub_ipc::publish(MsgPtr msg) {
  try {
    if (subscribed_.load(std::memory_order_acquire)) {
      std::vector<char> response_data(std::move(msg->serialize()));
      publisher_->send(response_data.data(), response_data.size());
    }
  } catch (const std::exception& e) {
    std::cerr << "\033[31mError publishing message: " << e.what() << "\033[0m"
              << std::endl;
  }
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
sub_ipc::sub_ipc(const std::string& topic_name, const TopicDataPtr& msg,
                 const size_t queue_size, bool verbose)
    : topic_name_("dz_ipc_" + topic_name + "_topic"), verbose_(verbose) {
  topic_msg_.reset(msg->clone());
  msg_queue_ = std::make_unique<CircularQueue<ipc_msg_base>>(queue_size);
  sem_init(&empty_queue_, 0, 0);
}
sub_ipc::~sub_ipc() {
  running = false;
  if (subscribe_thread_.joinable()) {
    subscribe_thread_.join();
  }
  if (subscriber_ && subscriber_->valid()) {
    subscriber_->clear();
  }
  sem_destroy(&empty_queue_);
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void sub_ipc::reset_message(const TopicDataPtr& msg) {
  topic_msg_.reset(msg->clone());
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void sub_ipc::InitChannel() {
  subscribe_thread_ = std::thread([this]() {
    std::string pub_sem_name = "/" + topic_name_ + "_pub_node";
    std::string sub_sem_name = "/" + topic_name_ + "_sub_node";
    sem_t* pub_node = sem_open(pub_sem_name.c_str(), O_CREAT, 0644, 0);
    sem_t* sub_node = sem_open(sub_sem_name.c_str(), O_CREAT, 0644, 0);
    if (pub_node == SEM_FAILED || sub_node == SEM_FAILED) {
      perror("sem_open");
      if (pub_node != SEM_FAILED) {
        sem_close(pub_node);
      }
      if (sub_node != SEM_FAILED) {
        sem_close(sub_node);
      }
      std::cerr << "\033[31mError opening semaphores for topic: " << topic_name_
                << "\033[0m" << std::endl;
      throw std::runtime_error("Fatal error: sem_open failed");
    }
    while (sem_trywait(pub_node) == 0);
    while (running) {
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts) != 0;
      ts.tv_nsec += 5 * 1000000;  // 10 ms -> ns
      if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += ts.tv_nsec / 1000000000;
        ts.tv_nsec = ts.tv_nsec % 1000000000;
      }
      sem_post(sub_node);
      if (sem_timedwait(pub_node, &ts) == 0) {
        try {
          subscriber_ = std::make_shared<ipc::route>(topic_name_.c_str(),
                                                     ipc::receiver, verbose_);
        } catch (...) {
          std::cerr << "\033[31mError initializing subscriber channel: "
                    << topic_name_ << "\033[0m" << std::endl;
          throw std::runtime_error("Error initializing subscriber channel: " +
                                   topic_name_);
        }
        break;
      }
    }
    std::cerr
        << "\033[33m[PublishSubscribeInfo] Subscriber has subscribed to topic: "
        << topic_name_ << "\033[0m" << std::endl;
    while (running) {
      buff_t raw_data_buff = subscriber_->recv(10);
      if (raw_data_buff.empty()) {
        continue;
      }
      std::vector<char> raw_data(
          static_cast<const char*>(raw_data_buff.data()),
          static_cast<const char*>(raw_data_buff.data()) +
              raw_data_buff.size());
      topic_msg_->topic()->deserialize(raw_data);
      MsgPtr ptr_cache;
      topic_msg_->swap(ptr_cache);
      msg_queue_->push(ptr_cache);
      while (sem_trywait(&empty_queue_) == 0);  // 清空信号量，防止积压
      sem_post(&empty_queue_);
    }
    sem_close(pub_node);
    sem_close(sub_node);
  });
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void sub_ipc::get(MsgPtr& msg) {
  if (msg_queue_->try_pop(msg)) {
    sem_wait(&empty_queue_);
    msg_queue_->try_pop(msg);
  }
}
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
bool sub_ipc::try_get(MsgPtr& msg) { return msg_queue_->try_pop(msg); }
}  // namespace dzIPC