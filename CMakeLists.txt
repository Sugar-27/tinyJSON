cmake_minimum_required(VERSION 3.5)
project (tinyjson_test VERSION 0.1.0)

if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    # 设置指定的C++编译器版本是必须的，如果不设置，或者为OFF，则指定版本不可用时，会使用上一版>本。
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    # 指定为C++14 版本
    set(CMAKE_CXX_STANDARD 14)
endif()

add_library(tinyjson tinyjson.cpp)
add_executable(tinyjson_test test.cpp)
target_link_libraries(tinyjson_test tinyjson)