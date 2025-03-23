cmake_minimum_required(VERSION 3.0.0)
project(libdco VERSION 0.1.0)

# 编译参数
# set(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_C_COMPILER /usr/bin/clang)
SET(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -fPIC -std=c++17 -pthread -fvisibility=default")

# 生成libdco
add_subdirectory(${PROJECT_SOURCE_DIR}/src)

# 生成example
add_subdirectory(${PROJECT_SOURCE_DIR}/example)

# 测试用例
enable_testing()
