cmake_minimum_required(VERSION 3.10)
project(mrworldwide C)

set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mrworldwide
        watcher.c
        hidapi.h
        hidapi_winapi.h)

target_link_libraries(mrworldwide "${CMAKE_CURRENT_SOURCE_DIR}/hidapi.lib")

add_custom_command(TARGET mrworldwide POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/hidapi.dll"
    $<TARGET_FILE_DIR:mrworldwide>)
