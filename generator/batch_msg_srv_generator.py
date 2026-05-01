#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
批量消息生成器 - 处理目录中的所有.msg文件
"""

import os
import sys
import glob
from typing import Optional, List, Tuple
from msg_generator import MessageGenerator
from srv_generator import ServiceGenerator

default_msg_path = "./msg/"
default_output_path = "./include/ipc_msg/"
default_srv_path = "./include/ipc_srv/"
default_srv_input_path = "./srv/"
default_interface_path = "./python/src/interface.cc"
dzipc_cat_path="./exec/dzipc_cat/main.cc"
default_pyi_path = "./python/dzipc.pyi"

AUTO_INC_BEGIN = "// AUTO_GENERATED_MSG_SRV_INCLUDES_BEGIN"
AUTO_INC_END = "// AUTO_GENERATED_MSG_SRV_INCLUDES_END"
AUTO_BIND_BEGIN = "// AUTO_GENERATED_MSG_SRV_BINDINGS_BEGIN"
AUTO_BIND_END = "// AUTO_GENERATED_MSG_SRV_BINDINGS_END"
AUTO_PYI_BEGIN = "# AUTO_GENERATED_MSG_SRV_STUBS_BEGIN"
AUTO_PYI_END = "# AUTO_GENERATED_MSG_SRV_STUBS_END"

PY_TYPE_MAPPING = {
    "bool": "bool",
    "int8": "int",
    "uint8": "int",
    "int16": "int",
    "uint16": "int",
    "int32": "int",
    "uint32": "int",
    "int64": "int",
    "uint64": "int",
    "float32": "float",
    "float64": "float",
    "string": "str",
}


def snake_to_pascal(name: str) -> str:
    return "".join(part.capitalize() for part in name.split("_") if part)


def parse_fields(lines: List[str]) -> List[str]:
    fields: List[str] = []
    for raw in lines:
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) >= 2:
            fields.append(parts[1])
    return fields


def parse_typed_fields(lines: List[str]) -> List[Tuple[str, str]]:
    fields: List[Tuple[str, str]] = []
    for raw in lines:
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) >= 2:
            field_type = parts[0]
            field_name = parts[1]
            is_array = field_type.endswith("[]")
            base_type = field_type[:-2] if is_array else field_type
            py_base = PY_TYPE_MAPPING.get(base_type, "object")
            py_type = f"List[{py_base}]" if is_array else py_base
            fields.append((field_name, py_type))
    return fields


def parse_msg_fields(msg_file: str) -> List[str]:
    with open(msg_file, "r", encoding="utf-8") as f:
        return parse_fields(f.readlines())


def parse_msg_fields_typed(msg_file: str) -> List[Tuple[str, str]]:
    with open(msg_file, "r", encoding="utf-8") as f:
        return parse_typed_fields(f.readlines())


def parse_srv_fields(srv_file: str) -> Tuple[List[str], List[str]]:
    with open(srv_file, "r", encoding="utf-8") as f:
        lines = f.readlines()

    req_lines: List[str] = []
    resp_lines: List[str] = []
    in_response = False

    for raw in lines:
        line = raw.strip()
        if line == "---":
            in_response = True
            continue

        if in_response:
            resp_lines.append(raw)
        else:
            req_lines.append(raw)

    return parse_fields(req_lines), parse_fields(resp_lines)


def parse_srv_fields_typed(srv_file: str) -> Tuple[List[Tuple[str, str]], List[Tuple[str, str]]]:
    with open(srv_file, "r", encoding="utf-8") as f:
        lines = f.readlines()

    req_lines: List[str] = []
    resp_lines: List[str] = []
    in_response = False

    for raw in lines:
        line = raw.strip()
        if line == "---":
            in_response = True
            continue

        if in_response:
            resp_lines.append(raw)
        else:
            req_lines.append(raw)

    return parse_typed_fields(req_lines), parse_typed_fields(resp_lines)


def replace_block(content: str, begin: str, end: str, block_body: str) -> str:
    begin_idx = content.find(begin)
    end_idx = content.find(end)
    if begin_idx == -1 or end_idx == -1 or end_idx < begin_idx:
        raise RuntimeError(f"未找到自动生成锚点: {begin} / {end}")

    begin_line_end = content.find("\n", begin_idx)
    if begin_line_end == -1:
        begin_line_end = begin_idx + len(begin)

    return content[: begin_line_end + 1] + block_body + content[end_idx:]


def generate_pybind_blocks(
    msg_files: List[str],
    srv_files: List[str],
    msg_root: str,
    srv_root: str,
) -> Tuple[str, str]:
    include_lines: List[str] = []
    binding_lines: List[str] = []
    msg_types: List[Tuple[str, str]] = []
    srv_types: List[str] = []

    for msg_file in sorted(msg_files):
        rel = os.path.relpath(msg_file, msg_root).replace("\\", "/")
        stem = os.path.splitext(rel)[0]
        class_name = os.path.splitext(os.path.basename(msg_file))[0]
        py_name = snake_to_pascal(class_name)
        fields = parse_msg_fields(msg_file)
        msg_types.append((py_name, class_name))

        include_lines.append(f'#include "ipc_msg/{stem}.hpp"')

        binding_lines.append(
            f'    py::class_<dzIPC::Msg::{py_name}, IpcMsgBase, std::shared_ptr<dzIPC::Msg::{py_name}>>(m, "{py_name}")'
        )
        binding_lines.append("        .def(py::init<>())")
        for field in fields:
            binding_lines.append(
                f'        .def_readwrite("{field}", &dzIPC::Msg::{py_name}::{field})'
            )
        if fields:
            binding_lines[-1] = binding_lines[-1] + ";"
        else:
            binding_lines.append("        ;")
        binding_lines.append("")

    for srv_file in sorted(srv_files):
        rel = os.path.relpath(srv_file, srv_root).replace("\\", "/")
        stem = os.path.splitext(rel)[0]
        base_name = os.path.splitext(os.path.basename(srv_file))[0]
        py_base = snake_to_pascal(base_name)
        req_fields, resp_fields = parse_srv_fields(srv_file)
        srv_types.append(py_base)

        include_lines.append(f'#include "ipc_srv/{stem}.hpp"')

        req_cls = f"{py_base}Request"
        resp_cls = f"{py_base}Response"

        binding_lines.append(
            f'    py::class_<dzIPC::Srv::{req_cls}, IpcMsgBase, std::shared_ptr<dzIPC::Srv::{req_cls}>>(m, "{py_base}Request")'
        )
        binding_lines.append("        .def(py::init<>())")
        for field in req_fields:
            binding_lines.append(
                f'        .def_readwrite("{field}", &dzIPC::Srv::{req_cls}::{field})'
            )
        if req_fields:
            binding_lines[-1] = binding_lines[-1] + ";"
        else:
            binding_lines.append("        ;")
        binding_lines.append("")

        binding_lines.append(
            f'    py::class_<dzIPC::Srv::{resp_cls}, IpcMsgBase, std::shared_ptr<dzIPC::Srv::{resp_cls}>>(m, "{py_base}Response")'
        )
        binding_lines.append("        .def(py::init<>())")
        for field in resp_fields:
            binding_lines.append(
                f'        .def_readwrite("{field}", &dzIPC::Srv::{resp_cls}::{field})'
            )
        if resp_fields:
            binding_lines[-1] = binding_lines[-1] + ";"
        else:
            binding_lines.append("        ;")
        binding_lines.append("")

    if msg_types:
        binding_lines.append('    m.def("message_types", []() {')
        binding_lines.append("        return std::vector<std::string>{")
        for py_name, _ in msg_types:
            binding_lines.append(f'            "{py_name}",')
        binding_lines.append("        };")
        binding_lines.append("    });")
        binding_lines.append("")

        binding_lines.append('    m.def("create_message", [](const std::string &type_name) -> std::shared_ptr<IpcMsgBase> {')
        for py_name, class_name in msg_types:
            binding_lines.append(f'        if (type_name == "{py_name}") return std::make_shared<dzIPC::Msg::{py_name}>();')
        binding_lines.append('        throw py::value_error("Unknown message type: " + type_name);')
        binding_lines.append("    });")
        binding_lines.append("")

    if srv_types:
        binding_lines.append('    m.def("service_types", []() {')
        binding_lines.append("        return std::vector<std::string>{")
        for py_base in srv_types:
            binding_lines.append(f'            "{py_base}",')
        binding_lines.append("        };")
        binding_lines.append("    });")
        binding_lines.append("")

        binding_lines.append('    m.def("create_service_request", [](const std::string &service_name) -> std::shared_ptr<IpcMsgBase> {')
        for srv_file in sorted(srv_files):
            base_name = os.path.splitext(os.path.basename(srv_file))[0]
            py_base = snake_to_pascal(base_name)
            req_cls = f"{py_base}Request"
            binding_lines.append(f'        if (service_name == "{py_base}") return std::make_shared<dzIPC::Srv::{req_cls}>();')
        binding_lines.append('        throw py::value_error("Unknown service type: " + service_name);')
        binding_lines.append("    });")
        binding_lines.append("")

        binding_lines.append('    m.def("create_service_response", [](const std::string &service_name) -> std::shared_ptr<IpcMsgBase> {')
        for srv_file in sorted(srv_files):
            base_name = os.path.splitext(os.path.basename(srv_file))[0]
            py_base = snake_to_pascal(base_name)
            resp_cls = f"{py_base}Response"
            binding_lines.append(f'        if (service_name == "{py_base}") return std::make_shared<dzIPC::Srv::{resp_cls}>();')
        binding_lines.append('        throw py::value_error("Unknown service type: " + service_name);')
        binding_lines.append("    });")
        binding_lines.append("")

    include_block = "\n".join(include_lines)
    if include_block:
        include_block += "\n"

    binding_block = "\n".join(binding_lines)
    if binding_block:
        binding_block += "\n"

    return include_block, binding_block


def generate_pyi_blocks(
    msg_files: List[str],
    srv_files: List[str],
    msg_root: str,
    srv_root: str,
) -> str:
    stub_lines: List[str] = []

    for msg_file in sorted(msg_files):
        class_name = os.path.splitext(os.path.basename(msg_file))[0]
        py_name = snake_to_pascal(class_name)
        fields = parse_msg_fields_typed(msg_file)
        stub_lines.append(f"class {py_name}(IpcMsgBase):")
        if fields:
            for field_name, field_type in fields:
                stub_lines.append(f"    {field_name}: {field_type}")
        else:
            stub_lines.append("    pass")
        stub_lines.append("")

    for srv_file in sorted(srv_files):
        base_name = os.path.splitext(os.path.basename(srv_file))[0]
        py_base = snake_to_pascal(base_name)
        req_fields, resp_fields = parse_srv_fields_typed(srv_file)

        stub_lines.append(f"class {py_base}Request(IpcMsgBase):")
        if req_fields:
            for field_name, field_type in req_fields:
                stub_lines.append(f"    {field_name}: {field_type}")
        else:
            stub_lines.append("    pass")
        stub_lines.append("")

        stub_lines.append(f"class {py_base}Response(IpcMsgBase):")
        if resp_fields:
            for field_name, field_type in resp_fields:
                stub_lines.append(f"    {field_name}: {field_type}")
        else:
            stub_lines.append("    pass")
        stub_lines.append("")

    if msg_files:
        stub_lines.append("def message_types() -> List[str]: ...")
        stub_lines.append("def create_message(type_name: str) -> IpcMsgBase: ...")
        stub_lines.append("")

    if srv_files:
        stub_lines.append("def service_types() -> List[str]: ...")
        stub_lines.append("def create_service_request(service_name: str) -> IpcMsgBase: ...")
        stub_lines.append("def create_service_response(service_name: str) -> IpcMsgBase: ...")
        stub_lines.append("")

    stub_block = "\n".join(stub_lines)
    if stub_block:
        stub_block += "\n"

    return stub_block


def generate_python_interface_bindings(
    msg_input_dir: str,
    srv_input_dir: str,
    interface_path: str = default_interface_path,
) -> None:
    msg_files = glob.glob(os.path.join(msg_input_dir, "**/*.msg"), recursive=True)
    srv_files = glob.glob(os.path.join(srv_input_dir, "**/*.srv"), recursive=True)

    if not os.path.exists(interface_path):
        raise RuntimeError(f"Python interface is not exist: {interface_path}")

    include_block, binding_block = generate_pybind_blocks(
        msg_files=msg_files,
        srv_files=srv_files,
        msg_root=msg_input_dir,
        srv_root=srv_input_dir,
    )

    with open(interface_path, "r", encoding="utf-8") as f:
        content = f.read()

    content = replace_block(content, AUTO_INC_BEGIN, AUTO_INC_END, include_block)
    content = replace_block(content, AUTO_BIND_BEGIN, AUTO_BIND_END, binding_block)

    with open(interface_path, "w", encoding="utf-8") as f:
        f.write(content)


def generate_python_stub_bindings(
    msg_input_dir: str,
    srv_input_dir: str,
    pyi_path: str = default_pyi_path,
) -> None:
    msg_files = glob.glob(os.path.join(msg_input_dir, "**/*.msg"), recursive=True)
    srv_files = glob.glob(os.path.join(srv_input_dir, "**/*.srv"), recursive=True)

    if not os.path.exists(pyi_path):
        raise RuntimeError(f"Python stub 文件不存在: {pyi_path}")

    stub_block = generate_pyi_blocks(
        msg_files=msg_files,
        srv_files=srv_files,
        msg_root=msg_input_dir,
        srv_root=srv_input_dir,
    )

    with open(pyi_path, "r", encoding="utf-8") as f:
        content = f.read()

    content = replace_block(content, AUTO_PYI_BEGIN, AUTO_PYI_END, stub_block)

    with open(pyi_path, "w", encoding="utf-8") as f:
        f.write(content)

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
        # 从dirs中移除IpcMsgBase，这样os.walk就不会进入这个目录
        if 'IpcMsgBase' in dirs:
            dirs.remove('IpcMsgBase')
        
        for file in files:
            try:
                os.remove(os.path.join(root, file))
            except OSError as e:
                print(f"警告: 无法删除文件 {os.path.join(root, file)}: {e}")
    
    # 删除空目录（从最深层开始）
    for root, dirs, files in os.walk(output_dir, topdown=False):
        # 跳过基础目录和IpcMsgBase目录
        if root == output_dir or 'IpcMsgBase' in root:
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

    try:
        generate_python_interface_bindings(msg_input_dir, srv_input_dir)
    except Exception as e:
        print(f"更新Python绑定失败: {e}")
        sys.exit(1)

    try:
        generate_python_stub_bindings(msg_input_dir, srv_input_dir)
    except Exception as e:
        print(f"更新Python stub失败: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()