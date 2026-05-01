#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import subprocess
import sys
import sysconfig
from pathlib import Path

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


class CMakeBuildExt(build_ext):
    def run(self) -> None:
        for ext in self.extensions:
            self.build_with_cmake(ext)

    def build_with_cmake(self, ext: Extension) -> None:
        python_dir = Path(__file__).resolve().parent
        source_dir = python_dir.parent
        cmake_build_dir = Path(os.environ.get("CPP_IPC_CMAKE_BUILD_DIR", source_dir / "build_py"))
        cmake_build_dir.mkdir(parents=True, exist_ok=True)

        build_type = os.environ.get("CMAKE_BUILD_TYPE", "Release")

        configure_cmd = [
            "cmake",
            "-S",
            str(source_dir),
            "-B",
            str(cmake_build_dir),
            f"-DPython3_EXECUTABLE={sys.executable}",
            f"-DCMAKE_BUILD_TYPE={build_type}",
            "-DLIBIPC_BUILD_PYTHON=ON",
            "-DLIBIPC_BUILD_TESTS=OFF",
            "-DLIBIPC_BUILD_DEMOS=OFF",
        ]

        subprocess.check_call(configure_cmd)

        build_cmd = [
            "cmake",
            "--build",
            str(cmake_build_dir),
            "--target",
            "dzipc",
            "-j",
        ]
        subprocess.check_call(build_cmd)

        ext_suffix = sysconfig.get_config_var("EXT_SUFFIX") or ".so"
        built_module = cmake_build_dir / "python" / f"dzipc{ext_suffix}"
        if not built_module.exists():
            raise FileNotFoundError(f"未找到已构建模块: {built_module}")

        output_path = Path(self.get_ext_fullpath(ext.name))
        output_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(built_module, output_path)


setup(
    name="dzipc",
    version="0.1.0",
    description="Python bindings for cpp-ipc (dzIPC)",
    ext_modules=[Extension("dzipc", sources=[])],
    cmdclass={"build_ext": CMakeBuildExt},
    zip_safe=False,
)
