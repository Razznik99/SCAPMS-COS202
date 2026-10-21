# =============================================================================
#  SCAPMS - Smart Clinic Appointment & Patient Management System
#  File   : CMakeLists.txt
#
#  Build with CMake + Qt 6 (MinGW or MSVC):
#      cmake -S . -B build-cmake -G "MinGW Makefiles" ^
#            -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64
#      cmake --build build-cmake -j 4
# =============================================================================
cmake_minimum_required(VERSION 3.21)

project(SCAPMS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

file(GLOB_RECURSE SCAPMS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h")

qt_add_executable(SCAPMS
    ${SCAPMS_SOURCES}
    resources/resources.qrc)

target_include_directories(SCAPMS PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(SCAPMS PRIVATE Qt6::Widgets)

# Console subsystem so that "--selftest" can print its report to the terminal.
set_target_properties(SCAPMS PROPERTIES WIN32_EXECUTABLE OFF)

if (MSVC)
    target_compile_options(SCAPMS PRIVATE /W4)
else()
    target_compile_options(SCAPMS PRIVATE -Wall -Wextra)
endif()
