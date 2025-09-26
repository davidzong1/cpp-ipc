#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ROS2风格消息文件到C++头文件生成器
生成包含序列化和反序列化函数的C++类
"""

import os
import sys
import re
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass


@dataclass
class FieldInfo:
    """字段信息"""
    field_type: str  # 原始类型，如 int32, string[], float64[]
    field_name: str  # 字段名
    cpp_type: str    # C++类型
    is_array: bool   # 是否为数组
    is_string: bool  # 是否为字符串


class MessageGenerator:
    """消息生成器"""
    
    # ROS2类型到C++类型的映射
    TYPE_MAPPING = {
        'bool': 'bool',
        'int8': 'int8_t',
        'uint8': 'uint8_t',
        'int16': 'int16_t',
        'uint16': 'uint16_t',
        'int32': 'int32_t',
        'uint32': 'uint32_t',
        'int64': 'int64_t',
        'uint64': 'uint64_t',
        'float32': 'float',
        'float64': 'double',
        'string': 'std::string',
    }

    def __init__(self):
        self.fields: List[FieldInfo] = []
        
    def parse_msg_file(self, msg_file_path: str) -> None:
        """解析.msg文件"""
        with open(msg_file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            
        for line in lines:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
                
            # 解析字段定义
            parts = line.split()
            if len(parts) >= 2:
                field_type = parts[0]
                field_name = parts[1]
                
                # 检查是否为数组
                is_array = field_type.endswith('[]')
                if is_array:
                    base_type = field_type[:-2]  # 移除[]
                else:
                    base_type = field_type
                
                # 检查是否为字符串
                is_string = base_type == 'string'
                
                # 转换为C++类型
                if base_type in self.TYPE_MAPPING:
                    cpp_base_type = self.TYPE_MAPPING[base_type]
                else:
                    raise ValueError(f"Unsupported type: {base_type}")
                
                if is_array:
                    cpp_type = f"std::vector<{cpp_base_type}>"
                else:
                    cpp_type = cpp_base_type
                
                field_info = FieldInfo(
                    field_type=field_type,
                    field_name=field_name,
                    cpp_type=cpp_type,
                    is_array=is_array,
                    is_string=is_string
                )
                self.fields.append(field_info)
    
    def generate_class_header(self, class_name: str) -> str:
        """生成类头部"""
        return f"""#pragma once
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include "ipc_msg/ipc_msg_base/ipc_msg_base.hpp"
namespace dzIPC::Msg {{
class {class_name} : public ipc_msg_base
{{
public:
    /* 构造函数和析构函数 */
    {class_name}() = default;
    ~{class_name}() = default;
"""

    def generate_member_variables(self) -> str:
        """生成成员变量"""
        lines = []
        for field in self.fields:
            comment = f"/* {field.field_name} */"
            lines.append(f"    {field.cpp_type} {field.field_name}; {comment}")
        return '\n'.join(lines)
    
    def generate_constructors(self, class_name: str) -> str:
        """生成构造函数"""
        return f"""
    /* 成员变量 */"""
    
    def generate_serialize_function(self) -> str:
        """生成序列化函数"""
        lines = [
            "",
            "    /* 序列化函数 */",
            "    std::vector<char> serialize() const",
            "    {",
        ]
        
        # 计算各个字段的大小变量定义
        for i, field in enumerate(self.fields):
            if field.is_string and not field.is_array:
                lines.append(f"        int32_t {field.field_name}_size = {field.field_name}.size() * sizeof(char);")
            elif field.is_array and field.is_string:
                lines.append(f"        int32_t {field.field_name}_count = {field.field_name}.size();")
                lines.append(f"        int32_t {field.field_name}_total_size_ = 0;")
                lines.append(f"        for (const auto& str : {field.field_name}) {{")
                lines.append(f"            {field.field_name}_total_size_ += sizeof(int32_t) + str.size() * sizeof(char);")
                lines.append(f"        }}")
            elif field.is_array:
                base_type = field.cpp_type[12:-1]  # 从 std::vector<type> 中提取 type
                if base_type == "bool":
                    lines.append(f"        int32_t {field.field_name}_count = {field.field_name}.size();")
                    lines.append(f"        int32_t {field.field_name}_size = {field.field_name}_count * sizeof(bool);")
                else:
                    lines.append(f"        int32_t {field.field_name}_count = {field.field_name}.size();")
                    lines.append(f"        int32_t {field.field_name}_size = {field.field_name}_count * sizeof({base_type});")
            else:
                lines.append(f"        int32_t {field.field_name}_size = sizeof({field.field_name});")
        
        lines.append("")
        
        # 计算总缓冲区大小
        lines.append("        // 计算总缓冲区大小")
        lines.append("        size_t total_size_ = 0;")
        
        for field in self.fields:
            if field.is_string and not field.is_array:
                lines.append(f"        total_size_ += sizeof({field.field_name}_size) + {field.field_name}_size;")
            elif field.is_array and field.is_string:
                lines.append(f"        total_size_ += sizeof({field.field_name}_count) + {field.field_name}_total_size_;")
            elif field.is_array:
                lines.append(f"        total_size_ += sizeof({field.field_name}_count) + {field.field_name}_size;")
            else:
                lines.append(f"        total_size_ += {field.field_name}_size;")
        
        lines.append("")
        lines.append("        // 一次性分配缓冲区")
        lines.append("        std::vector<char> buffer(total_size_);")
        lines.append("        total_size = total_size_;")
        lines.append("        size_t offset = 0;")
        lines.append("")
        
        # 序列化每个字段
        for field in self.fields:
            if field.is_string and not field.is_array:
                # 单个字符串
                lines.extend([
                    f"        // 序列化 {field.field_name}",
                    f"        std::memcpy(buffer.data() + offset, &{field.field_name}_size, sizeof({field.field_name}_size));",
                    f"        offset += sizeof({field.field_name}_size);",
                    f"        std::memcpy(buffer.data() + offset, {field.field_name}.data(), {field.field_name}_size);",
                    f"        offset += {field.field_name}_size;",
                    ""
                ])
            elif field.is_array and field.is_string:
                # 字符串数组
                lines.extend([
                    f"        // 序列化 {field.field_name}",
                    f"        std::memcpy(buffer.data() + offset, &{field.field_name}_count, sizeof({field.field_name}_count));",
                    f"        offset += sizeof({field.field_name}_count);",
                    f"        for (const auto& str : {field.field_name}) {{",
                    f"            int32_t str_size = str.size() * sizeof(char);",
                    f"            std::memcpy(buffer.data() + offset, &str_size, sizeof(str_size));",
                    f"            offset += sizeof(str_size);",
                    f"            std::memcpy(buffer.data() + offset, str.data(), str_size);",
                    f"            offset += str_size;",
                    f"        }}",
                    ""
                ])
            elif field.is_array:
                # 基本类型数组
                base_type = field.cpp_type[12:-1]  # 从 std::vector<type> 中提取 type
                
                # 特殊处理 std::vector<bool>
                if base_type == "bool":
                    lines.extend([
                        f"        // 序列化 {field.field_name} (bool数组特殊处理)",
                        f"        std::memcpy(buffer.data() + offset, &{field.field_name}_count, sizeof({field.field_name}_count));",
                        f"        offset += sizeof({field.field_name}_count);",
                        f"        for (int32_t i = 0; i < {field.field_name}_count; ++i) {{",
                        f"            bool val = {field.field_name}[i];",
                        f"            std::memcpy(buffer.data() + offset, &val, sizeof(bool));",
                        f"            offset += sizeof(bool);",
                        f"        }}",
                        ""
                    ])
                else:
                    lines.extend([
                        f"        // 序列化 {field.field_name}",
                        f"        std::memcpy(buffer.data() + offset, &{field.field_name}_count, sizeof({field.field_name}_count));",
                        f"        offset += sizeof({field.field_name}_count);",
                        f"        if ({field.field_name}_count > 0) {{",
                        f"            std::memcpy(buffer.data() + offset, {field.field_name}.data(), {field.field_name}_size);",
                        f"            offset += {field.field_name}_size;",
                        f"        }}",
                        ""
                    ])
            else:
                # 基本类型
                lines.extend([
                    f"        // 序列化 {field.field_name}",
                    f"        std::memcpy(buffer.data() + offset, &{field.field_name}, sizeof({field.field_name}));",
                    f"        offset += sizeof({field.field_name});",
                    ""
                ])
        
        lines.extend([
            "        return buffer;",
            "    }"
        ])
        
        return '\n'.join(lines)
    
    def generate_deserialize_function(self) -> str:
        """生成反序列化函数"""
        lines = [
            "",
            "    /* 反序列化函数 */",
            "    void deserialize(const std::vector<char>& buffer) override",
            "    {",
            "        size_t offset = 0;",
        ]
        
        # 反序列化每个字段
        for field in self.fields:
            if field.is_string and not field.is_array:
                # 单个字符串
                lines.extend([
                    f"        // 反序列化 {field.field_name}",
                    f"        int32_t {field.field_name}_size;",
                    f"        std::memcpy(&{field.field_name}_size, buffer.data() + offset, sizeof({field.field_name}_size));",
                    f"        offset += sizeof({field.field_name}_size);",
                    f"        {field.field_name}.resize({field.field_name}_size / sizeof(char));",
                    f"        std::memcpy(const_cast<char*>({field.field_name}.data()), buffer.data() + offset, {field.field_name}_size);",
                    f"        offset += {field.field_name}_size;",
                    ""
                ])
            elif field.is_array and field.is_string:
                # 字符串数组
                lines.extend([
                    f"        // 反序列化 {field.field_name}",
                    f"        int32_t {field.field_name}_count;",
                    f"        std::memcpy(&{field.field_name}_count, buffer.data() + offset, sizeof({field.field_name}_count));",
                    f"        offset += sizeof({field.field_name}_count);",
                    f"        {field.field_name}.clear();",
                    f"        {field.field_name}.reserve({field.field_name}_count);",
                    f"        for (int32_t i = 0; i < {field.field_name}_count; ++i) {{",
                    f"            int32_t str_size;",
                    f"            std::memcpy(&str_size, buffer.data() + offset, sizeof(str_size));",
                    f"            offset += sizeof(str_size);",
                    f"            std::string str(str_size / sizeof(char), '\\0');",
                    f"            std::memcpy(const_cast<char*>(str.data()), buffer.data() + offset, str_size);",
                    f"            offset += str_size;",
                    f"            {field.field_name}.push_back(std::move(str));",
                    f"        }}",
                    ""
                ])
            elif field.is_array:
                # 基本类型数组
                base_type = field.cpp_type[12:-1]  # 从 std::vector<type> 中提取 type
                
                # 特殊处理 std::vector<bool>
                if base_type == "bool":
                    lines.extend([
                        f"        // 反序列化 {field.field_name} (bool数组特殊处理)",
                        f"        int32_t {field.field_name}_count;",
                        f"        std::memcpy(&{field.field_name}_count, buffer.data() + offset, sizeof({field.field_name}_count));",
                        f"        offset += sizeof({field.field_name}_count);",
                        f"        {field.field_name}.clear();",
                        f"        {field.field_name}.reserve({field.field_name}_count);",
                        f"        for (int32_t i = 0; i < {field.field_name}_count; ++i) {{",
                        f"            bool val;",
                        f"            std::memcpy(&val, buffer.data() + offset, sizeof(bool));",
                        f"            offset += sizeof(bool);",
                        f"            {field.field_name}.push_back(val);",
                        f"        }}",
                        ""
                    ])
                else:
                    lines.extend([
                        f"        // 反序列化 {field.field_name}",
                        f"        int32_t {field.field_name}_count;",
                        f"        std::memcpy(&{field.field_name}_count, buffer.data() + offset, sizeof({field.field_name}_count));",
                        f"        offset += sizeof({field.field_name}_count);",
                        f"        {field.field_name}.resize({field.field_name}_count);",
                        f"        if ({field.field_name}_count > 0) {{",
                        f"            std::memcpy({field.field_name}.data(), buffer.data() + offset, {field.field_name}_count * sizeof({base_type}));",
                        f"            offset += {field.field_name}_count * sizeof({base_type});",
                        f"        }}",
                        ""
                    ])
            else:
                # 基本类型
                lines.extend([
                    f"        // 反序列化 {field.field_name}",
                    f"        std::memcpy(&{field.field_name}, buffer.data() + offset, sizeof({field.field_name}));",
                    f"        offset += sizeof({field.field_name});",
                    ""
                ])
        
        lines.append("    }")
        
        return '\n'.join(lines)
    
    def generate_clone_function(self, class_name: str) -> str:
        """生成克隆函数"""
        return f"""
        /* 克隆函数 */
        {class_name}* clone() const override
        {{
            return new {class_name}(*this);
        }}"""

    
    def generate_class_footer(self) -> str:
        """生成类尾部"""
        return "\n};\n} // namespace dzIPC::Msg\n"
    
    def generate_hpp_file(self, class_name: str) -> str:
        """生成完整的.hpp文件"""
        parts = [
            self.generate_class_header(class_name),
            self.generate_constructors(class_name),
            "",
            self.generate_member_variables(),
            self.generate_serialize_function(),
            self.generate_deserialize_function(),
            self.generate_clone_function(class_name),
            self.generate_class_footer()
        ]
        return '\n'.join(parts)
    
    def process_msg_file(self, msg_file_path: str, output_dir: Optional[str] = None) -> str:
        """处理.msg文件并生成.hpp文件"""
        # 解析消息文件
        self.parse_msg_file(msg_file_path)
        
        # 获取类名
        base_name = os.path.splitext(os.path.basename(msg_file_path))[0]
        class_name = base_name
        
        # 生成.hpp文件内容
        hpp_content = self.generate_hpp_file(class_name)
        
        # 确定输出路径
        if output_dir is None:
            output_dir = os.path.dirname(msg_file_path)
        
        output_file_path = os.path.join(output_dir, f"{base_name}.hpp")
        
        # 写入文件
        with open(output_file_path, 'w', encoding='utf-8') as f:
            f.write(hpp_content)
        
        return output_file_path


def main():
    """主函数"""
    if len(sys.argv) != 2:
        print("用法: python3 msg_generator.py <msg_file_path>")
        print("示例: python3 msg_generator.py /path/to/message.msg")
        sys.exit(1)
    
    msg_file_path = sys.argv[1]
    
    if not os.path.exists(msg_file_path):
        print(f"错误: 文件 {msg_file_path} 不存在")
        sys.exit(1)
    
    if not msg_file_path.endswith('.msg'):
        print("错误: 输入文件必须是.msg格式")
        sys.exit(1)
    
    try:
        generator = MessageGenerator()
        output_file = generator.process_msg_file(msg_file_path)
        
    except Exception as e:
        print(f"生成失败: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
