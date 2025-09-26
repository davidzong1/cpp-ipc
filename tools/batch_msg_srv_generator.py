#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
批量消息生成器 - 处理目录中的所有.msg文件
"""

import os
import sys
import glob
from typing import Optional, List
from msg_generator import MessageGenerator
from srv_generator import ServiceGenerator

default_msg_path = "./msg/"
default_output_path = "./include/ipc_msg/"
default_srv_path = "./include/ipc_srv/"
default_srv_input_path = "./srv/"
def process_msg_directory(input_dir: str, output_dir: Optional[str] = None) -> None:
    """处理目录中的所有.msg文件"""
    if output_dir is None:
        output_dir = default_output_path
    
    # 确保输出目录存在
    os.makedirs(output_dir, exist_ok=True)
    # 删除所有输出目录的文件
    for root, dirs, files in os.walk(output_dir):
        # 从dirs中移除ipc_msg_base，这样os.walk就不会进入这个目录
        if 'ipc_msg_base' in dirs:
            dirs.remove('ipc_msg_base')
        
        for file in files:
            try:
                os.remove(os.path.join(root, file))
            except OSError as e:
                print(f"警告: 无法删除文件 {os.path.join(root, file)}: {e}")
    
    # 删除空目录（从最深层开始）
    for root, dirs, files in os.walk(output_dir, topdown=False):
        # 跳过基础目录和ipc_msg_base目录
        if root == output_dir or 'ipc_msg_base' in root:
            continue
        try:
            # 只删除空目录
            os.rmdir(root)
        except OSError as e:
            # 目录不为空或其他错误，忽略
            pass
    
    # 查找所有.msg文件
    msg_files = glob.glob(os.path.join(input_dir, "**/*.msg"), recursive=True)
    
    if not msg_files:
        print(f"在目录 {input_dir} 中未找到.msg文件")
        return
    
    success_count = 0
    failed_files = []
    
    for msg_file in msg_files:
        try:
            generator = MessageGenerator()
            
            # 保持相对目录结构
            rel_path = os.path.relpath(msg_file, input_dir)
            rel_dir = os.path.dirname(rel_path)
            target_output_dir = os.path.join(output_dir, rel_dir) if rel_dir else output_dir
            
            # 确保目标输出目录存在
            os.makedirs(target_output_dir, exist_ok=True)
            
            output_file = generator.process_msg_file(msg_file, target_output_dir)
            success_count += 1
            
        except Exception as e:
            print(f"  错误: {e}")
            failed_files.append(msg_file)
    
    
    if failed_files:
        print(f"\n失败的文件:")
        for file in failed_files:
            print(f"  - {file}")


def process_srv_directory(input_dir: str, output_dir: Optional[str] = None) -> None:
    """处理目录中的所有.srv文件"""
    if output_dir is None:
        output_dir = default_srv_path
    
    # 确保输出目录存在
    os.makedirs(output_dir, exist_ok=True)
    # 删除所有输出目录的文件
    for root, dirs, files in os.walk(output_dir):
        # 从dirs中移除ipc_msg_base，这样os.walk就不会进入这个目录
        if 'ipc_msg_base' in dirs:
            dirs.remove('ipc_msg_base')
        
        for file in files:
            try:
                os.remove(os.path.join(root, file))
            except OSError as e:
                print(f"警告: 无法删除文件 {os.path.join(root, file)}: {e}")
    
    # 删除空目录（从最深层开始）
    for root, dirs, files in os.walk(output_dir, topdown=False):
        # 跳过基础目录和ipc_msg_base目录
        if root == output_dir or 'ipc_msg_base' in root:
            continue
        try:
            # 只删除空目录
            os.rmdir(root)
        except OSError as e:
            # 目录不为空或其他错误，忽略
            pass
    
    # 查找所有.msg文件
    msg_files = glob.glob(os.path.join(input_dir, "**/*.srv"), recursive=True)
    
    if not msg_files:
        print(f"在目录 {input_dir} 中未找到.srv文件")
        return
    
    
    success_count = 0
    failed_files = []
    
    for msg_file in msg_files:
        try:
            generator = ServiceGenerator()
            
            # 保持相对目录结构
            rel_path = os.path.relpath(msg_file, input_dir)
            rel_dir = os.path.dirname(rel_path)
            target_output_dir = os.path.join(output_dir, rel_dir) if rel_dir else output_dir
            
            # 确保目标输出目录存在
            os.makedirs(target_output_dir, exist_ok=True)
            
            output_file = generator.process_srv_file(msg_file, target_output_dir)
            success_count += 1
            
        except Exception as e:
            print(f"  错误: {e}")
            failed_files.append(msg_file)
    

    
    if failed_files:
        print(f"\n失败的文件:")
        for file in failed_files:
            print(f"    - {file}")

def get_msg_directories(input_dir: str) -> List[str]:
    """递归检索包含.msg文件的目录"""
    msg_directories = set()
    
    # 递归查找所有.msg文件
    for root, dirs, files in os.walk(input_dir):
        for file in files:
            if file.endswith('.msg'):
                msg_directories.add(root)
                break  # 找到一个.msg文件就添加目录，然后继续下一个目录
    
    return sorted(list(msg_directories))

def get_srv_directories(input_dir: str) -> List[str]:
    """递归检索包含.srv文件的目录"""
    srv_directories = set()
    
    # 递归查找所有.srv文件
    for root, dirs, files in os.walk(input_dir):
        for file in files:
            if file.endswith('.srv'):
                srv_directories.add(root)
                break  # 找到一个.srv文件就添加目录，然后继续下一个目录

    return sorted(list(srv_directories))

def main():
    """主函数"""
    msg_input_dir = default_msg_path
        
    
    # 递归检索包含.msg文件的目录
    msg_list = get_msg_directories(msg_input_dir)
    
    
    if not os.path.exists(msg_input_dir):
        print(f"错误: 输入目录 {msg_input_dir} 不存在")
        sys.exit(1)
    
    if not os.path.isdir(msg_input_dir):
        print(f"错误: {msg_input_dir} 不是一个目录")
        sys.exit(1)
    
    # 打印找到的包含.msg文件的目录
    if msg_list:
        pass
    else:
        print(f"在目录 {msg_input_dir} 中未找到包含.msg文件的目录")
        return
    
    try:
        process_msg_directory(msg_input_dir)
    except Exception as e:
        print(f"处理失败: {e}")
        sys.exit(1)
        
    # 递归检索包含.srv文件的目录
    srv_input_dir = default_srv_input_path
    srv_list = get_srv_directories(srv_input_dir)

    if not srv_list:
        print(f"在目录 {srv_input_dir} 中未找到包含.srv文件的目录")
        return



    try:
        process_srv_directory(srv_input_dir)
    except Exception as e:
        print(f"处理失败: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()