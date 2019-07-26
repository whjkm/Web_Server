# CMake 最低版本要求
cmake_minimum_required(VERSION 2.8)

# 项目信息
project(WebServer CXX)

# 指定编译模式
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# c++ 编译器的选项
set(CXX_FLAGS
    -g
    -Wall
    -std=c++11
    -D_PTHREADS
    -Wno-unused-parameter
)

# 设置c++编译器
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")

# 优化等级
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# $变量
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")


string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
# 打印消息
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

# 添加子目录
add_subdirectory(WebServer)