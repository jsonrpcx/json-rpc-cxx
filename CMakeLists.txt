cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
project(libjson-rpc-cpp VERSION 2.0.0 LANGUAGES CXX)

add_executable(jsonrpccpp-test src/client.hpp src/dispatcher.hpp src/main.cpp src/client.test.cpp src/typemapper.test.cpp src/dispatcher.test.cpp src/server.test.cpp)
target_include_directories(jsonrpccpp-test PUBLIC vendor src)

enable_testing()
add_test(NAME test COMMAND jsonrpccpp-test)