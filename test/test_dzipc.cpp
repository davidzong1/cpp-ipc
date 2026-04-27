#include "dzIPC/dzipc.h"
#include "ipc_msg/test_msg2/test_msg.hpp"
#include "ipc_srv/request_response_test/request_response_test.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace
{
    using namespace dzIPC;
    std::atomic<bool> socket_response_complete{false};
    std::atomic<bool> socket_response_ok{true};
    std::mutex socket_socket_response_error;
    std::string response_error;

    std::atomic<bool> socket_subscriber_done{false};
    std::atomic<int> socket_subscriber_count{0};

    void socket_record_response_error(const std::string &message)
    {
        socket_response_ok.store(false);
        std::lock_guard<std::mutex> lock(socket_socket_response_error);
        if (response_error.empty())
        {
            response_error = message;
        }
    }

    void socket_ser_thread_function()
    {
        ServerDataPtr message_ = ServerDataPtrMake<dzIPC::Srv::request_response_test_Request>(114514); // msg id is 114514,default is 0
        ServerIPC service_ipc(
            "request_response_test", message_, [](ServerDataPtr &msg)
            {
        auto req = msg->request()->msgcast<dzIPC::Srv::request_response_test_Request>();
        auto res = msg->response()->msgcast<dzIPC::Srv::request_response_test_Response>();
        res->response.resize(req->request.size());
        for (int i = 0; i < static_cast<int>(req->request.size()); i++) {
          res->response[i] = req->request[i] + 1.0;
        } },
            1, IPC_SOCKET, true);
        service_ipc.InitChannel();
        while (!socket_response_complete.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void socket_cli_thread_function()
    {
        ServerDataPtr message_ = ServerDataPtrMake<dzIPC::Srv::request_response_test_Request>(114514); // msg id is 114514,default is 0
        ClientIPC client_ipc("request_response_test", message_, 1, IPC_SOCKET, true);
        std::vector<double> test_data;
        client_ipc.InitChannel();
        int cnt = 0;
        while (cnt < 10)
        {
            std::chrono::duration<double, std::micro> elapsed_us[10];
            test_data.clear();
            for (int i = 0; i < 10; i++)
            {
                test_data.push_back(i);
                message_->request()->msgcast<dzIPC::Srv::request_response_test_Request>()->request = test_data;
                auto start = std::chrono::high_resolution_clock::now();
                while (!client_ipc.send_request(message_))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                auto end = std::chrono::high_resolution_clock::now();
                elapsed_us[i] = end - start;
                auto response_ = message_->response()->msgcast<dzIPC::Srv::request_response_test_Response>();
                if (response_->response.size() != test_data.size())
                {
                    std::cerr << "Expected response size: " << test_data.size()
                              << ", but got: " << response_->response.size() << std::endl;
                    socket_record_response_error("response size mismatch");
                    socket_response_complete.store(true);
                    return;
                }
                for (int k = 0; k < static_cast<int>(test_data.size()); k++)
                {
                    if (response_->response[k] != test_data[k] + 1.0)
                    {
                        std::cerr << "Expected response value: " << test_data[k] + 1.0
                                  << ", but got: " << response_->response[k] << std::endl;
                        socket_record_response_error("response value mismatch");
                        socket_response_complete.store(true);
                        return;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            std::cout << "Round " << cnt + 1 << " latency (us): ";
            for (int i = 0; i < 10; i++)
            {
                std::cout << static_cast<long long>(elapsed_us[i].count()) << " ";
            }
            std::cout << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            cnt++;
        }
        socket_response_complete.store(true);
    }

    void pulish_thread_function()
    {
        TopicDataPtr topic_msg_ = TopicDataPtrMake<dzIPC::Msg::test_msg>();
        PublisherIPC publisher(topic_msg_, "test_msg2", 1, IPC_SOCKET, true);
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
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    void subscribe_thread_function()
    {
        TopicDataPtr topic_msg_ = TopicDataPtrMake<dzIPC::Msg::test_msg>();
        SubscriberIPC subscriber(topic_msg_, "test_msg2", 1, 10, IPC_SOCKET, true);
        subscriber.InitChannel();
        bool exit_flag = false;
        while (!exit_flag)
        {
            if (subscriber.try_get(topic_msg_))
            {
                auto rev_msg = topic_msg_->topic()->msgcast<dzIPC::Msg::test_msg>();
                socket_subscriber_count.fetch_add(1);
                for (const auto &str : rev_msg->data3)
                {
                    if (str == "exit")
                    {
                        exit_flag = true;
                        socket_subscriber_done.store(true);
                        break;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

} // namespace

TEST(DzIpcSocket, RequestResponse)
{
    socket_response_complete.store(false);
    socket_response_ok.store(true);
    {
        std::lock_guard<std::mutex> lock(socket_socket_response_error);
        response_error.clear();
    }

    std::thread ser_thread(socket_ser_thread_function);
    std::thread cli_thread(socket_cli_thread_function);
    ser_thread.join();
    cli_thread.join();

    std::string error_snapshot;
    {
        std::lock_guard<std::mutex> lock(socket_socket_response_error);
        error_snapshot = response_error;
    }
    ASSERT_TRUE(socket_response_ok.load()) << error_snapshot;
}

TEST(DzIpcSocket, PubSub)
{
    socket_subscriber_done.store(false);
    socket_subscriber_count.store(0);

    std::thread pub_thread(pulish_thread_function);
    std::thread sub_thread(subscribe_thread_function);
    pub_thread.join();
    sub_thread.join();

    EXPECT_TRUE(socket_subscriber_done.load());
    EXPECT_GE(socket_subscriber_count.load(), 1);
}