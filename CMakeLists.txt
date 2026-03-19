cmake_minimum_required(VERSION 3.10)
project(ICPMSystem)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(code code.cpp)

# Set optimization flags for release build
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")