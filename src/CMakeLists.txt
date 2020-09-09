cmake_minimum_required(VERSION 3.16)
project(XyRegEngine)

set(CMAKE_CXX_STANDARD 20)

add_library(XyRegEngineLib STATIC lex.cpp nfa.cpp)