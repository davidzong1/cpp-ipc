#pragma once
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include "ipc_msg/ipc_msg_base/ipc_msg_base.hpp"
namespace dzIPC::Msg {
class complex_message : public ipc_msg_base
{
public:
    /* 构造函数和析构函数 */
    complex_message() = default;
    ~complex_message() = default;


    /* 成员变量 */

    bool status; /* status */
    int8_t tiny_int; /* tiny_int */
    uint8_t tiny_uint; /* tiny_uint */
    int16_t small_int; /* small_int */
    uint16_t small_uint; /* small_uint */
    int32_t normal_int; /* normal_int */
    uint32_t normal_uint; /* normal_uint */
    int64_t big_int; /* big_int */
    uint64_t big_uint; /* big_uint */
    float single_precision; /* single_precision */
    double double_precision; /* double_precision */
    std::string message; /* message */
    std::vector<bool> status_array; /* status_array */
    std::vector<int8_t> tiny_int_array; /* tiny_int_array */
    std::vector<uint8_t> tiny_uint_array; /* tiny_uint_array */
    std::vector<int16_t> small_int_array; /* small_int_array */
    std::vector<uint16_t> small_uint_array; /* small_uint_array */
    std::vector<int32_t> normal_int_array; /* normal_int_array */
    std::vector<uint32_t> normal_uint_array; /* normal_uint_array */
    std::vector<int64_t> big_int_array; /* big_int_array */
    std::vector<uint64_t> big_uint_array; /* big_uint_array */
    std::vector<float> single_precision_array; /* single_precision_array */
    std::vector<double> double_precision_array; /* double_precision_array */
    std::vector<std::string> message_array; /* message_array */

