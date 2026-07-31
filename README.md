# Phigros Auto Player

一个用于 **Phigros** 的自动演奏库。

> ⚠️ **仅供学习、逆向分析和研究用途，请勿用于破坏游戏公平性。**

---

## Features

- 自动读取谱面 Note
- 自动计算点击时机
- 简单易用的接口
- 低 CPU 占用

## Requirements

- C++20
- Android
- 已获取目标进程权限（Root / 调试环境）
- CMake >= 3.20
- Android NDK (r27 或更高版本)
- Ninja (推荐)

## Build

git clone --recursive https://github.com/shasha152/phigros_auto_player.git

cd phigros_auto_player

cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE={NDK}/build/cmake/android.toolchain.cmake -DANDROID_TOOLCHAIN=clang -DANDROID_NATIVE_API_LEVEL=30 -DANDROID_STL=c++_shared -DANDROID_ABI=arm64-v8a -DANDROID_CPP_FEATURES=rtti

cd build

ninja