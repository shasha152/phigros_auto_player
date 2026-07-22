cmake_minimum_required(VERSION 3.10)

project(better_auto_player VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party/pfr)

# set(ANDROID_TOOLCHAIN clang)
# set(ANDROID_NATIVE_API_LEVEL 30)
# set(ANDROID_STL c++_shared)
# set(ANDROID_ABI arm64-v8a)
# set(ANDROID_CPP_FEATURES rtti)
file(GLOB CPP_SOURCE src/touch/*.cpp)

add_executable(${PROJECT_NAME} main/main.cpp ${CPP_SOURCE})

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE log android c++_static pfr)