    /* 序列化函数 */
    std::vector<char> serialize() const
    {
        int32_t status_size = sizeof(status);
        int32_t tiny_int_size = sizeof(tiny_int);
        int32_t tiny_uint_size = sizeof(tiny_uint);
        int32_t small_int_size = sizeof(small_int);
        int32_t small_uint_size = sizeof(small_uint);
        int32_t normal_int_size = sizeof(normal_int);
        int32_t normal_uint_size = sizeof(normal_uint);
        int32_t big_int_size = sizeof(big_int);
        int32_t big_uint_size = sizeof(big_uint);
        int32_t single_precision_size = sizeof(single_precision);
        int32_t double_precision_size = sizeof(double_precision);
        int32_t message_size = message.size() * sizeof(char);
        int32_t status_array_count = status_array.size();
        int32_t status_array_size = status_array_count * sizeof(bool);
        int32_t tiny_int_array_count = tiny_int_array.size();
        int32_t tiny_int_array_size = tiny_int_array_count * sizeof(int8_t);
        int32_t tiny_uint_array_count = tiny_uint_array.size();
        int32_t tiny_uint_array_size = tiny_uint_array_count * sizeof(uint8_t);
        int32_t small_int_array_count = small_int_array.size();
        int32_t small_int_array_size = small_int_array_count * sizeof(int16_t);
        int32_t small_uint_array_count = small_uint_array.size();
        int32_t small_uint_array_size = small_uint_array_count * sizeof(uint16_t);
        int32_t normal_int_array_count = normal_int_array.size();
        int32_t normal_int_array_size = normal_int_array_count * sizeof(int32_t);
        int32_t normal_uint_array_count = normal_uint_array.size();
        int32_t normal_uint_array_size = normal_uint_array_count * sizeof(uint32_t);
        int32_t big_int_array_count = big_int_array.size();
        int32_t big_int_array_size = big_int_array_count * sizeof(int64_t);
        int32_t big_uint_array_count = big_uint_array.size();
        int32_t big_uint_array_size = big_uint_array_count * sizeof(uint64_t);
        int32_t single_precision_array_count = single_precision_array.size();
        int32_t single_precision_array_size = single_precision_array_count * sizeof(float);
        int32_t double_precision_array_count = double_precision_array.size();
        int32_t double_precision_array_size = double_precision_array_count * sizeof(double);
        int32_t message_array_count = message_array.size();
        int32_t message_array_total_size_ = 0;
        for (const auto& str : message_array) {
            message_array_total_size_ += sizeof(int32_t) + str.size() * sizeof(char);
        }

        // 计算总缓冲区大小
        size_t total_size_ = 0;
        total_size_ += status_size;
        total_size_ += tiny_int_size;
        total_size_ += tiny_uint_size;
        total_size_ += small_int_size;
        total_size_ += small_uint_size;
        total_size_ += normal_int_size;
        total_size_ += normal_uint_size;
        total_size_ += big_int_size;
        total_size_ += big_uint_size;
        total_size_ += single_precision_size;
        total_size_ += double_precision_size;
        total_size_ += sizeof(message_size) + message_size;
        total_size_ += sizeof(status_array_count) + status_array_size;
        total_size_ += sizeof(tiny_int_array_count) + tiny_int_array_size;
        total_size_ += sizeof(tiny_uint_array_count) + tiny_uint_array_size;
        total_size_ += sizeof(small_int_array_count) + small_int_array_size;
        total_size_ += sizeof(small_uint_array_count) + small_uint_array_size;
        total_size_ += sizeof(normal_int_array_count) + normal_int_array_size;
        total_size_ += sizeof(normal_uint_array_count) + normal_uint_array_size;
        total_size_ += sizeof(big_int_array_count) + big_int_array_size;
        total_size_ += sizeof(big_uint_array_count) + big_uint_array_size;
        total_size_ += sizeof(single_precision_array_count) + single_precision_array_size;
        total_size_ += sizeof(double_precision_array_count) + double_precision_array_size;
        total_size_ += sizeof(message_array_count) + message_array_total_size_;

        // 一次性分配缓冲区
        std::vector<char> buffer(total_size_);
        total_size = total_size_;
        size_t offset = 0;

        // 序列化 status
        std::memcpy(buffer.data() + offset, &status, sizeof(status));
        offset += sizeof(status);

        // 序列化 tiny_int
        std::memcpy(buffer.data() + offset, &tiny_int, sizeof(tiny_int));
        offset += sizeof(tiny_int);

        // 序列化 tiny_uint
        std::memcpy(buffer.data() + offset, &tiny_uint, sizeof(tiny_uint));
        offset += sizeof(tiny_uint);

        // 序列化 small_int
        std::memcpy(buffer.data() + offset, &small_int, sizeof(small_int));
        offset += sizeof(small_int);

        // 序列化 small_uint
        std::memcpy(buffer.data() + offset, &small_uint, sizeof(small_uint));
        offset += sizeof(small_uint);

        // 序列化 normal_int
        std::memcpy(buffer.data() + offset, &normal_int, sizeof(normal_int));
        offset += sizeof(normal_int);

        // 序列化 normal_uint
        std::memcpy(buffer.data() + offset, &normal_uint, sizeof(normal_uint));
        offset += sizeof(normal_uint);

        // 序列化 big_int
        std::memcpy(buffer.data() + offset, &big_int, sizeof(big_int));
        offset += sizeof(big_int);

        // 序列化 big_uint
        std::memcpy(buffer.data() + offset, &big_uint, sizeof(big_uint));
        offset += sizeof(big_uint);

        // 序列化 single_precision
        std::memcpy(buffer.data() + offset, &single_precision, sizeof(single_precision));
        offset += sizeof(single_precision);

        // 序列化 double_precision
        std::memcpy(buffer.data() + offset, &double_precision, sizeof(double_precision));
        offset += sizeof(double_precision);

        // 序列化 message
        std::memcpy(buffer.data() + offset, &message_size, sizeof(message_size));
        offset += sizeof(message_size);
        std::memcpy(buffer.data() + offset, message.data(), message_size);
        offset += message_size;

        // 序列化 status_array (bool数组特殊处理)
        std::memcpy(buffer.data() + offset, &status_array_count, sizeof(status_array_count));
        offset += sizeof(status_array_count);
        for (int32_t i = 0; i < status_array_count; ++i) {
            bool val = status_array[i];
            std::memcpy(buffer.data() + offset, &val, sizeof(bool));
            offset += sizeof(bool);
        }

        // 序列化 tiny_int_array
        std::memcpy(buffer.data() + offset, &tiny_int_array_count, sizeof(tiny_int_array_count));
        offset += sizeof(tiny_int_array_count);
        if (tiny_int_array_count > 0) {
            std::memcpy(buffer.data() + offset, tiny_int_array.data(), tiny_int_array_size);
            offset += tiny_int_array_size;
        }

        // 序列化 tiny_uint_array
        std::memcpy(buffer.data() + offset, &tiny_uint_array_count, sizeof(tiny_uint_array_count));
        offset += sizeof(tiny_uint_array_count);
        if (tiny_uint_array_count > 0) {
            std::memcpy(buffer.data() + offset, tiny_uint_array.data(), tiny_uint_array_size);
            offset += tiny_uint_array_size;
        }

        // 序列化 small_int_array
        std::memcpy(buffer.data() + offset, &small_int_array_count, sizeof(small_int_array_count));
        offset += sizeof(small_int_array_count);
        if (small_int_array_count > 0) {
            std::memcpy(buffer.data() + offset, small_int_array.data(), small_int_array_size);
            offset += small_int_array_size;
        }

        // 序列化 small_uint_array
        std::memcpy(buffer.data() + offset, &small_uint_array_count, sizeof(small_uint_array_count));
        offset += sizeof(small_uint_array_count);
        if (small_uint_array_count > 0) {
            std::memcpy(buffer.data() + offset, small_uint_array.data(), small_uint_array_size);
            offset += small_uint_array_size;
        }

        // 序列化 normal_int_array
        std::memcpy(buffer.data() + offset, &normal_int_array_count, sizeof(normal_int_array_count));
        offset += sizeof(normal_int_array_count);
        if (normal_int_array_count > 0) {
            std::memcpy(buffer.data() + offset, normal_int_array.data(), normal_int_array_size);
            offset += normal_int_array_size;
        }

        // 序列化 normal_uint_array
        std::memcpy(buffer.data() + offset, &normal_uint_array_count, sizeof(normal_uint_array_count));
        offset += sizeof(normal_uint_array_count);
        if (normal_uint_array_count > 0) {
            std::memcpy(buffer.data() + offset, normal_uint_array.data(), normal_uint_array_size);
            offset += normal_uint_array_size;
        }

        // 序列化 big_int_array
        std::memcpy(buffer.data() + offset, &big_int_array_count, sizeof(big_int_array_count));
        offset += sizeof(big_int_array_count);
        if (big_int_array_count > 0) {
            std::memcpy(buffer.data() + offset, big_int_array.data(), big_int_array_size);
            offset += big_int_array_size;
        }

        // 序列化 big_uint_array
        std::memcpy(buffer.data() + offset, &big_uint_array_count, sizeof(big_uint_array_count));
        offset += sizeof(big_uint_array_count);
        if (big_uint_array_count > 0) {
            std::memcpy(buffer.data() + offset, big_uint_array.data(), big_uint_array_size);
            offset += big_uint_array_size;
        }

        // 序列化 single_precision_array
        std::memcpy(buffer.data() + offset, &single_precision_array_count, sizeof(single_precision_array_count));
        offset += sizeof(single_precision_array_count);
        if (single_precision_array_count > 0) {
            std::memcpy(buffer.data() + offset, single_precision_array.data(), single_precision_array_size);
            offset += single_precision_array_size;
        }

        // 序列化 double_precision_array
        std::memcpy(buffer.data() + offset, &double_precision_array_count, sizeof(double_precision_array_count));
        offset += sizeof(double_precision_array_count);
        if (double_precision_array_count > 0) {
            std::memcpy(buffer.data() + offset, double_precision_array.data(), double_precision_array_size);
            offset += double_precision_array_size;
        }

        // 序列化 message_array
        std::memcpy(buffer.data() + offset, &message_array_count, sizeof(message_array_count));
        offset += sizeof(message_array_count);
        for (const auto& str : message_array) {
            int32_t str_size = str.size() * sizeof(char);
            std::memcpy(buffer.data() + offset, &str_size, sizeof(str_size));
            offset += sizeof(str_size);
            std::memcpy(buffer.data() + offset, str.data(), str_size);
            offset += str_size;
        }

        return buffer;
    }

