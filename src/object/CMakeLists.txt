# cmake_minimum_required(VERSION <specify CMake version here>)
project(object)

set(CMAKE_CXX_STANDARD 14)
cmake_minimum_required(VERSION 3.5)

option(DEBUG "debug for gdb" ON)

if(DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()



include_directories(../../../3rd/include)

include_directories(src)
include_directories(src/Advance)
include_directories(src/Advance/Buffer)
include_directories(src/CodeHelper)
include_directories(src/Common)
include_directories(src/Perform)



