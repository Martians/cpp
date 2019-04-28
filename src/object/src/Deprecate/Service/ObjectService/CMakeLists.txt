
link_directories("/usr/pgsql/lib")

file(GLOB SRCS *.cpp ObjectClient/*.cpp)
#file(GLOB SRCS ObjectClient/*.cpp)

add_library(object ${SRCS})
target_link_libraries(object common pq)
add_dependencies(object common) 

add_executable(Object Main.cpp)
target_link_libraries(Object object boost_program_options)
add_dependencies(Object object) 

# add_subdirectory(UnitTest)
