#ifndef TOPIC_DATA_H
#define TOPIC_DATA_H
#include <memory>
#include "ipc_msg/ipc_msg_base/ipc_msg_base.hpp"
namespace dzIPC {
class TopicData {
 public:
  TopicData(const std::shared_ptr<ipc_msg_base>& topic) {
    topic_.reset(topic->clone());
    topic_cache.reset(topic->clone());
  }

  TopicData(std::shared_ptr<ipc_msg_base>&& topic) : topic_(std::move(topic)) {
    topic_cache.reset(topic_->clone());
  }
  std::shared_ptr<ipc_msg_base>& topic() { return topic_; };

  TopicData* clone() const { return new TopicData(*this); }
  void swap(std::shared_ptr<ipc_msg_base>& other) {
    other = std::move(topic_);
    topic_.reset(topic_cache->clone());
  }

 private:
  TopicData(const TopicData& other) {
    topic_.reset(other.topic_->clone());
    topic_cache.reset(other.topic_cache->clone());
  }
  TopicData() = default;
  std::shared_ptr<ipc_msg_base> topic_, topic_cache;
};
}  // namespace dzIPC

#endif  // DATA_H