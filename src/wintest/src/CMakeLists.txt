

cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_FLAGS "-fPIC -O0 -Wall -g")

file(GLOB srcs . *.cpp *.c)

add_executable(test1 ${srcs})

set_target_properties (test1 PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# target_link_libraries(test1 glog)
