cmake_minimum_required(VERSION 3.10)

project(better_auto_player VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party/elfio)

file(GLOB CPP_SOURCE src/touch/*.cpp src/mem/*.cpp src/il2cpp/*.cpp)

add_executable(${PROJECT_NAME} main/main.cpp ${CPP_SOURCE})

target_link_libraries(${PROJECT_NAME} PRIVATE log android c++_static elfio)

target_include_directories(${PROJECT_NAME}
  PRIVATE
  include
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pfr/include
)