#include <dzIPC/shm_pub_sub_ipc.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <fcntl.h>
#include <typeinfo>
namespace dzIPC
{
  namespace shm
  {
    using namespace ipc;
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    shm_pub_ipc::shm_pub_ipc(const std::string &topic_name, bool verbose)
        : topic_name_("dz_ipc_" + topic_name + "_topic"), raw_topic_name_(topic_name), verbose_(verbose) {}
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    shm_pub_ipc::~shm_pub_ipc()
    {
      running = false;
      if (publish_thread_.joinable())
      {
        publish_thread_.join();
      }
      if (publisher_ && publisher_->valid())
      {
        publisher_->clear();
      }
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_pub_ipc::InitChannel()
    {
      try
      {
        ipc::route::clear_storage(topic_name_.c_str());
        publisher_ = std::make_shared<ipc::route>(topic_name_.c_str(), ipc::sender,
                                                  verbose_);
        pool_reg_.rebind({dzIPC::info_pool::EntryKind::ShmPub,
                          raw_topic_name_,
                          /*type_name=*/"",
                          /*domain_id=*/0,
                          "channel=" + topic_name_});
        publish_thread_ = std::thread(&shm_pub_ipc::sub_listener, this);
      }
      catch (const std::exception &e)
      {
        std::cerr << "\033[31m[PublisherInfo] Error initializing channel: " << e.what() << "\033[0m"
                  << std::endl;
      }
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_pub_ipc::sub_listener()
    {
      std::string pub_sem_name = "/" + topic_name_ + "_pub_node";
      std::string sub_sem_name = "/" + topic_name_ + "_sub_node";
      ipc::sync::semaphore pub_node;
      ipc::sync::semaphore sub_node;
      if (!pub_node.open(pub_sem_name.c_str(), 0))
      {
        std::cerr << "\033[31m[PublisherInfo] Error opening publisher semaphore for topic: "
                  << topic_name_ << "\033[0m" << std::endl;
        throw std::runtime_error("Fatal error: sem_open failed");
      }
      if (!sub_node.open(sub_sem_name.c_str(), 0))
      {
        pub_node.close();
        std::cerr << "\033[31m[PublisherInfo] Error opening subscriber semaphore for topic: "
                  << topic_name_ << "\033[0m" << std::endl;
        throw std::runtime_error("Fatal error: sem_open failed");
      }

      while (sub_node.try_wait())
        ;
      if (verbose_)
        std::cerr << "\033[33m[PublisherInfo] Publisher has created topic: "
                  << topic_name_ << "\033[0m" << std::endl;
      // After the connection is established, the publisher enters a waiting loop until it detects
      // the subscriber's presence (synchronized via semaphore), and then begins publishing messages.
      uint64_t ts = 100; // 100ms
      while (running)
      {
        if (sub_node.wait(ts))
        {
          pub_node.post();
          subscribed_.store(true, std::memory_order_relaxed);
          if (verbose_)
            std::cerr << "\033[33m[PublisherInfo] Publisher detected a "
                         "subscriber on topic: "
                      << topic_name_ << "\033[0m" << std::endl;
        }
      }
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_pub_ipc::publish(MsgPtr msg)
    {
      try
      {
        if (subscribed_.load(std::memory_order_acquire))
        {
          ipc::buffer response_data(std::move(msg->serialize()));
          publisher_->send(response_data.data(), response_data.size());
        }
      }
      catch (const std::exception &e)
      {
        std::cerr << "\033[31m[PublisherInfo] Error publishing message: " << e.what() << "\033[0m"
                  << std::endl;
      }
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    shm_sub_ipc::shm_sub_ipc(const std::string &topic_name, const TopicDataPtr &msg,
                             const size_t queue_size, bool verbose)
        : topic_name_("dz_ipc_" + topic_name + "_topic"), raw_topic_name_(topic_name), verbose_(verbose)
    {
      topic_msg_.reset(msg->clone());
      msg_queue_ = std::make_unique<CircularQueue<ipc_msg_base>>(queue_size);
      // empty_queue_ = new ipc::sync::count_sem(0);
    }
    shm_sub_ipc::~shm_sub_ipc()
    {
      running = false;
      if (subscribe_thread_.joinable())
      {
        subscribe_thread_.join();
      }
      if (subscriber_ && subscriber_->valid())
      {
        subscriber_->release();
      }
      // delete empty_queue_;
      // empty_queue_ = nullptr;
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_sub_ipc::reset_message(const TopicDataPtr &msg)
    {
      topic_msg_.reset(msg->clone());
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_sub_ipc::InitChannel()
    {
      pool_reg_.rebind({dzIPC::info_pool::EntryKind::ShmSub,
                        raw_topic_name_,
                        topic_msg_ && topic_msg_->topic()
                            ? dzIPC::info_pool::demangle(typeid(*topic_msg_->topic()).name())
                            : std::string{},
                        /*domain_id=*/0,
                        "channel=" + topic_name_});
      subscribe_thread_ = std::thread([this]()
                                      {
    std::string pub_sem_name = "/" + topic_name_ + "_pub_node";
    std::string sub_sem_name = "/" + topic_name_ + "_sub_node";
    ipc::sync::semaphore pub_node;
    ipc::sync::semaphore sub_node;
    if (!pub_node.open(pub_sem_name.c_str(), 0))
    {
      std::cerr << "\033[31m[SubscriberInfo] Error opening publisher semaphore for topic: "
                << topic_name_ << "\033[0m" << std::endl;
      throw std::runtime_error("[SubscriberInfo] Fatal error: sem_open failed");
    }
    if (!sub_node.open(sub_sem_name.c_str(), 0))
    {
      pub_node.close();
      std::cerr << "\033[31m[SubscriberInfo] Error opening subscriber semaphore for topic: "
                << topic_name_ << "\033[0m" << std::endl;
      throw std::runtime_error("[SubscriberInfo] Fatal error: sem_open failed");
    }
    while (pub_node.try_wait())
      ;// 清空信号量，防止残留
    /*  循环检测发布者是否准备好 */
    while (running) {
      uint64_t ts=100; // 100ms
      sub_node.post(); // 通知发布者自己已准备好
      if (pub_node.wait(ts)) {
        try {
          subscriber_ = std::make_shared<ipc::route>(topic_name_.c_str(),
                                                     ipc::receiver, verbose_);
        } catch (...) {
          std::cerr << "\033[31m[SubscriberInfo] Error initializing subscriber channel: "
                    << topic_name_ << "\033[0m" << std::endl;
          throw std::runtime_error("[SubscriberInfo] Error initializing subscriber channel: " +
                                   topic_name_);
        }
        break;
      }
    }
    pub_node.close();
    sub_node.close();
    if (verbose_)
    {
      std::cerr
          << "\033[33m[SubscriberInfo] Subscriber has subscribed to topic: "
          << topic_name_ << "\033[0m" << std::endl;
    }
    /* 进入订阅循环 */
    while (running) {
      buff_t raw_data = subscriber_->recv(10);
      if (raw_data.empty()) {
        continue;
      }
      if(!topic_msg_->check_msg_id(raw_data))
        continue;
      topic_msg_->topic()->deserialize(raw_data);
      MsgPtr ptr_cache;
      topic_msg_->swap(ptr_cache);
      msg_queue_->push(std::move(ptr_cache));
      // while (empty_queue_->try_wait());  // 清空信号量，防止积压
      // empty_queue_->post();
    } });
      subscribe_thread_.detach();
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    void shm_sub_ipc::get(MsgPtr &msg)
    {
      msg_queue_->pop(msg);
    }
    /******************************************************************************************************/
    /******************************************************************************************************/
    /******************************************************************************************************/
    bool shm_sub_ipc::try_get(MsgPtr &msg) { return msg_queue_->try_pop(msg); }
  }
} // namespace dzIPC