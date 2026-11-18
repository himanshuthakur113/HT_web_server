cmake_minimum_required(VERSION 3.10)
project(HT_Network_Project)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(server main.cpp src/HT_server.cpp src/HTTPRequest.cpp src/HTTPResponse.cpp)

target_include_directories(server PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(server PRIVATE Threads::Threads)
