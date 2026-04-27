#ifndef TOPIC_DATA_H
#define TOPIC_DATA_H
#include <memory>
#include "ipc_msg/ipc_msg_base/ipc_msg_base.hpp"
namespace dzIPC
{
  class TopicData
  {
  public:
    TopicData(const std::shared_ptr<ipc_msg_base> &topic, size_t msg_id = 0)
    {
      topic_.reset(topic->clone());
      topic_cache.reset(topic->clone());
      topic_->set_msg_id(msg_id);
      topic_cache->set_msg_id(msg_id);
      msg_id_ = msg_id;
    }

    TopicData(std::shared_ptr<ipc_msg_base> &&topic, size_t msg_id = 0) : topic_(std::move(topic)), msg_id_(msg_id)
    {
      topic_cache.reset(topic_->clone());
      topic_->set_msg_id(msg_id);
      topic_cache->set_msg_id(msg_id);
    }
    std::shared_ptr<ipc_msg_base> &topic() { return topic_; };

    TopicData *clone() const { return new TopicData(*this); }
    void update(std::shared_ptr<ipc_msg_base> &other)
    {
      topic_ = std::move(other);
    }
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
      msg_id_ = other.msg_id_;
    }
    TopicData() = default;
    std::shared_ptr<ipc_msg_base> topic_, topic_cache;
    size_t msg_id_{0};
  };
} // namespace dzIPC

#endif // DATA_H