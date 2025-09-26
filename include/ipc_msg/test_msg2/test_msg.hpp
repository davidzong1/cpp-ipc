#pragma once
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include "ipc_msg/ipc_msg_base/ipc_msg_base.hpp"
namespace dzIPC::Msg {
class test_msg : public ipc_msg_base
{
public:
    /* 构造函数和析构函数 */
    test_msg() = default;
    ~test_msg() = default;


    /* 成员变量 */

    std::vector<double> data1; /* data1 */
    std::vector<int32_t> data2; /* data2 */
    std::vector<std::string> data3; /* data3 */
    bool data4; /* data4 */

    /* 序列化函数 */
    std::vector<char> serialize() const
    {
        int32_t data1_count = data1.size();
        int32_t data1_size = data1_count * sizeof(double);
        int32_t data2_count = data2.size();
        int32_t data2_size = data2_count * sizeof(int32_t);
        int32_t data3_count = data3.size();
        int32_t data3_total_size_ = 0;
        for (const auto& str : data3) {
            data3_total_size_ += sizeof(int32_t) + str.size() * sizeof(char);
        }
        int32_t data4_size = sizeof(data4);

        // 计算总缓冲区大小
        size_t total_size_ = 0;
        total_size_ += sizeof(data1_count) + data1_size;
        total_size_ += sizeof(data2_count) + data2_size;
        total_size_ += sizeof(data3_count) + data3_total_size_;
        total_size_ += data4_size;

        // 一次性分配缓冲区
        std::vector<char> buffer(total_size_);
        total_size = total_size_;
        size_t offset = 0;

        // 序列化 data1
        std::memcpy(buffer.data() + offset, &data1_count, sizeof(data1_count));
        offset += sizeof(data1_count);
        if (data1_count > 0) {
            std::memcpy(buffer.data() + offset, data1.data(), data1_size);
            offset += data1_size;
        }

        // 序列化 data2
        std::memcpy(buffer.data() + offset, &data2_count, sizeof(data2_count));
        offset += sizeof(data2_count);
        if (data2_count > 0) {
            std::memcpy(buffer.data() + offset, data2.data(), data2_size);
            offset += data2_size;
        }

        // 序列化 data3
        std::memcpy(buffer.data() + offset, &data3_count, sizeof(data3_count));
        offset += sizeof(data3_count);
        for (const auto& str : data3) {
            int32_t str_size = str.size() * sizeof(char);
            std::memcpy(buffer.data() + offset, &str_size, sizeof(str_size));
            offset += sizeof(str_size);
            std::memcpy(buffer.data() + offset, str.data(), str_size);
            offset += str_size;
        }

        // 序列化 data4
        std::memcpy(buffer.data() + offset, &data4, sizeof(data4));
        offset += sizeof(data4);

        return buffer;
    }

    /* 反序列化函数 */
    void deserialize(const std::vector<char>& buffer) override
    {
        size_t offset = 0;
        // 反序列化 data1
        int32_t data1_count;
        std::memcpy(&data1_count, buffer.data() + offset, sizeof(data1_count));
        offset += sizeof(data1_count);
        data1.resize(data1_count);
        if (data1_count > 0) {
            std::memcpy(data1.data(), buffer.data() + offset, data1_count * sizeof(double));
            offset += data1_count * sizeof(double);
        }

        // 反序列化 data2
        int32_t data2_count;
        std::memcpy(&data2_count, buffer.data() + offset, sizeof(data2_count));
        offset += sizeof(data2_count);
        data2.resize(data2_count);
        if (data2_count > 0) {
            std::memcpy(data2.data(), buffer.data() + offset, data2_count * sizeof(int32_t));
            offset += data2_count * sizeof(int32_t);
        }

        // 反序列化 data3
        int32_t data3_count;
        std::memcpy(&data3_count, buffer.data() + offset, sizeof(data3_count));
        offset += sizeof(data3_count);
        data3.clear();
        data3.reserve(data3_count);
        for (int32_t i = 0; i < data3_count; ++i) {
            int32_t str_size;
            std::memcpy(&str_size, buffer.data() + offset, sizeof(str_size));
            offset += sizeof(str_size);
            std::string str(str_size / sizeof(char), '\0');
            std::memcpy(const_cast<char*>(str.data()), buffer.data() + offset, str_size);
            offset += str_size;
            data3.push_back(std::move(str));
        }

        // 反序列化 data4
        std::memcpy(&data4, buffer.data() + offset, sizeof(data4));
        offset += sizeof(data4);

    }

        /* 克隆函数 */
        test_msg* clone() const override
        {
            return new test_msg(*this);
        }

};
} // namespace dzIPC::Msg
