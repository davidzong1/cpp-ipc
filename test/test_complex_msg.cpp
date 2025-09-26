#include <iostream>
#include <vector>
#include <cassert>
#include <random>
#include <chrono>
#include "ipc_msg/test_messages/complex_message.hpp"
using namespace dzIPC::Msg;
class ComplexMessageTest {
 private:
  std::random_device rd;
  std::mt19937 gen;

 public:
  ComplexMessageTest() : gen(rd()) {}

  // 生成随机测试数据
  complex_message generateRandomMessage() {
    complex_message msg;

    std::uniform_int_distribution<> bool_dis(0, 1);
    std::uniform_int_distribution<int8_t> int8_dis(-128, 127);
    std::uniform_int_distribution<uint8_t> uint8_dis(0, 255);
    std::uniform_int_distribution<int16_t> int16_dis(-32768, 32767);
    std::uniform_int_distribution<uint16_t> uint16_dis(0, 65535);
    std::uniform_int_distribution<int32_t> int32_dis(-2147483648, 2147483647);
    std::uniform_int_distribution<uint32_t> uint32_dis(0, 4294967295U);
    std::uniform_int_distribution<int64_t> int64_dis(-9223372036854775807LL,
                                                     9223372036854775807LL);
    std::uniform_int_distribution<uint64_t> uint64_dis(0,
                                                       18446744073709551615ULL);
    std::uniform_real_distribution<float> float_dis(-1000.0f, 1000.0f);
    std::uniform_real_distribution<double> double_dis(-1000.0, 1000.0);
    std::uniform_int_distribution<> array_size_dis(0, 10);

    // 基本类型
    msg.status = bool_dis(gen);
    msg.tiny_int = int8_dis(gen);
    msg.tiny_uint = uint8_dis(gen);
    msg.small_int = int16_dis(gen);
    msg.small_uint = uint16_dis(gen);
    msg.normal_int = int32_dis(gen);
    msg.normal_uint = uint32_dis(gen);
    msg.big_int = int64_dis(gen);
    msg.big_uint = uint64_dis(gen);
    msg.single_precision = float_dis(gen);
    msg.double_precision = double_dis(gen);
    msg.message = "Test message " + std::to_string(gen());

    // 数组类型
    int array_size = array_size_dis(gen);

    msg.status_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.status_array.push_back(bool_dis(gen));
    }

