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
    /* 序列化函数 */
    std::vector<char> serialize() const override
    {
        std::vector<char> data(sizeof(size_t) + sizeof(bool));
        *reinterpret_cast<size_t *>(data.data()) = dz_ipc_msg_id;
        *reinterpret_cast<bool *>(data.data() + sizeof(size_t)) = host_flag;
        return data;
    }
    /* 反序列化函数 */
    void deserialize(const std::vector<char> &buffer) override
    {
        size_t offset = sizeof(size_t);
        std::memcpy(&host_flag, buffer.data() + offset, sizeof(bool));
    }
    ipc_pub_sub_id_init_msg *clone() const override { return new ipc_pub_sub_id_init_msg(*this); }
};