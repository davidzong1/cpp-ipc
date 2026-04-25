#include "ipc_msg/ipc_msg_base/ipc_msg_base.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <cstring>
class ipc_pub_sub_id_init_msg : public ipc_msg_base
{
public:
    /* 构造函数和析构函数 */
    ipc_pub_sub_id_init_msg() = default;
    ~ipc_pub_sub_id_init_msg() = default;
    bool host_flag{false};
    bool cli_flag{false};
    bool run_status{true};
    bool check_host(ipc::buffer &raw_data)
    {
        uint8_t *data_ptr = reinterpret_cast<uint8_t *>(raw_data.data());
        return data_ptr[0] == 1;
    }
    bool check_cli(ipc::buffer &raw_data)
    {
        uint8_t *data_ptr = reinterpret_cast<uint8_t *>(raw_data.data());
        return data_ptr[1] == 1;
    }
    bool check_run_status(ipc::buffer &raw_data)
    {
        uint8_t *data_ptr = reinterpret_cast<uint8_t *>(raw_data.data());
        return data_ptr[2] == 1;
    }
    /* 序列化函数 */
    ipc::buffer serialize() override
    {
        size_t total_size_ = sizeof(bool) * 3;
        ipc::buffer data = std::move(this->serialize_data_cut(total_size_));
        uint32_t offset = 0;
        uint16_t page = 1;
        uint8_t host_flag_cache = host_flag ? 1 : 0;
        uint8_t cli_flag_cache = cli_flag ? 1 : 0;
        uint8_t run_status_cache = run_status ? 1 : 0;
        this->adapt_memcpy_tos(static_cast<uint8_t *>(data.data()), reinterpret_cast<const uint8_t *>(&host_flag_cache), page, offset, 1);
        this->adapt_memcpy_tos(static_cast<uint8_t *>(data.data()), reinterpret_cast<const uint8_t *>(&cli_flag_cache), page, offset, 1);
        this->adapt_memcpy_tos(static_cast<uint8_t *>(data.data()), reinterpret_cast<const uint8_t *>(&run_status_cache), page, offset, 1);
        this->add_tail_msg(static_cast<uint8_t *>(data.data()) + offset, page);
        return data;
    }
    /* 反序列化函数 */
    void deserialize(const ipc::buffer &buffer) override
    {
        uint32_t offset = 0;
        uint8_t host_flag_cache;
        uint8_t cli_flag_cache;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&host_flag_cache), static_cast<const uint8_t *>(buffer.data()), offset, 1);
        host_flag = host_flag_cache == 1 ? true : false;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&cli_flag_cache), static_cast<const uint8_t *>(buffer.data()), offset, 1);
        cli_flag = cli_flag_cache == 1 ? true : false;
        uint8_t run_status_cache;
        this->adapt_memcpy_tods(reinterpret_cast<uint8_t *>(&run_status_cache), static_cast<const uint8_t *>(buffer.data()), offset, 1);
        run_status = run_status_cache == 1 ? true : false;
    }
    ipc_pub_sub_id_init_msg *clone() const override { return new ipc_pub_sub_id_init_msg(*this); }
};