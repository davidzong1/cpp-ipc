#pragma once
#include "ipc_msg/ipc_msg_base/ipc_msg_base.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

class ipc_rtps_nack_msg : public ipc_msg_base
{
public:
    static constexpr uint32_t kRtpsNackMsgId = 0x445A4E4B; // "DZNK"
    static constexpr std::size_t kMaxMissingPages = 256;

    ipc_rtps_nack_msg()
    {
        set_msg_id(kRtpsNackMsgId);
    }

    ~ipc_rtps_nack_msg() = default;

    uint16_t page_cnt{0};
    uint32_t total_size{0};
    uint32_t data_msg_id{0};
    uint32_t receiver_id{0};
    std::vector<uint16_t> missing_pages;

    ipc::buffer serialize() override
    {
        const uint16_t miss_cnt = static_cast<uint16_t>(std::min(missing_pages.size(), kMaxMissingPages));
        const uint32_t total_size_ = sizeof(page_cnt) + sizeof(total_size) + sizeof(data_msg_id) +
                                     sizeof(receiver_id) + sizeof(miss_cnt) +
                                     static_cast<uint32_t>(miss_cnt * sizeof(uint16_t));

        ipc::buffer data = serialize_data_cut(total_size_);
        uint32_t offset = 0;
        uint16_t page = 1;

        adapt_memcpy_tos(static_cast<uint8_t *>(data.data()), reinterpret_cast<const uint8_t *>(&page_cnt), page, offset, sizeof(page_cnt));
        adapt_memcpy_tos(static_cast<uint8_t *>(data.data()), reinterpret_cast<const uint8_t *>(&total_size), page, offset, sizeof(total_size));
        adapt_memcpy_tos(static_cast<uint8_t *>(data.data()), reinterpret_cast<const uint8_t *>(&data_msg_id), page, offset, sizeof(data_msg_id));
        adapt_memcpy_tos(static_cast<uint8_t *>(data.data()), reinterpret_cast<const uint8_t *>(&receiver_id), page, offset, sizeof(receiver_id));
        adapt_memcpy_tos(static_cast<uint8_t *>(data.data()), reinterpret_cast<const uint8_t *>(&miss_cnt), page, offset, sizeof(miss_cnt));

        for (uint16_t i = 0; i < miss_cnt; ++i)
        {
            const uint16_t page_idx = missing_pages[i];
            adapt_memcpy_tos(static_cast<uint8_t *>(data.data()), reinterpret_cast<const uint8_t *>(&page_idx), page, offset, sizeof(page_idx));
        }

        add_tail_msg(static_cast<uint8_t *>(data.data()) + offset, page);
        return data;
    }

    void deserialize(const ipc::buffer &buffer) override
    {
        uint32_t offset = 0;
        uint16_t miss_cnt = 0;

        adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&page_cnt), static_cast<const uint8_t *>(buffer.data()) + offset, offset, sizeof(page_cnt));
        adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&total_size), static_cast<const uint8_t *>(buffer.data()) + offset, offset, sizeof(total_size));
        adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&data_msg_id), static_cast<const uint8_t *>(buffer.data()) + offset, offset, sizeof(data_msg_id));
        adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&receiver_id), static_cast<const uint8_t *>(buffer.data()) + offset, offset, sizeof(receiver_id));
        adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&miss_cnt), static_cast<const uint8_t *>(buffer.data()) + offset, offset, sizeof(miss_cnt));

        const uint16_t clamp_cnt = static_cast<uint16_t>(std::min<std::size_t>(miss_cnt, kMaxMissingPages));
        missing_pages.clear();
        missing_pages.reserve(clamp_cnt);

        for (uint16_t i = 0; i < clamp_cnt; ++i)
        {
            uint16_t page_idx = 0;
            adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&page_idx), static_cast<const uint8_t *>(buffer.data()) + offset, offset, sizeof(page_idx));
            missing_pages.push_back(page_idx);
        }
    }

    ipc_rtps_nack_msg *clone() const override { return new ipc_rtps_nack_msg(*this); }
};
