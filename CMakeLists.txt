cmake_minimum_required(VERSION 3.15)
project(LimitOrderBook)

set(CMAKE_CXX_STANDARD 20)

# vcpkg toolchain provides this
find_package(Catch2 CONFIG REQUIRED)

# Main executable
include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(main src/main.cpp)

# Test executable
add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR})