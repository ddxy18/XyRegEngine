cmake_minimum_required(VERSION 3.16)
project(XyRegEngine)

set(CMAKE_CXX_STANDARD 20)

include_directories(
        ../GoogleTest/googletest/include
        ../GoogleTest/googlemock/include)

add_executable(XyRegEngineTest lex_test.cpp nfa_test.cpp)

add_subdirectory(../GoogleTest ../GoogleTest)
add_subdirectory(../src ../src)

target_link_libraries(XyRegEngineTest XyRegEngineLib)
target_link_libraries(XyRegEngineTest gtest_main)

enable_testing()