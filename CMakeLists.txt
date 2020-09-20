cmake_minimum_required(VERSION 3.16)
project(XyRegEngine)

set(CMAKE_CXX_STANDARD 20)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")

include_directories(include)

#add_executable(Temp temp/temp.cpp)

add_executable(XyRegEngine src/main.cpp)

add_subdirectory(test)
target_link_libraries(XyRegEngine XyRegEngineLib)