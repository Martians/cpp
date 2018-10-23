
cmake_minimum_required(VERSION 2.8)

file(GLOB SRCS *.cpp ../Perform/*.cpp ../Advance/*.cpp ../Advance/Buffer/*.cpp)
set(COMMON_LIB "-lpthread")

OPTION(COMMON_SPACE "using common space, can be set in any applet" OFF)
OPTION(COMMON_TEST "complie all test" ON)
OPTION(USING_UUID "using uuid" ON)

OPTION(TEST_MODE "using test mode, do some verify" OFF)

if (USING_COMMON_SPACE)
    add_definitions(-DCOMMON_SPACE=1)
ENDif (USING_COMMON_SPACE)

if (COMMON_TEST)
    add_definitions(-DCOMMON_TEST=1)
    add_executable(test_common Main.cxx)
    add_dependencies(test_common bd_common)
    target_link_libraries(test_common bd_common) 
ENDif (COMMON_TEST)

if (USING_UUID)
    add_definitions(-DUSING_UUID=1)
    set(COMMON_LIB "${COMMON_LIB} -luuid")  
ENDif (USING_UUID)

if (TEST_MODE)
    add_definitions(-DTEST_MODE=1)
ENDif (TEST_MODE)

add_library(bd_common ${SRCS})
target_link_libraries(bd_common ${COMMON_LIB})



#include_directories("." "src")

#set(DEVEL_HOME "/root/devel/")
#link_directories(${DEVEL_HOME}/static_lib ${DEVEL_HOME}/hyper_lib)

#set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH}:./hyper_lib)
# -rdynamic
# -Wl,-Bdynamic -Wl,-Bstatic
# -Wl,-rpath=. 
