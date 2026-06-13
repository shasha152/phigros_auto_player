cmake_minimum_required(VERSION 3.10)

project(better_auto_player VERSION 1.0 LANGUAGES CXX)

set(ANDROID_TOOLCHAIN clang)
set(ANDROID_NATIVE_API_LEVEL 30)
set(ANDROID_STL c++_shared)
set(ANDROID_ABI arm64-v8a)
set(ANDROID_CPP_FEATURES rtti)

add_executable(${PROJECT_NAME} main/main.cpp)