cmake_minimum_required(VERSION 3.16)
project(mos_link_protocol LANGUAGES CXX)

add_library(mos_link_protocol STATIC src/frame.cpp)
target_include_directories(mos_link_protocol PUBLIC include)
target_compile_features(mos_link_protocol PUBLIC cxx_std_17)

include(CTest)
if(BUILD_TESTING)
  add_executable(mos_link_protocol_tests tests/frame_test.cpp)
  target_link_libraries(mos_link_protocol_tests PRIVATE mos_link_protocol)
  add_test(NAME mos_link_protocol_tests COMMAND mos_link_protocol_tests)
endif()
