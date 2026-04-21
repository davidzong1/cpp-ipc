#include "dzIPC/socket_pub_sub_ipc.h"
#include "ipc_srv/request_response_test/request_response_test.hpp"
#include "ipc_msg/test_msg2/test_msg.hpp"
#include <thread>
#include <iostream>
#include <chrono>
void pulish_thread_function()
{
    dzIPC::socket::TopicDataPtr topic_msg_ = std::make_shared<dzIPC::TopicData>(
        std::make_shared<dzIPC::Msg::test_msg>());
    dzIPC::socket::socket_pub_ipc publisher("test_msg2", 1, true);
    publisher.InitChannel();
    int count = 0;
    bool exit_flag = false;
    while (!exit_flag)
    {
        auto msg = std::make_shared<dzIPC::Msg::test_msg>();
        msg->data1 = {1.1, 2.2, 3.3};
        msg->data2 = {1, 2, 3, 4};
        msg->data3 = {"hello", "world", "from", "dzIPC"};
        if (count >= 10)
        {
            msg->data3.push_back("exit");
            exit_flag = true;
        }
        msg->data4 = (count % 2 == 0);
        count++;
        publisher.publish(msg);
        std::cerr << "Publisher published message " << count << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cerr << "Publisher thread is exiting" << std::endl;
}

void subscribe_thread_function()
{
    dzIPC::socket::TopicDataPtr topic_msg_ = std::make_shared<dzIPC::TopicData>(
        std::make_shared<dzIPC::Msg::test_msg>());
    dzIPC::socket::socket_sub_ipc subscriber(topic_msg_, "test_msg2", 1, true);
    subscriber.InitChannel();
    bool exit_flag = false;
    while (!exit_flag)
    {
        dzIPC::socket::MsgPtr<> msg;
        if (subscriber.try_get(msg))
        {
            auto received_msg = std::static_pointer_cast<dzIPC::Msg::test_msg>(msg);
            std::cout << "#### Subscriber received data1: ";
            for (const auto &val : received_msg->data1)
            {
                std::cout << val << " ";
            }
            std::cout << "\n#### Subscriber received data2: ";
            for (const auto &val : received_msg->data2)
            {
                std::cout << val << " ";
            }
            std::cout << "\n#### Subscriber received data3: ";
            for (const auto &str : received_msg->data3)
            {
                std::cout << str << " ";
                if (str == "exit")
                {
                    exit_flag = true;
                }
            }
            std::cout << "\n#### Subscriber received data4: "
                      << (received_msg->data4 ? "true" : "false") << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cerr << "Subscriber thread is exiting" << std::endl;
}
int main()
{
    std::thread pub_thread(pulish_thread_function);
    std::thread sub_thread(subscribe_thread_function);
    pub_thread.join();
    sub_thread.join();
    return 0;
}