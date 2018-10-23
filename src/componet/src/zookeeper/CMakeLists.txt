
file(GLOB srcs . *.cpp *.c)

add_executable(zook ${srcs})

set_target_properties (zook PROPERTIES CLEAN_DIRECT_OUTPUT 1)
# add_dependencies(nimble_kvserver kvengine)
target_link_libraries(zook zkmt zookeeper_mt glog pthread)

