cmake_minimum_required(VERSION 3.25)

project(BetterOption)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)


add_library(better_option INTERFACE)
target_include_directories(better_option INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if(BUILD_TESTING)
add_subdirectory(tests)
endif()