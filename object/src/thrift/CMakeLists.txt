

file(GLOB_RECURSE SRCS ./*.cpp)
#file(GLOB SRCS ObjectClient/*.cpp)

add_library(thriftlib ${SRCS})