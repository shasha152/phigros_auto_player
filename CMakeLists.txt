cmake_minimum_required(VERSION 3.10)

project(phigros_auto_player VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AUTO_PLAYER_MAIN "Build auto_player's unit main" ON)

add_subdirectory(third_party/elfio)

file(GLOB CPP_SOURCE src/touch/*.cpp src/mem/*.cpp src/il2cpp/*.cpp src/meta/*.cpp)
add_library(phigros_auto_player STATIC ${CPP_SOURCE})
add_library(ap::phigros_auto_player ALIAS phigros_auto_player)

if(AUTO_PLAYER_MAIN)
  add_executable(better_auto_player main/main.cpp)
  target_link_libraries(better_auto_player PRIVATE ap::phigros_auto_player)
endif()

target_link_libraries(phigros_auto_player PRIVATE log android c++_static elfio::elfio)

target_include_directories(phigros_auto_player
  PUBLIC
  include
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pfr/include
)
