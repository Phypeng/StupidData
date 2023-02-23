cmake_minimum_required(VERSION 3.16)
project(StupidData)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(StupidData
        main.cpp
        s_data.cpp
        s_data.h)
