from __future__ import annotations
import struct
import argparse
import sys
import time
from typing import List,Type, Dict
try:
    import dzipc as ipc
except Exception as exc:
    print("[ERROR] Cannot import dzipc：", exc)
    print("[HINT] Please first build the pybind module and add the directory where the .so file is located to the PYTHONPATH environment variable")
    sys.exit(1)
import socket
# 64-bit FNV-1a constants
FNV_OFFSET_BASIS_64 = 14695981039346656037
FNV_PRIME_64 = 1099511628211
UDP_DISCOVERY_BASE_PORT = 7400  # 根据实际值修改


def fnv1a64(data: str) -> int:
    """64-bit FNV-1a hash function."""
    hash_value = FNV_OFFSET_BASIS_64
    for byte in data.encode("utf-8"):
        hash_value ^= byte
        hash_value = (hash_value * FNV_PRIME_64) & 0xFFFFFFFFFFFFFFFF  # 保持64位截断
    return hash_value


def udp_discovery_port_calculate(topic_name: str, domain_id: int) -> int:
    """
    根据话题名称和域ID计算UDP发现端口号。
    
    :param topic_name: 话题名称
    :param domain_id:  域ID
    :return: 合法的UDP端口号 (uint16)
    :raises ValueError: 端口号超出 65535
    """
    hash_value = fnv1a64(topic_name) % 10000
    limited_port = UDP_DISCOVERY_BASE_PORT + domain_id * hash_value
    if limited_port > 65535:
        raise ValueError(
            "Calculated port number exceeds the maximum allowed value of 65535. "
            "Please choose a different topic name or domain ID."
        )
    return limited_port


def udp_discovery_addr_calculate(topic_name: str) -> str:
    """
    根据话题名称计算UDP组播地址（239.255.x.y）。
    
    :param topic_name: 话题名称
    :return: IPv4组播地址字符串
    """
    ip_prefix = "239.255."
    mid_hash = fnv1a64(topic_name + "_mid") % 254
    end_hash = fnv1a64(topic_name + "_end") % 254
    return f"{ip_prefix}{mid_hash}.{end_hash}"


_MESSAGE_REGISTRY: Dict[str, Type[ipc.IpcMsgBase]] = {
    cls.__name__: cls
    for cls in [
        ipc.ComplexMessage,
        ipc.TestMsg,
        ipc.RequestResponseTestRequest,
        ipc.RequestResponseTestResponse,
    ]
}


def get_message_class(type_name: str) -> Type[ipc.IpcMsgBase]:
    """
    根据类型名称字符串返回对应的消息类。

    :param type_name: 消息类名称字符串，如 "TestMsg"
    :return: 对应的消息类（未实例化）
    :raises KeyError: 未找到匹配类型时
    """
    if type_name not in _MESSAGE_REGISTRY:
        raise KeyError(
            f"Unknown message type: '{type_name}'. "
            f"Available types: {list(_MESSAGE_REGISTRY.keys())}"
        )
    return _MESSAGE_REGISTRY[type_name]


def create_message_by_name(type_name: str) -> ipc.IpcMsgBase:
    """
    根据类型名称字符串实例化并返回对应的消息对象。

    :param type_name: 消息类名称字符串
    :return: 对应消息类的实例
    """
    return get_message_class(type_name)()


def create_multicast_receiver(
    multicast_ip: str,
    port: int,
    buffer_size_mb: int = 65535,
    timeout: float | None = None,
) -> socket.socket:
    """
    创建并配置组播UDP接收套接字。

    :param multicast_ip:    组播地址，如 "239.255.1.1"
    :param port:            监听端口
    :param buffer_size_mb:  接收缓冲区大小（MB）
    :param timeout:         接收超时（秒），None 表示阻塞
    :return:                已配置的 socket 对象
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

    # 端口复用（同机多进程同时接收同一组播）
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, buffer_size_mb*1024*1024)

    # 绑定到组播端口
    sock.bind(("0.0.0.0", port))

    # 加入组播组
    mreq = struct.pack(
        "4s4s",
        socket.inet_aton(multicast_ip),
        socket.inet_aton("0.0.0.0"),
    )
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

    if timeout is not None:
        sock.settimeout(timeout)

    return sock


def create_shm_receiver(topic_name: str, domain_id: int) -> ipc.SHMReceiver:
    """
    创建并配置共享内存接收器。

    :param topic_name: 话题名称
    :param domain_id:  域ID
    :return: 已配置的 SHMReceiver 对象
    """
    return ipc.SHMReceiver(topic_name, domain_id)



def main(args):
    topic_name = args.topic
    domain_id=args.domain_id
    msg_id = args.msg_id
    ser_or_shm:bool =False if args.transport == "shm" else True
    ouput_info:str=""
    try:
        port = udp_discovery_port_calculate(topic_name, domain_id)
        addr = udp_discovery_addr_calculate(topic_name)
        ouput_info+=f"Topic Name: {topic_name}\n"
        ouput_info+=f"Domain ID: {domain_id}\n"
        ouput_info+=f"Message ID: {msg_id}\n"
        ouput_info+=f"Calculated UDP Discovery Port: {port}\n"
        ouput_info+=f"Calculated UDP Discovery Address: {addr}\n"
    except ValueError as e:
        print(f"[ERROR] {e}")
        sys.exit(1)
    if ser_or_shm:
        sock = create_multicast_receiver(addr, port)
    else:
        sock= 
    