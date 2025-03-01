cmake_minimum_required(VERSION 3.13)
project(gb_emu)
set(CMAKE_CXX_STANDARD 14)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

set(SOURCE_FILES src/main.cpp)

add_executable(gb_emu ${SOURCE_FILES})
target_link_libraries(gb_emu PRIVATE SDL3::SDL3)