cmake_minimum_required(VERSION 3.5)
project (tinyjson_test)

if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall")
endif()

add_library(tinyjson tinyjson.cpp)
add_executable(tinyjson_test test.cpp)
target_link_libraries(tinyjson_test tinyjson)