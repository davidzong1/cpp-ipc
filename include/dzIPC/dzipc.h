#pragma once
#include "dzIPC/topic_ipc.h"
#include "dzIPC/server_ipc.h"
namespace dzIPC
{

    constexpr IPCType IPC_SHM = IPCType::Shm;       // 共享内存通信模式
    constexpr IPCType IPC_SOCKET = IPCType::Socket; // UDP通信模式

    template <bool IsServer>
    using ser_cli_IPC = std::conditional_t<IsServer, dzIPC::pimpl::server_ipc_impl, dzIPC::pimpl::client_ipc_impl>;

    template <bool IsPublisher>
    using pub_sub_IPC = std::conditional_t<IsPublisher, dzIPC::pimpl::publisher_ipc_impl, dzIPC::pimpl::subscriber_ipc_impl>;

    using msgPtr = std::shared_ptr<ipc_msg_base>; // 基类消息智能指针类型定义，用于接收数据

    /* 服务相关类定义 */
    using ServerIPC = ser_cli_IPC<true>;                                            // 服务客户通信-服务端类
    using ClientIPC = ser_cli_IPC<false>;                                           // 服务客户通信-客户端类
    using ServerIPCPtr = std::shared_ptr<ServerIPC>;                                // 服务客户通信-服务端类智能指针
    using ClientIPCPtr = std::shared_ptr<ClientIPC>;                                // 服务客户通信-客户端类智能指针
    using ServerCallBackFun = std::function<void(std::shared_ptr<ServiceData> &)>;  // 服务端回调函数类型定义
    using ServerDataPtr = std::shared_ptr<ServiceData>;                             // 服务数据智能指针类型定义
    template <typename T = ipc_msg_base,                                            //
              typename = std::enable_if_t<std::is_base_of<ipc_msg_base, T>::value>> //
    inline std::shared_ptr<ServiceData> ServerDataPtrMake(int msg_id = 0)           // 服务数据智能指针创建函数定义
    {
        return std::make_shared<ServiceData>(std::make_shared<T>(), std::make_shared<T>(), msg_id);
    }

    /* 话题相关类定义 */
    using PublisherIPC = pub_sub_IPC<true>;                                         // 发布订阅通信-发布者类
    using SubscriberIPC = pub_sub_IPC<false>;                                       // 发布订阅通信-订阅者类
    using PublisherIPCPtr = std::shared_ptr<PublisherIPC>;                          // 发布订阅通信-发布者类智能指针
    using SubscriberIPCPtr = std::shared_ptr<SubscriberIPC>;                        // 发布订阅通信-订阅者类智能指针
    using TopicDataPtr = std::shared_ptr<TopicData>;                                // 话题数据智能指针类型定义
    template <typename T = ipc_msg_base,                                            //
              typename = std::enable_if_t<std::is_base_of<ipc_msg_base, T>::value>> //
    inline std::shared_ptr<TopicData> TopicDataPtrMake(int msg_id = 0)              // 话题数据智能指针创建函数定义
    {
        return std::make_shared<TopicData>(std::make_shared<T>(), msg_id);
    }

}