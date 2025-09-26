#pragma once

#include <vector>

class ipc_msg_base {
 public:
  ipc_msg_base() = default;
  ~ipc_msg_base() = default;
  /* 序列化函数 */
  virtual std::vector<char> serialize() const = 0;
  /* 反序列化函数 */
  virtual void deserialize(const std::vector<char>& data) = 0;
  /* 克隆函数 */
  virtual ipc_msg_base* clone() const = 0;

 protected:
  mutable size_t total_size = 0;
};