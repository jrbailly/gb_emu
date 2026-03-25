cmake_minimum_required(VERSION 3.13)
project(gb_emu)
set(CMAKE_CXX_STANDARD 20)
set (CMAKE_COLOR_MAKEFILE ON)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared PATHS "D:\\workspace\\SDL3-3.4.2")
#find_package(GTest REQUIRED)

file(GLOB_RECURSE SRCS src/*.cpp core/src/*.cpp frontend/src/*.cpp)
include_directories ("inc" "core/inc" "frontend/inc")
add_executable(gb_emu ${SRCS})
target_link_libraries(gb_emu PRIVATE SDL3::SDL3)
target_compile_options(gb_emu PRIVATE -Wall -Wextra -Wpedantic -Werror)


file(GLOB_RECURSE SRCS_TEST core/src/cpu.cpp core/src/ram.cpp tests/*.cpp)
install(TARGETS gb_emu DESTINATION bin)

#add_executable(gb_emu_test ${SRCS_TEST})
#target_link_libraries(gb_emu_test PRIVATE SDL3::SDL3 GTest::gtest_main)
#gtest_discover_tests(gb_emu_test)
