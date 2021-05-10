cmake_minimum_required(VERSION 3.16)
project(proxy-server)
set(CMAKE_CXX_STANDARD 17)

set(EXECUTABLE_NAME "server")

add_executable(${EXECUTABLE_NAME}
        srcs/main.cpp
        srcs/config/Config.cpp
        srcs/Server.cpp
        srcs/Bridge.cpp
        )
target_include_directories(${EXECUTABLE_NAME} PUBLIC srcs/config srcs)