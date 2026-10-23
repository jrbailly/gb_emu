cmake_minimum_required(VERSION 3.16)
project(gb_emu)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_COLOR_MAKEFILE ON)
enable_testing()

if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ---------------------------------------------------------------------------
# Frontend selection
#   Default : Qt6   (cmake ..)
#   SDL3    : cmake .. -DFRONTEND_QT=OFF
# ---------------------------------------------------------------------------
option(FRONTEND_QT "Build with Qt6 frontend" ON)

# ---------------------------------------------------------------------------
# Common dependencies (always required)
# ---------------------------------------------------------------------------
find_package(GTest REQUIRED)
find_package(libzip REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
include(GoogleTest)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
file(GLOB APP_SRCS  src/*.cpp)
file(GLOB CORE_SRCS core/src/*.cpp)
include_directories("inc" "core/inc" "frontend/inc")

if(FRONTEND_QT)
    find_package(Qt6 6.7 REQUIRED COMPONENTS Core Quick Qml Multimedia LinguistTools)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    set(FRONTEND_SRCS
        frontend/src/audio_qt.cpp
    )

    qt_add_executable(gb_emu
        ${APP_SRCS}
        ${CORE_SRCS}
        ${FRONTEND_SRCS}
    )

    qt_add_qml_module(gb_emu
        URI Gbemu
        VERSION 1.0
        QML_FILES
        ui/Main.qml
        ui/AboutDialog.qml
        ui/KeyBindingsDialog.qml
        SOURCES
        frontend/inc/frontend_qt.h
        frontend/src/frontend_qt.cpp
        frontend/inc/gb_framebuffer_item.h
        frontend/src/gb_framebuffer_item.cpp
        frontend/inc/controllers_qt.h
        frontend/src/controllers_qt.cpp
    )

    qt_add_translations(gb_emu
        TS_FILES
        translations/gb_emu_en.ts
        translations/gb_emu_fr.ts
        RESOURCE_PREFIX "/i18n"
    )

    target_link_libraries(gb_emu PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Qml
        Qt6::Multimedia
        libzip::zip
        Threads::Threads
        nlohmann_json::nlohmann_json
    )

    target_compile_definitions(gb_emu PRIVATE FRONTEND_QT)

else()
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

    file(GLOB FRONTEND_SRCS
        frontend/src/frontend_sdl.cpp
        frontend/src/audio_sdl.cpp
    )

    add_executable(gb_emu
        ${APP_SRCS}
        ${CORE_SRCS}
        ${FRONTEND_SRCS}
    )

    target_link_libraries(gb_emu PRIVATE
        SDL3::SDL3
        libzip::zip
        Threads::Threads
        nlohmann_json::nlohmann_json
    )

endif()

file(GLOB_RECURSE SRCS_TEST core/src/*.cpp tests/*.cpp)
install(TARGETS gb_emu DESTINATION bin)

add_executable(gb_emu_test ${SRCS_TEST})
target_link_libraries(gb_emu_test PRIVATE GTest::gtest_main nlohmann_json::nlohmann_json)

# Copy the CPU tests into the build directory.
add_custom_command(TARGET gb_emu_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpu_tests"
            "${CMAKE_CURRENT_BINARY_DIR}/cpu_tests")

gtest_discover_tests(gb_emu_test)
