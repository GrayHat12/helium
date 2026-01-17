cmake_minimum_required(VERSION 3.16)

project(helium)

set(CMAKE_CXX_STANDARD 20)
add_executable(helium src/main.cpp)

file(READ "./neutron/neutron.asm" DATA_CONTENT)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/static.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/static.hpp"
    @ONLY
)

target_include_directories(helium PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
