
include_directories(gen-cpp)
file(GLOB SRCS ./gen-cpp/*.cpp)

add_executable(server ${SRCS} WorkServer.cpp)
target_link_libraries(server thrift)

add_executable(client ${SRCS} WorkClient.cpp)
target_link_libraries(client thrift)
add_dependencies(client thrift server) 
