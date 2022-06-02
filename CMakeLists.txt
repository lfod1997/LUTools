cmake_minimum_required(VERSION 3.14)
project(LUTools)

set(CMAKE_CXX_STANDARD 17)

include_directories(lib)
add_executable(LUTools src/main.cpp src/defines.cpp)
