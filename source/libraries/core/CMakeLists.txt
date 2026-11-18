cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_PROJECT_NAME ControlUnit)
include(cmake/gcc-arm-none-eabi.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME} LANGUAGES C CXX ASM)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_executable(${CMAKE_PROJECT_NAME})
add_subdirectory(cmake/stm32cubemx)
add_subdirectory(../custom ${CMAKE_CURRENT_BINARY_DIR}/custom)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/ControlUnit.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    controlunit_custom
)
