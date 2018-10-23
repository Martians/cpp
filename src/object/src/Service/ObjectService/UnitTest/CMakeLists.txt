
file(GLOB src *.cpp)

add_executable(ObjectUnitTest ${src})

target_link_libraries(ObjectUnitTest gtest object)
add_dependencies(ObjectUnitTest object)
