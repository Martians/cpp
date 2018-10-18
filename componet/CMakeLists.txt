
cmake_minimum_required(VERSION 2.8.5)
PROJECT(database)

add_compile_options(-std=c++11)

set(CMAKE_CXX_FLAGS "-fPIC -O0 -Wall -g")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
include_directories(include)
link_directories(libs)

add_subdirectory(src)
