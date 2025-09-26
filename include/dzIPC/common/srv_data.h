#ifndef SRV_DATA_H
#define SRV_DATA_H
#include <memory>
#include "ipc_msg/ipc_msg_base/ipc_msg_base.hpp"
namespace dzIPC {
class ServiceData {
 public:
  ServiceData(const std::shared_ptr<ipc_msg_base>& request,
              const std::shared_ptr<ipc_msg_base>& response) {
    request_.reset(request->clone());
    response_.reset(response->clone());
  }

  ServiceData(std::shared_ptr<ipc_msg_base>&& request,
              std::shared_ptr<ipc_msg_base>&& response)
      : request_(std::move(request)), response_(std::move(response)) {}
  std::shared_ptr<ipc_msg_base>& request() { return request_; };
  std::shared_ptr<ipc_msg_base>& response() { return response_; }
  ServiceData(const std::shared_ptr<ServiceData>& other)
      : request_(other->request_->clone()),
        response_(other->response_->clone()) {}
  ServiceData* clone() { return new ServiceData(*this); }

 private:
  ServiceData(const ServiceData& other) {
    request_.reset(other.request_->clone());
    response_.reset(other.response_->clone());
  }
  ServiceData() = default;
  std::shared_ptr<ipc_msg_base> request_;
  std::shared_ptr<ipc_msg_base> response_;
};
}  // namespace dzIPC

#endif  // DATA_H