    /* 反序列化函数 */
    void deserialize(const std::vector<char>& buffer) override
    {
        size_t offset = 0;
        // 反序列化 status
        std::memcpy(&status, buffer.data() + offset, sizeof(status));
        offset += sizeof(status);

        // 反序列化 tiny_int
        std::memcpy(&tiny_int, buffer.data() + offset, sizeof(tiny_int));
        offset += sizeof(tiny_int);

        // 反序列化 tiny_uint
        std::memcpy(&tiny_uint, buffer.data() + offset, sizeof(tiny_uint));
        offset += sizeof(tiny_uint);

        // 反序列化 small_int
        std::memcpy(&small_int, buffer.data() + offset, sizeof(small_int));
        offset += sizeof(small_int);

        // 反序列化 small_uint
        std::memcpy(&small_uint, buffer.data() + offset, sizeof(small_uint));
        offset += sizeof(small_uint);

        // 反序列化 normal_int
        std::memcpy(&normal_int, buffer.data() + offset, sizeof(normal_int));
        offset += sizeof(normal_int);

        // 反序列化 normal_uint
        std::memcpy(&normal_uint, buffer.data() + offset, sizeof(normal_uint));
        offset += sizeof(normal_uint);

        // 反序列化 big_int
        std::memcpy(&big_int, buffer.data() + offset, sizeof(big_int));
        offset += sizeof(big_int);

        // 反序列化 big_uint
        std::memcpy(&big_uint, buffer.data() + offset, sizeof(big_uint));
        offset += sizeof(big_uint);

        // 反序列化 single_precision
        std::memcpy(&single_precision, buffer.data() + offset, sizeof(single_precision));
        offset += sizeof(single_precision);

        // 反序列化 double_precision
        std::memcpy(&double_precision, buffer.data() + offset, sizeof(double_precision));
        offset += sizeof(double_precision);

        // 反序列化 message
        int32_t message_size;
        std::memcpy(&message_size, buffer.data() + offset, sizeof(message_size));
        offset += sizeof(message_size);
        message.resize(message_size / sizeof(char));
        std::memcpy(const_cast<char*>(message.data()), buffer.data() + offset, message_size);
        offset += message_size;

        // 反序列化 status_array (bool数组特殊处理)
        int32_t status_array_count;
        std::memcpy(&status_array_count, buffer.data() + offset, sizeof(status_array_count));
        offset += sizeof(status_array_count);
        status_array.clear();
        status_array.reserve(status_array_count);
        for (int32_t i = 0; i < status_array_count; ++i) {
            bool val;
            std::memcpy(&val, buffer.data() + offset, sizeof(bool));
            offset += sizeof(bool);
            status_array.push_back(val);
        }

        // 反序列化 tiny_int_array
        int32_t tiny_int_array_count;
        std::memcpy(&tiny_int_array_count, buffer.data() + offset, sizeof(tiny_int_array_count));
        offset += sizeof(tiny_int_array_count);
        tiny_int_array.resize(tiny_int_array_count);
        if (tiny_int_array_count > 0) {
            std::memcpy(tiny_int_array.data(), buffer.data() + offset, tiny_int_array_count * sizeof(int8_t));
            offset += tiny_int_array_count * sizeof(int8_t);
        }

        // 反序列化 tiny_uint_array
        int32_t tiny_uint_array_count;
        std::memcpy(&tiny_uint_array_count, buffer.data() + offset, sizeof(tiny_uint_array_count));
        offset += sizeof(tiny_uint_array_count);
        tiny_uint_array.resize(tiny_uint_array_count);
        if (tiny_uint_array_count > 0) {
            std::memcpy(tiny_uint_array.data(), buffer.data() + offset, tiny_uint_array_count * sizeof(uint8_t));
            offset += tiny_uint_array_count * sizeof(uint8_t);
        }

        // 反序列化 small_int_array
        int32_t small_int_array_count;
        std::memcpy(&small_int_array_count, buffer.data() + offset, sizeof(small_int_array_count));
        offset += sizeof(small_int_array_count);
        small_int_array.resize(small_int_array_count);
        if (small_int_array_count > 0) {
            std::memcpy(small_int_array.data(), buffer.data() + offset, small_int_array_count * sizeof(int16_t));
            offset += small_int_array_count * sizeof(int16_t);
        }

        // 反序列化 small_uint_array
        int32_t small_uint_array_count;
        std::memcpy(&small_uint_array_count, buffer.data() + offset, sizeof(small_uint_array_count));
        offset += sizeof(small_uint_array_count);
        small_uint_array.resize(small_uint_array_count);
        if (small_uint_array_count > 0) {
            std::memcpy(small_uint_array.data(), buffer.data() + offset, small_uint_array_count * sizeof(uint16_t));
            offset += small_uint_array_count * sizeof(uint16_t);
        }

        // 反序列化 normal_int_array
        int32_t normal_int_array_count;
        std::memcpy(&normal_int_array_count, buffer.data() + offset, sizeof(normal_int_array_count));
        offset += sizeof(normal_int_array_count);
        normal_int_array.resize(normal_int_array_count);
        if (normal_int_array_count > 0) {
            std::memcpy(normal_int_array.data(), buffer.data() + offset, normal_int_array_count * sizeof(int32_t));
            offset += normal_int_array_count * sizeof(int32_t);
        }

        // 反序列化 normal_uint_array
        int32_t normal_uint_array_count;
        std::memcpy(&normal_uint_array_count, buffer.data() + offset, sizeof(normal_uint_array_count));
        offset += sizeof(normal_uint_array_count);
        normal_uint_array.resize(normal_uint_array_count);
        if (normal_uint_array_count > 0) {
            std::memcpy(normal_uint_array.data(), buffer.data() + offset, normal_uint_array_count * sizeof(uint32_t));
            offset += normal_uint_array_count * sizeof(uint32_t);
        }

        // 反序列化 big_int_array
        int32_t big_int_array_count;
        std::memcpy(&big_int_array_count, buffer.data() + offset, sizeof(big_int_array_count));
        offset += sizeof(big_int_array_count);
        big_int_array.resize(big_int_array_count);
        if (big_int_array_count > 0) {
            std::memcpy(big_int_array.data(), buffer.data() + offset, big_int_array_count * sizeof(int64_t));
            offset += big_int_array_count * sizeof(int64_t);
        }

        // 反序列化 big_uint_array
        int32_t big_uint_array_count;
        std::memcpy(&big_uint_array_count, buffer.data() + offset, sizeof(big_uint_array_count));
        offset += sizeof(big_uint_array_count);
        big_uint_array.resize(big_uint_array_count);
        if (big_uint_array_count > 0) {
            std::memcpy(big_uint_array.data(), buffer.data() + offset, big_uint_array_count * sizeof(uint64_t));
            offset += big_uint_array_count * sizeof(uint64_t);
        }

        // 反序列化 single_precision_array
        int32_t single_precision_array_count;
        std::memcpy(&single_precision_array_count, buffer.data() + offset, sizeof(single_precision_array_count));
        offset += sizeof(single_precision_array_count);
        single_precision_array.resize(single_precision_array_count);
        if (single_precision_array_count > 0) {
            std::memcpy(single_precision_array.data(), buffer.data() + offset, single_precision_array_count * sizeof(float));
            offset += single_precision_array_count * sizeof(float);
        }

        // 反序列化 double_precision_array
        int32_t double_precision_array_count;
        std::memcpy(&double_precision_array_count, buffer.data() + offset, sizeof(double_precision_array_count));
        offset += sizeof(double_precision_array_count);
        double_precision_array.resize(double_precision_array_count);
        if (double_precision_array_count > 0) {
            std::memcpy(double_precision_array.data(), buffer.data() + offset, double_precision_array_count * sizeof(double));
            offset += double_precision_array_count * sizeof(double);
        }

        // 反序列化 message_array
        int32_t message_array_count;
        std::memcpy(&message_array_count, buffer.data() + offset, sizeof(message_array_count));
        offset += sizeof(message_array_count);
        message_array.clear();
        message_array.reserve(message_array_count);
        for (int32_t i = 0; i < message_array_count; ++i) {
            int32_t str_size;
            std::memcpy(&str_size, buffer.data() + offset, sizeof(str_size));
            offset += sizeof(str_size);
            std::string str(str_size / sizeof(char), '\0');
            std::memcpy(const_cast<char*>(str.data()), buffer.data() + offset, str_size);
            offset += str_size;
            message_array.push_back(std::move(str));
        }

    }

        /* 克隆函数 */
        complex_message* clone() const override
        {
            return new complex_message(*this);
        }

};
} // namespace dzIPC::Msg
