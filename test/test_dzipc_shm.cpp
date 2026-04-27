#include "dzIPC/shm_ser_cli_ipc.h"
#include "dzIPC/shm_pub_sub_ipc.h"
#include "ipc_srv/request_response_test/request_response_test.hpp"
#include "ipc_msg/test_msg2/test_msg.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

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
        } }, 1,
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
    dzIPC::shm::shm_cli_ipc client_ipc("request_response_test", message_, 1, true);
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
          record_response_error("response size mismatch");
          response_complete.store(true);
          return;
        }
        for (int k = 0; k < static_cast<int>(test_data.size()); k++)
        {
          if (response_->response[k] != test_data[k] + 1.0)
          {
            record_response_error("response value mismatch");
            response_complete.store(true);
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

    response_complete.store(true);
  }

  void publish_thread_function()
  {
    std::shared_ptr<TopicData> topic_msg_ = std::make_shared<dzIPC::TopicData>(
        std::make_shared<dzIPC::Msg::test_msg>());
    dzIPC::shm::shm_pub_ipc publisher(topic_msg_, "test_msg2", 1, true);
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
      if (!publisher.publish(msg))
      {
        std::cerr << "\033[31mError publishing message on topic: "
                  << "\033[0m" << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  }

  void subscribe_thread_function()
  {
    std::shared_ptr<TopicData> topic_msg_ = std::make_shared<dzIPC::TopicData>(
        std::make_shared<dzIPC::Msg::test_msg>());
    dzIPC::shm::shm_sub_ipc subscriber(topic_msg_, "test_msg2", 10, 1, true);
    subscriber.InitChannel();
    bool exit_flag = false;
    while (!exit_flag)
    {
      if (subscriber.try_get(topic_msg_))
      {
        auto received_msg = topic_msg_->topic()->msgcast<dzIPC::Msg::test_msg>();
        subscriber_count.fetch_add(1);
        for (const auto &str : received_msg->data3)
        {
          if (str == "exit")
          {
            exit_flag = true;
            subscriber_done.store(true);
            break;
          }
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  }

} // namespace

TEST(DzIpcShm, RequestResponse)
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

TEST(DzIpcShm, PubSub)
{
  subscriber_done.store(false);
  subscriber_count.store(0);

  std::thread pub_thread(publish_thread_function);
  std::thread sub_thread(subscribe_thread_function);
  pub_thread.join();
  sub_thread.join();

  ASSERT_TRUE(subscriber_done.load());
  ASSERT_GT(subscriber_count.load(), 0);
}