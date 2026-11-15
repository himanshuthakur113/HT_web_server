cmake_minimum_required(VERSION 3.10)
project(HT_Network_Project)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(server main.cpp HT_server.cpp HTTPRequest.cpp)

target_link_libraries(server PRIVATE Threads::Threads)

add_executable(client client.cpp)