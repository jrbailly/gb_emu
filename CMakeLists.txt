cmake_minimum_required(VERSION 3.13)
project(gb_emu)
set(CMAKE_CXX_STANDARD 20)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared PATHS "C:\\workspace\\SDL3-3.2.8")

file(GLOB_RECURSE SRCS src/*.cpp)
include_directories ("inc")

add_executable(gb_emu ${SRCS})
target_link_libraries(gb_emu PRIVATE SDL3::SDL3)