
file(GLOB SRCS *.cpp)

OPTION(TEST_COMMAND "test command" ON)
OPTION(TEST_CLIENT "test client" ON)

OPTION(RANDOM_OUTPUT "output random key" OFF)
OPTION(PIECEWISE_OUTPUT "piecewise generate output" OFF)

add_library(bd_client ${SRCS})
target_link_libraries(bd_client bd_common boost_program_options)
add_dependencies(bd_client bd_common) 

IF(RANDOM_OUTPUT)
    add_definitions(-DRANDOM_OUTPUT=1)
ENDIF(RANDOM_OUTPUT)

IF(PIECEWISE_OUTPUT)
    add_definitions(-DPIECEWISE_OUTPUT=1)
ENDIF(PIECEWISE_OUTPUT)

IF(TEST_CLIENT)
    add_executable(test_client Sample/TestClient.cpp)
    target_link_libraries(test_client bd_client) 
ENDIF(TEST_CLIENT)

IF(TEST_COMMAND)
    add_executable(test_command Sample/TestCommand.cpp)
    target_link_libraries(test_command bd_client) 
ENDIF(TEST_COMMAND)
