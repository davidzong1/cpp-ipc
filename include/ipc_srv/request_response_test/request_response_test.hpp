#pragma once
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include "ipc_msg/ipc_msg_base/ipc_msg_base.hpp"

namespace dzIPC::Srv {

// 请求类
class request_response_test_Request : public ipc_msg_base
{
public:
    /* 构造函数和析构函数 */
    request_response_test_Request() = default;
    ~request_response_test_Request() = default;

    /* 成员变量 */
    std::vector<double> request; /* request */

    /* 序列化函数 */
    std::vector<char> serialize() const override
    {
        int32_t request_count = request.size();
        int32_t request_size = request_count * sizeof(double);

        // 计算总缓冲区大小
        size_t total_size_ = 0;
        total_size_ += sizeof(request_count) + request_size;

        // 一次性分配缓冲区
        std::vector<char> buffer(total_size_);
        total_size = total_size_;
        size_t offset = 0;

        // 序列化 request
        std::memcpy(buffer.data() + offset, &request_count, sizeof(request_count));
        offset += sizeof(request_count);
        if (request_count > 0) {
            std::memcpy(buffer.data() + offset, request.data(), request_size);
            offset += request_size;
        }

        return buffer;
    }

    /* 反序列化函数 */
    void deserialize(const std::vector<char>& buffer) override
    {
        size_t offset = 0;
        // 反序列化 request
        int32_t request_count;
        std::memcpy(&request_count, buffer.data() + offset, sizeof(request_count));
        offset += sizeof(request_count);
        request.resize(request_count);
        if (request_count > 0) {
            std::memcpy(request.data(), buffer.data() + offset, request_count * sizeof(double));
            offset += request_count * sizeof(double);
        }

    }

    /* 克隆函数 */
    request_response_test_Request* clone() const override
    {
        return new request_response_test_Request(*this);
    }
};

// 响应类
class request_response_test_Response : public ipc_msg_base
{
public:
    /* 构造函数和析构函数 */
    request_response_test_Response() = default;
    ~request_response_test_Response() = default;

    /* 成员变量 */
    std::vector<double> response; /* response */

    /* 序列化函数 */
    std::vector<char> serialize() const override
    {
        int32_t response_count = response.size();
        int32_t response_size = response_count * sizeof(double);

        // 计算总缓冲区大小
        size_t total_size_ = 0;
        total_size_ += sizeof(response_count) + response_size;

        // 一次性分配缓冲区
        std::vector<char> buffer(total_size_);
        total_size = total_size_;
        size_t offset = 0;

        // 序列化 response
        std::memcpy(buffer.data() + offset, &response_count, sizeof(response_count));
        offset += sizeof(response_count);
        if (response_count > 0) {
            std::memcpy(buffer.data() + offset, response.data(), response_size);
            offset += response_size;
        }

        return buffer;
    }

    /* 反序列化函数 */
    void deserialize(const std::vector<char>& buffer) override
    {
        size_t offset = 0;
        // 反序列化 response
        int32_t response_count;
        std::memcpy(&response_count, buffer.data() + offset, sizeof(response_count));
        offset += sizeof(response_count);
        response.resize(response_count);
        if (response_count > 0) {
            std::memcpy(response.data(), buffer.data() + offset, response_count * sizeof(double));
            offset += response_count * sizeof(double);
        }

    }

    /* 克隆函数 */
    request_response_test_Response* clone() const override
    {
        return new request_response_test_Response(*this);
    }
};

} // namespace dzIPC::Srv