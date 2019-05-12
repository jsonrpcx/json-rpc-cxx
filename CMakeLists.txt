cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
project(json-rpc-cxx VERSION 2.0.0 LANGUAGES CXX)

add_executable(jsonrpccpp-test src/client.hpp src/dispatcher.hpp test/main.cpp test/client.cpp test/typemapper.cpp test/dispatcher.cpp test/server.cpp)
target_include_directories(jsonrpccpp-test PUBLIC vendor src)

enable_testing()
add_test(NAME test COMMAND jsonrpccpp-test)