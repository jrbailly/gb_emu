cmake_minimum_required(VERSION 3.13)
project(gb_emu)
set(CMAKE_CXX_STANDARD 20)
set (CMAKE_COLOR_MAKEFILE ON)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared PATHS "D:\\workspace\\SDL3-3.2.8")
find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)

file(GLOB_RECURSE SRCS src/*.cpp)
include_directories ("inc")
add_executable(gb_emu ${SRCS})
target_link_libraries(gb_emu PRIVATE SDL3::SDL3 nlohmann_json::nlohmann_json)


file(GLOB_RECURSE SRCS_TEST src/cpu.cpp src/ram.cpp tests/*.cpp)
#add_executable(gb_emu_test ${SRCS_TEST})
#target_link_libraries(gb_emu_test PRIVATE SDL3::SDL3 nlohmann_json::nlohmann_json GTest::gtest_main)
#gtest_discover_tests(gb_emu_test)