    msg.tiny_int_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.tiny_int_array.push_back(int8_dis(gen));
    }

    msg.tiny_uint_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.tiny_uint_array.push_back(uint8_dis(gen));
    }

    msg.small_int_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.small_int_array.push_back(int16_dis(gen));
    }

    msg.small_uint_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.small_uint_array.push_back(uint16_dis(gen));
    }

    msg.normal_int_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.normal_int_array.push_back(int32_dis(gen));
    }

    msg.normal_uint_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.normal_uint_array.push_back(uint32_dis(gen));
    }

    msg.big_int_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.big_int_array.push_back(int64_dis(gen));
    }

    msg.big_uint_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.big_uint_array.push_back(uint64_dis(gen));
    }

    msg.single_precision_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.single_precision_array.push_back(float_dis(gen));
    }

    msg.double_precision_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.double_precision_array.push_back(double_dis(gen));
    }

    msg.message_array.clear();
    for (int i = 0; i < array_size; ++i) {
      msg.message_array.push_back("String " + std::to_string(i) + "_" +
                                  std::to_string(gen() % 1000));
    }

    return msg;
  }

  // 验证两个消息是否相等
  bool verifyMessages(const complex_message &original,
                      const complex_message &deserialized) {
    if (original.status != deserialized.status) return false;
    if (original.tiny_int != deserialized.tiny_int) return false;
    if (original.tiny_uint != deserialized.tiny_uint) return false;
    if (original.small_int != deserialized.small_int) return false;
    if (original.small_uint != deserialized.small_uint) return false;
    if (original.normal_int != deserialized.normal_int) return false;
    if (original.normal_uint != deserialized.normal_uint) return false;
    if (original.big_int != deserialized.big_int) return false;
    if (original.big_uint != deserialized.big_uint) return false;
    if (std::abs(original.single_precision - deserialized.single_precision) >
        1e-6f)
      return false;
    if (std::abs(original.double_precision - deserialized.double_precision) >
        1e-12)
      return false;
    if (original.message != deserialized.message) return false;

    if (original.status_array != deserialized.status_array) return false;
    if (original.tiny_int_array != deserialized.tiny_int_array) return false;
    if (original.tiny_uint_array != deserialized.tiny_uint_array) return false;
    if (original.small_int_array != deserialized.small_int_array) return false;
    if (original.small_uint_array != deserialized.small_uint_array)
      return false;
    if (original.normal_int_array != deserialized.normal_int_array)
      return false;
    if (original.normal_uint_array != deserialized.normal_uint_array)
      return false;
    if (original.big_int_array != deserialized.big_int_array) return false;
    if (original.big_uint_array != deserialized.big_uint_array) return false;
    if (original.message_array != deserialized.message_array) return false;

    // 浮点数组需要特殊处理
    if (original.single_precision_array.size() !=
        deserialized.single_precision_array.size())
      return false;
    for (size_t i = 0; i < original.single_precision_array.size(); ++i) {
      if (std::abs(original.single_precision_array[i] -
                   deserialized.single_precision_array[i]) > 1e-6f)
        return false;
    }

    if (original.double_precision_array.size() !=
        deserialized.double_precision_array.size())
      return false;
    for (size_t i = 0; i < original.double_precision_array.size(); ++i) {
      if (std::abs(original.double_precision_array[i] -
                   deserialized.double_precision_array[i]) > 1e-12)
        return false;
    }

    return true;
  }

  // 性能测试
  void performanceTest(int iterations = 1000) {
    std::cout << "\n=== 性能测试 ===" << std::endl;
    std::cout << "测试迭代次数: " << iterations << std::endl;

    auto msg = generateRandomMessage();

    // 序列化性能测试
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<char> serialized_data;
    for (int i = 0; i < iterations; ++i) {
      serialized_data = msg.serialize();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto serialize_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "序列化耗时: " << serialize_time.count() << " 微秒"
              << std::endl;
    std::cout << "平均每次序列化: " << serialize_time.count() / iterations
              << " 微秒" << std::endl;
    std::cout << "序列化后大小: " << serialized_data.size() << " 字节"
              << std::endl;

    // 反序列化性能测试
    start = std::chrono::high_resolution_clock::now();
    complex_message deserialized;
    for (int i = 0; i < iterations; ++i) {
      deserialized.deserialize(serialized_data);
    }
    end = std::chrono::high_resolution_clock::now();

    auto deserialize_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "反序列化耗时: " << deserialize_time.count() << " 微秒"
              << std::endl;
    std::cout << "平均每次反序列化: " << deserialize_time.count() / iterations
              << " 微秒" << std::endl;

    // 验证正确性
    if (verifyMessages(msg, deserialized)) {
      std::cout << "✓ 性能测试中的数据验证通过" << std::endl;
    } else {
      std::cout << "✗ 性能测试中的数据验证失败" << std::endl;
    }
  }

  // 运行所有测试
  void runAllTests() {
    std::cout << "=== 复杂消息类型测试 ===" << std::endl;

    int passed = 0;
    int total = 100;

    for (int i = 0; i < total; ++i) {
      auto original = generateRandomMessage();
      auto serialized = original.serialize();

      complex_message deserialized;
      deserialized.deserialize(serialized);

      if (verifyMessages(original, deserialized)) {
        passed++;
      } else {
        std::cout << "测试 " << i + 1 << " 失败" << std::endl;
      }

      if ((i + 1) % 20 == 0) {
        std::cout << "完成 " << i + 1 << "/" << total << " 个测试..."
                  << std::endl;
      }
    }

    std::cout << "\n测试结果: " << passed << "/" << total << " 通过"
              << std::endl;

    if (passed == total) {
      std::cout << "✓ 所有随机测试通过！" << std::endl;
      performanceTest();
    } else {
      std::cout << "✗ 有 " << (total - passed) << " 个测试失败！" << std::endl;
    }
  }
};

int main() {
  ComplexMessageTest test;
  test.runAllTests();
  return 0;
}
