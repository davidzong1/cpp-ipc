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

    bool status;    /* status */
    int8_t tiny_int;    /* tiny_int */
    uint8_t tiny_uint;    /* tiny_uint */
    int16_t small_int;    /* small_int */
    uint16_t small_uint;    /* small_uint */
    int32_t normal_int;    /* normal_int */
    uint32_t normal_uint;    /* normal_uint */
    int64_t big_int;    /* big_int */
    uint64_t big_uint;    /* big_uint */
    float single_precision;    /* single_precision */
    double double_precision;    /* double_precision */
    std::string message;    /* message */
    std::vector<bool> status_array;    /* status_array */
    std::vector<int8_t> tiny_int_array;    /* tiny_int_array */
    std::vector<uint8_t> tiny_uint_array;    /* tiny_uint_array */
    std::vector<int16_t> small_int_array;    /* small_int_array */
    std::vector<uint16_t> small_uint_array;    /* small_uint_array */
    std::vector<int32_t> normal_int_array;    /* normal_int_array */
    std::vector<uint32_t> normal_uint_array;    /* normal_uint_array */
    std::vector<int64_t> big_int_array;    /* big_int_array */
    std::vector<uint64_t> big_uint_array;    /* big_uint_array */
    std::vector<float> single_precision_array;    /* single_precision_array */
    std::vector<double> double_precision_array;    /* double_precision_array */
    std::vector<std::string> message_array;    /* message_array */

        /* 序列化函数 */
        ipc::buffer serialize() override
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
        int32_t message_size = message.size() ;
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
            message_array_total_size_ += sizeof(int32_t) + str.size() ;
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
        ipc::buffer buffer = std::move(this->serialize_data_cut(total_size_));
        uint32_t offset = 0;
        uint16_t page = 1;

        // 序列化 status
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&status), page, offset, sizeof(status));

        // 序列化 tiny_int
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&tiny_int), page, offset, sizeof(tiny_int));

        // 序列化 tiny_uint
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&tiny_uint), page, offset, sizeof(tiny_uint));

        // 序列化 small_int
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&small_int), page, offset, sizeof(small_int));

        // 序列化 small_uint
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&small_uint), page, offset, sizeof(small_uint));

        // 序列化 normal_int
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&normal_int), page, offset, sizeof(normal_int));

        // 序列化 normal_uint
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&normal_uint), page, offset, sizeof(normal_uint));

        // 序列化 big_int
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&big_int), page, offset, sizeof(big_int));

        // 序列化 big_uint
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&big_uint), page, offset, sizeof(big_uint));

        // 序列化 single_precision
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&single_precision), page, offset, sizeof(single_precision));

        // 序列化 double_precision
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&double_precision), page, offset, sizeof(double_precision));

        // 序列化 message
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&message_size), page, offset, sizeof(message_size));          this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(message.data()), page, offset, message_size);

        // 序列化 status_array (bool数组特殊处理)
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&status_array_count), page, offset, sizeof(status_array_count));
        std::vector<uint8_t> bool_byte(status_array_count, 0);
        for (int32_t i = 0; i < status_array_count; ++i) {
            bool_byte[i] = status_array[i] ? 1 : 0;
        }
        if (status_array_count > 0) {
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(bool_byte.data()), page, offset, status_array_count);
        }

        // 序列化 tiny_int_array
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&tiny_int_array_count), page, offset, sizeof(tiny_int_array_count));
        if (tiny_int_array_count > 0) {
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(tiny_int_array.data()), page, offset, tiny_int_array_size);
        }

        // 序列化 tiny_uint_array
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&tiny_uint_array_count), page, offset, sizeof(tiny_uint_array_count));
        if (tiny_uint_array_count > 0) {
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(tiny_uint_array.data()), page, offset, tiny_uint_array_size);
        }

        // 序列化 small_int_array
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&small_int_array_count), page, offset, sizeof(small_int_array_count));
        if (small_int_array_count > 0) {
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(small_int_array.data()), page, offset, small_int_array_size);
        }

        // 序列化 small_uint_array
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&small_uint_array_count), page, offset, sizeof(small_uint_array_count));
        if (small_uint_array_count > 0) {
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(small_uint_array.data()), page, offset, small_uint_array_size);
        }

        // 序列化 normal_int_array
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&normal_int_array_count), page, offset, sizeof(normal_int_array_count));
        if (normal_int_array_count > 0) {
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(normal_int_array.data()), page, offset, normal_int_array_size);
        }

        // 序列化 normal_uint_array
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&normal_uint_array_count), page, offset, sizeof(normal_uint_array_count));
        if (normal_uint_array_count > 0) {
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(normal_uint_array.data()), page, offset, normal_uint_array_size);
        }

        // 序列化 big_int_array
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&big_int_array_count), page, offset, sizeof(big_int_array_count));
        if (big_int_array_count > 0) {
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(big_int_array.data()), page, offset, big_int_array_size);
        }

        // 序列化 big_uint_array
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&big_uint_array_count), page, offset, sizeof(big_uint_array_count));
        if (big_uint_array_count > 0) {
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(big_uint_array.data()), page, offset, big_uint_array_size);
        }

        // 序列化 single_precision_array
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&single_precision_array_count), page, offset, sizeof(single_precision_array_count));
        if (single_precision_array_count > 0) {
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(single_precision_array.data()), page, offset, single_precision_array_size);
        }

        // 序列化 double_precision_array
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&double_precision_array_count), page, offset, sizeof(double_precision_array_count));
        if (double_precision_array_count > 0) {
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(double_precision_array.data()), page, offset, double_precision_array_size);
        }

        // 序列化 message_array
        this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&message_array_count), page, offset, sizeof(message_array_count));
        for (const auto& str : message_array) {
            int32_t str_size = str.size();
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(&str_size), page, offset, sizeof(str_size));
            this->adapt_memcpy_tos(static_cast<uint8_t *>(buffer.data()), reinterpret_cast<const uint8_t *>(str.data()), page, offset, str_size);
        }

        this->add_tail_msg(static_cast<uint8_t *>(buffer.data()) + offset, page);
        return buffer;
    }

    /* 反序列化函数 */
    void deserialize(const ipc::buffer& buffer) override
    {
        uint32_t offset = 0;
        deserialize_data_cut(buffer.size());
        // 反序列化 status
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&status), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(status));

        // 反序列化 tiny_int
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&tiny_int), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(tiny_int));

        // 反序列化 tiny_uint
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&tiny_uint), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(tiny_uint));

        // 反序列化 small_int
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&small_int), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(small_int));

        // 反序列化 small_uint
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&small_uint), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(small_uint));

        // 反序列化 normal_int
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&normal_int), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(normal_int));

        // 反序列化 normal_uint
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&normal_uint), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(normal_uint));

        // 反序列化 big_int
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&big_int), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(big_int));

        // 反序列化 big_uint
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&big_uint), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(big_uint));

        // 反序列化 single_precision
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&single_precision), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(single_precision));

        // 反序列化 double_precision
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&double_precision), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(double_precision));

        // 反序列化 message
        int32_t message_size;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&message_size), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(message_size));
        message.resize(message_size );
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(message.data()), static_cast<const uint8_t *>(buffer.data()), offset, message_size);

        // 反序列化 status_array (bool数组特殊处理)
        int32_t status_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&status_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(status_array_count));
        status_array.clear();
        status_array.resize(status_array_count);
        if (status_array_count > 0) {
            std::vector<uint8_t> bool_byte(status_array_count, 0);
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(bool_byte.data()), static_cast<const uint8_t *>(buffer.data()), offset, status_array_count);
            for (int32_t i = 0; i < status_array_count; ++i) {
                status_array[i] = (bool_byte[i] != 0);
            }
        }

        // 反序列化 tiny_int_array
        int32_t tiny_int_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&tiny_int_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(tiny_int_array_count));
        tiny_int_array.resize(tiny_int_array_count);
        if (tiny_int_array_count > 0) {
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(tiny_int_array.data()), static_cast<const uint8_t *>(buffer.data()), offset, tiny_int_array_count * sizeof(int8_t));
        }

        // 反序列化 tiny_uint_array
        int32_t tiny_uint_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&tiny_uint_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(tiny_uint_array_count));
        tiny_uint_array.resize(tiny_uint_array_count);
        if (tiny_uint_array_count > 0) {
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(tiny_uint_array.data()), static_cast<const uint8_t *>(buffer.data()), offset, tiny_uint_array_count * sizeof(uint8_t));
        }

        // 反序列化 small_int_array
        int32_t small_int_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&small_int_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(small_int_array_count));
        small_int_array.resize(small_int_array_count);
        if (small_int_array_count > 0) {
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(small_int_array.data()), static_cast<const uint8_t *>(buffer.data()), offset, small_int_array_count * sizeof(int16_t));
        }

        // 反序列化 small_uint_array
        int32_t small_uint_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&small_uint_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(small_uint_array_count));
        small_uint_array.resize(small_uint_array_count);
        if (small_uint_array_count > 0) {
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(small_uint_array.data()), static_cast<const uint8_t *>(buffer.data()), offset, small_uint_array_count * sizeof(uint16_t));
        }

        // 反序列化 normal_int_array
        int32_t normal_int_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&normal_int_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(normal_int_array_count));
        normal_int_array.resize(normal_int_array_count);
        if (normal_int_array_count > 0) {
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(normal_int_array.data()), static_cast<const uint8_t *>(buffer.data()), offset, normal_int_array_count * sizeof(int32_t));
        }

        // 反序列化 normal_uint_array
        int32_t normal_uint_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&normal_uint_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(normal_uint_array_count));
        normal_uint_array.resize(normal_uint_array_count);
        if (normal_uint_array_count > 0) {
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(normal_uint_array.data()), static_cast<const uint8_t *>(buffer.data()), offset, normal_uint_array_count * sizeof(uint32_t));
        }

        // 反序列化 big_int_array
        int32_t big_int_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&big_int_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(big_int_array_count));
        big_int_array.resize(big_int_array_count);
        if (big_int_array_count > 0) {
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(big_int_array.data()), static_cast<const uint8_t *>(buffer.data()), offset, big_int_array_count * sizeof(int64_t));
        }

        // 反序列化 big_uint_array
        int32_t big_uint_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&big_uint_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(big_uint_array_count));
        big_uint_array.resize(big_uint_array_count);
        if (big_uint_array_count > 0) {
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(big_uint_array.data()), static_cast<const uint8_t *>(buffer.data()), offset, big_uint_array_count * sizeof(uint64_t));
        }

        // 反序列化 single_precision_array
        int32_t single_precision_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&single_precision_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(single_precision_array_count));
        single_precision_array.resize(single_precision_array_count);
        if (single_precision_array_count > 0) {
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(single_precision_array.data()), static_cast<const uint8_t *>(buffer.data()), offset, single_precision_array_count * sizeof(float));
        }

        // 反序列化 double_precision_array
        int32_t double_precision_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&double_precision_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(double_precision_array_count));
        double_precision_array.resize(double_precision_array_count);
        if (double_precision_array_count > 0) {
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(double_precision_array.data()), static_cast<const uint8_t *>(buffer.data()), offset, double_precision_array_count * sizeof(double));
        }

        // 反序列化 message_array
        int32_t message_array_count;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&message_array_count), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(message_array_count));
        message_array.clear();
        message_array.reserve(message_array_count);
        for (int32_t i = 0; i < message_array_count; ++i) {
            int32_t str_size;
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&str_size), static_cast<const uint8_t *>(buffer.data()), offset, sizeof(str_size));
            std::string str(str_size, '\0');
            this->adapt_memcpy_tods(reinterpret_cast<uint8_t*>(str.data()), static_cast<const uint8_t *>(buffer.data()), offset, str_size);
            message_array.emplace_back(std::move(str));
        }

    }

        /* 克隆函数 */
        complex_message* clone() const override
        {
            return new complex_message(*this);
        }

};
} // namespace dzIPC::Msg
