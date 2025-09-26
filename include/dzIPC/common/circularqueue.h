#pragma once
#include <mutex>
template <typename msgType>
class CircularQueue {
 public:
  using MsgPtr = std::shared_ptr<msgType>;
  explicit CircularQueue(size_t capacity)
      : cap_(capacity), head_(0), tail_(0), count_(0) {
    buf_.resize(capacity);
  }

  // 推入一条消息；当队列满时覆盖最旧消息（保证接收最新）
  void push(const MsgPtr& msg) {
    std::lock_guard<std::mutex> lk(mtx_);
    buf_[tail_] = msg;
    tail_ = (tail_ + 1) % cap_;
    if (count_ == cap_) {
      head_ = tail_;  // 覆盖 oldest：移动 head 指向下一个
    } else {
      ++count_;
    }
  }
  void push(const MsgPtr&& msg) {
    std::lock_guard<std::mutex> lk(mtx_);
    buf_[tail_] = std::move(msg);
    tail_ = (tail_ + 1) % cap_;
    if (count_ == cap_) {
      head_ = tail_;  // 覆盖 oldest：移动 head 指向下一个
    } else {
      ++count_;
    }
  }
  // 尝试弹出一条消息，成功返回 true 并将消息写入 out
  bool try_pop(MsgPtr& out) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (count_ == 0) return false;
    out = buf_[head_];
    head_ = (head_ + 1) % cap_;
    --count_;
    return true;
  }
  bool try_pop(MsgPtr&& out) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (count_ == 0) return false;
    out = std::move(buf_[head_]);
    head_ = (head_ + 1) % cap_;
    --count_;
    return true;
  }
  size_t size() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return count_;
  }

 private:
  std::vector<MsgPtr> buf_;
  size_t cap_;
  size_t head_;
  size_t tail_;
  size_t count_;
  mutable std::mutex mtx_;
};