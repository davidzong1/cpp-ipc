#include "dzIPC/shm_pub_sub_ipc.h"
#include "dzIPC/shm_ser_cli_ipc.h"
#include "ipc_msg/test_msg2/test_msg.hpp"
#include "ipc_srv/request_response_test/request_response_test.hpp"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
namespace
{
    std::atomic<bool> response_complete{false};
    std::atomic<bool> response_ok{true};
    std::mutex response_mutex;
    std::string response_error;

    std::atomic<bool> subscriber_done{false};
    std::atomic<int> subscriber_count{0};
    using namespace dzIPC;
    void record_response_error(const std::string &message)
    {
        response_ok.store(false);
        std::lock_guard<std::mutex> lock(response_mutex);
        if (response_error.empty())
        {
            response_error = message;
        }
    }

    void ser_thread_function()
    {
        std::shared_ptr<ServiceData> message_ = std::make_shared<dzIPC::ServiceData>(
            std::make_shared<dzIPC::Srv::request_response_test_Request>(),
            std::make_shared<dzIPC::Srv::request_response_test_Response>());
        dzIPC::shm::shm_ser_ipc service_ipc(
            "request_response_test", message_, [](std::shared_ptr<ServiceData> &msg)
            {
        auto req =
            std::static_pointer_cast<dzIPC::Srv::request_response_test_Request>(
                msg->request());
        auto res = std::static_pointer_cast<
            dzIPC::Srv::request_response_test_Response>(msg->response());
        res->response.resize(req->request.size());
        for (int i = 0; i < static_cast<int>(req->request.size()); i++) {
          res->response[i] = req->request[i] + 1.0;
        } },
            true);
        service_ipc.InitChannel();
        while (!response_complete.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void cli_thread_function()
    {
        std::shared_ptr<ServiceData> message_ = std::make_shared<dzIPC::ServiceData>(
            std::make_shared<dzIPC::Srv::request_response_test_Request>(),
            std::make_shared<dzIPC::Srv::request_response_test_Response>());
        dzIPC::shm::shm_cli_ipc client_ipc("request_response_test", message_, true);
        std::vector<double> test_data;
        client_ipc.InitChannel();
        int cnt = 0;

        test_data.clear();
        for (int i = 0; i < 5000; i++)
        {
            test_data.push_back(i);
        }
        message_->request()->msgcast<dzIPC::Srv::request_response_test_Request>()->request = test_data;
        auto start = std::chrono::high_resolution_clock::now();
        while (!client_ipc.send_request(message_))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> elapsed_us = end - start;
        std::cout << "Time taken to send request: " << elapsed_us.count() << " us" << std::endl;
        auto response_ = message_->response()->msgcast<dzIPC::Srv::request_response_test_Response>();
        if (response_->response.size() != test_data.size())
        {
            std::cerr << "Expected response size: " << test_data.size()
                      << ", but got: " << response_->response.size() << std::endl;
            record_response_error("response size mismatch");
            response_complete.store(true);
            return;
        }
        for (int k = 0; k < static_cast<int>(test_data.size()); k++)
        {
            if (response_->response[k] != test_data[k] + 1.0)
            {
                std::cerr << "Expected response value: " << test_data[k] + 1.0
                          << ", but got: " << response_->response[k] << std::endl;
                record_response_error("response value mismatch");
                response_complete.store(true);
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        cnt++;

        response_complete.store(true);
    }

    TEST(DzIpcshm, RequestResponse)
    {
        response_complete.store(false);
        response_ok.store(true);
        {
            std::lock_guard<std::mutex> lock(response_mutex);
            response_error.clear();
        }

        std::thread ser_thread(ser_thread_function);
        std::thread cli_thread(cli_thread_function);
        ser_thread.join();
        cli_thread.join();

        std::string error_snapshot;
        {
            std::lock_guard<std::mutex> lock(response_mutex);
            error_snapshot = response_error;
        }
        ASSERT_TRUE(response_ok.load()) << error_snapshot;
    }
}