cmake_minimum_required(VERSION 3.19)
project(proxy-server)
set(CMAKE_CXX_STANDARD 17)

set(EXECUTABLE_NAME "server")

add_executable(${EXECUTABLE_NAME} server.cpp config/Config.cpp)
target_include_directories(${EXECUTABLE_NAME} PUBLIC config)