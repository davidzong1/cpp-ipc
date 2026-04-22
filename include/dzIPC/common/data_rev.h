#pragma once
#include <string>
#include <cstdint>
#include "dzIPC/common/srv_data.h"
#include "dzIPC/common/topic_data.h"
#include "libipc/udp.h"
namespace dzIPC
{
    namespace socket
    {
        bool chunk_rev_topic(std::shared_ptr<ipc::socket::UDPNode> &node, std::shared_ptr<TopicData> &rev_msg, uint64_t tm);
        bool chunk_rev_server(std::shared_ptr<ipc::socket::UDPNode> &node, std::shared_ptr<ServiceData> &rev_msg, uint64_t tm, bool ser_or_cli);
        bool chunk_send(std::shared_ptr<ipc::socket::UDPNode> &node, ipc::buffer &publish_data);
    }
}