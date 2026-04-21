#include "libipc/udp.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{

    bool connect_with_retry(ipc::socket::UDPNode &node, const char *label, int retries = 5)
    {
        for (int i = 0; i < retries; ++i)
        {
            if (node.connect())
            {
                return true;
            }
            std::cerr << "[" << label << "] connect failed, retry " << (i + 1) << "/" << retries << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        return false;
    }

} // namespace

int main()
{
    const char *group_ip = "239.255.194.45"; // local multicast range
    const uint16_t port = 13222;

    ipc::socket::UDPNode receiver("recv", group_ip, port);
    ipc::socket::UDPNode sender("send", group_ip, port);

    if (!connect_with_retry(receiver, "receiver"))
    {
        std::cerr << "receiver connect failed" << std::endl;
        return 1;
    }
    if (!connect_with_retry(sender, "sender"))
    {
        std::cerr << "sender connect failed" << std::endl;
        return 1;
    }

    const std::string message = "udp test message";
    std::vector<uint8_t> payload(message.begin(), message.end());
    ipc::buffer send_buf(payload.data(), payload.size());

    if (!sender.send(send_buf))
    {
        std::cerr << "send failed" << std::endl;
        return 1;
    }
    auto start_time = std::chrono::steady_clock::now();
    ipc::buffer recv_buf = receiver.receive(10);
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    if (recv_buf.empty())
    {
        std::cerr << "receive timeout" << std::endl;
        return 1;
    }

    const char *recv_data = static_cast<const char *>(recv_buf.data());
    std::string received(recv_data, recv_data + recv_buf.size());
    std::cout << "received: " << received << std::endl;
    std::cout << "round-trip time: " << duration.count() << " us" << std::endl;
    receiver.close();
    sender.close();
    return 0;
}