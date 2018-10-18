
PROJECT(ObjectProject)

cmake_minimum_required(VERSION 2.8)

set(COMMON_CXX_FLAGS "-g -Wall -D_REENTRANT --std=c++11")
set(CMAKE_CXX_FLAGS "${COMMON_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

set(PROJECT_SRC ${PROJECT_SOURCE_DIR}/src)
set(EXEC_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${EXEC_OUTPUT_PATH})
set(LIBRARY_OUTPUT_PATH ${EXEC_OUTPUT_PATH}/lib)

include_directories(${PROJECT_SRC} ${PROJECT_SOURCE_DIR}/include)

if (WIN32)
include_directories(
        "D:/Workspace/Environment/boost_1_57_0"
        "D:/Program Files (x86)/Microsoft Visual Studio 8/VC/PlatformSDK/Include")
add_subdirectory(src/thrift)
endif(WIN32)

link_directories(${PROJECT_SOURCE_DIR}/lib /usr/local/lib)

add_subdirectory(src)
