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
                const std::shared_ptr<ipc_msg_base> &response, size_t domain_id = 0)
    {
      request_.reset(request->clone());
      response_.reset(response->clone());
      request_->set_msg_id(domain_id);
      response_->set_msg_id(domain_id);
      domain_id_ = domain_id;
    }

    ServiceData(std::shared_ptr<ipc_msg_base> &&request,
                std::shared_ptr<ipc_msg_base> &&response, size_t domain_id = 0)
        : request_(std::move(request)), response_(std::move(response)), domain_id_(domain_id)
    {
      request_->set_msg_id(domain_id);
      response_->set_msg_id(domain_id);
    }

    std::shared_ptr<ipc_msg_base> &request() { return request_; };

    std::shared_ptr<ipc_msg_base> &response() { return response_; }

    ServiceData(const std::shared_ptr<ServiceData> &other)
        : request_(other->request_->clone()),
          response_(other->response_->clone()),
          domain_id_(other->domain_id_) {}

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
      domain_id_ = other.domain_id_;
    }
    ServiceData() = default;
    std::shared_ptr<ipc_msg_base> request_;
    std::shared_ptr<ipc_msg_base> response_;
    size_t domain_id_{0};
  };
} // namespace dzIPC

#endif // DATA_H