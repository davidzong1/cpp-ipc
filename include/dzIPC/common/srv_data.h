#ifndef SRV_DATA_H
#define SRV_DATA_H
#include <memory>
#include "ipc_msg/ipc_msg_base/ipc_msg_base.hpp"
namespace dzIPC
{
  class ServiceData
  {
  public:
    ServiceData(const std::shared_ptr<ipc_msg_base> &request,
                const std::shared_ptr<ipc_msg_base> &response, uint32_t msg_id = 0) : msg_id_(msg_id)
    {
      request_.reset(request->clone());
      response_.reset(response->clone());
      request_->set_msg_id(msg_id);
      response_->set_msg_id(msg_id);
    }

    ServiceData(std::shared_ptr<ipc_msg_base> &&request,
                std::shared_ptr<ipc_msg_base> &&response, uint32_t msg_id = 0)
        : request_(std::move(request)), response_(std::move(response)), msg_id_(msg_id)
    {
      request_->set_msg_id(msg_id);
      response_->set_msg_id(msg_id);
    }

    std::shared_ptr<ipc_msg_base> &request() { return request_; };

    std::shared_ptr<ipc_msg_base> &response() { return response_; }

    ServiceData(const std::shared_ptr<ServiceData> &other)
        : request_(other->request_->clone()),
          response_(other->response_->clone()), msg_id_(other->msg_id_) {}

    ServiceData *clone() { return new ServiceData(*this); }

    bool check_msg_id(const ipc::buffer &data)
    {
      return request_->check_id(data);
    }

  private:
    ServiceData(const ServiceData &other)
    {
      request_.reset(other.request_->clone());
      response_.reset(other.response_->clone());
      msg_id_ = other.msg_id_;
    }
    ServiceData() = default;
    std::shared_ptr<ipc_msg_base> request_;
    std::shared_ptr<ipc_msg_base> response_;
    uint32_t msg_id_;
  };
} // namespace dzIPC

#endif // DATA_H