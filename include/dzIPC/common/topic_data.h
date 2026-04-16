#ifndef TOPIC_DATA_H
#define TOPIC_DATA_H
#include <memory>
#include "ipc_msg/ipc_msg_base/ipc_msg_base.hpp"
namespace dzIPC
{
  class TopicData
  {
  public:
    TopicData(const std::shared_ptr<ipc_msg_base> &topic, size_t domain_id = 0)
    {
      topic_.reset(topic->clone());
      topic_cache.reset(topic->clone());
      topic_->set_msg_id(domain_id);
      topic_cache->set_msg_id(domain_id);
      domain_id_ = domain_id;
    }

    TopicData(std::shared_ptr<ipc_msg_base> &&topic, size_t domain_id = 0) : topic_(std::move(topic)), domain_id_(domain_id)
    {
      topic_cache.reset(topic_->clone());
      topic_->set_msg_id(domain_id);
      topic_cache->set_msg_id(domain_id);
    }
    std::shared_ptr<ipc_msg_base> &topic() { return topic_; };

    TopicData *clone() const { return new TopicData(*this); }
    void swap(std::shared_ptr<ipc_msg_base> &other)
    {
      other = std::move(topic_);
      topic_.reset(topic_cache->clone());
    }
    bool check_msg_id(const ipc::buffer &data)
    {
      return topic_cache->check_id(data);
    }

  private:
    TopicData(const TopicData &other)
    {
      topic_.reset(other.topic_->clone());
      topic_cache.reset(other.topic_cache->clone());
      domain_id_ = other.domain_id_;
    }
    TopicData() = default;
    std::shared_ptr<ipc_msg_base> topic_, topic_cache;
    size_t domain_id_{0};
  };
} // namespace dzIPC

#endif // DATA_H