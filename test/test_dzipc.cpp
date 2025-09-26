#include "dzIPC/ser_cli_ipc.h"
#include "dzIPC/pub_sub_ipc.h"
#include "ipc_srv/request_response_test/request_response_test.hpp"
#include "ipc_msg/test_msg2/test_msg.hpp"
#include <thread>
#include <iostream>
#include <chrono>
bool response_complete = false;
void ser_thread_function() {
  std::cout << "Service thread is running" << std::endl;
  dzIPC::ServiceDataPtr message_ = std::make_shared<dzIPC::ServiceData>(
      std::make_shared<dzIPC::Srv::request_response_test_Request>(),
      std::make_shared<dzIPC::Srv::request_response_test_Response>());
  dzIPC::ser_ipc service_ipc(
      "request_response_test", message_, [](dzIPC::ServiceDataPtr& msg) {
        auto req =
            std::static_pointer_cast<dzIPC::Srv::request_response_test_Request>(
                msg->request());
        auto res = std::static_pointer_cast<
            dzIPC::Srv::request_response_test_Response>(msg->response());
        res->response.resize(req->request.size());
        for (int i = 0; i < req->request.size(); i++) {
          res->response[i] = req->request[i] + 1.0;
        }
      });
  service_ipc.InitChannel();
  while (!response_complete) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  std::cerr << "Service thread is exiting" << std::endl;
}
void cli_thread_function() {
  std::cout << "Client thread is running" << std::endl;
  dzIPC::ServiceDataPtr message_ = std::make_shared<dzIPC::ServiceData>(
      std::make_shared<dzIPC::Srv::request_response_test_Request>(),
      std::make_shared<dzIPC::Srv::request_response_test_Response>());
  dzIPC::cli_ipc client_ipc("request_response_test", message_);
  std::vector<double> test_data = {1.0, 2.0, 3.0, 4.0, 5.0};
  client_ipc.InitChannel();
  auto request = std::make_shared<dzIPC::Srv::request_response_test_Request>();
  int cnt = 0;
  while (cnt < 100) {
    std::vector<std::chrono::microseconds> times;
    test_data.clear();
    for (int i = 0; i < 10; i++) {
      test_data.push_back(i);
      dzIPC::msgcast<dzIPC::Srv::request_response_test_Request>(
          message_->request())
          ->request = test_data;
      auto last_time = std::chrono::steady_clock::now();
      client_ipc.send_request(message_);
      auto now_time = std::chrono::steady_clock::now();
      times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
          now_time - last_time));
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    auto response_ = dzIPC::msgcast<dzIPC::Srv::request_response_test_Response>(
        message_->response());
    std::cout << "#### Response:";
    for (int i = 0; i < response_->response.size(); i++) {
      std::cout << " " << response_->response[i];
    }

    std::cout << std::endl << "#### Request-Response time: ";
    for (int i = 0; i < times.size(); i++) {
      std::cout << times[i].count() << " us    ";
    }
    std::cout << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cnt++;
  }
  response_complete = true;
  std::cerr << "Client thread is exiting" << std::endl;
}

void pulish_thread_function() {
  dzIPC::TopicDataPtr topic_msg_ = std::make_shared<dzIPC::TopicData>(
      std::make_shared<dzIPC::Msg::test_msg>());
  dzIPC::pub_ipc publisher("test_msg2");
  publisher.InitChannel();
  int count = 0;
  while (true) {
    auto msg = std::make_shared<dzIPC::Msg::test_msg>();
    msg->data1 = {1.1, 2.2, 3.3};
    msg->data2 = {1, 2, 3, 4};
    msg->data3 = {"hello", "world", "from", "dzIPC"};
    msg->data4 = (count % 2 == 0);
    count++;
    publisher.publish(msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

void subscribe_thread_function() {
  dzIPC::TopicDataPtr topic_msg_ = std::make_shared<dzIPC::TopicData>(
      std::make_shared<dzIPC::Msg::test_msg>());
  dzIPC::sub_ipc subscriber("test_msg2", topic_msg_, 10);
  subscriber.InitChannel();
  while (true) {
    dzIPC::MsgPtr msg;
    if (subscriber.try_get(msg)) {
      auto received_msg = std::static_pointer_cast<dzIPC::Msg::test_msg>(msg);
      std::cout << "#### Subscriber received data1: ";
      for (const auto& val : received_msg->data1) {
        std::cout << val << " ";
      }
      std::cout << "\n#### Subscriber received data2: ";
      for (const auto& val : received_msg->data2) {
        std::cout << val << " ";
      }
      std::cout << "\n#### Subscriber received data3: ";
      for (const auto& str : received_msg->data3) {
        std::cout << str << " ";
      }
      std::cout << "\n#### Subscriber received data4: "
                << (received_msg->data4 ? "true" : "false") << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

int main() {
  std::thread ser_thread(ser_thread_function);
  std::thread cli_thread(cli_thread_function);
  std::thread pub_thread(pulish_thread_function);
  std::thread sub_thread(subscribe_thread_function);
  ser_thread.join();
  cli_thread.join();
  pub_thread.join();
  sub_thread.join();
  return 0;
}