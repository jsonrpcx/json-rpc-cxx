cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
project(json-rpc-cxx VERSION 2.0.0 LANGUAGES CXX)

option(CODE_COVERAGE "Enable coverage reporting" OFF)
add_library(coverage_config INTERFACE)
if(CODE_COVERAGE)
    message("Enabled coverage flags")
    target_compile_options(coverage_config INTERFACE -O0 -g --coverage)
    target_link_libraries(coverage_config INTERFACE --coverage)
endif()

add_executable(jsonrpccpp-test test/main.cpp test/client.cpp test/typemapper.cpp test/dispatcher.cpp test/server.cpp test/batchclient.cpp test/testclientconnector.hpp examples/warehouse/warehouseapp.cpp test/warehouseapp.cpp)
target_include_directories(jsonrpccpp-test PUBLIC vendor jsonrpccxx ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(jsonrpccpp-test coverage_config)

add_executable(example-warehouse examples/warehouse/main.cpp examples/warehouse/warehouseapp.cpp examples/warehouse/types.h examples/inmemoryconnector.hpp)
target_include_directories(example-warehouse PUBLIC vendor ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()
add_test(NAME test COMMAND jsonrpccpp-test)