cmake_minimum_required(VERSION 3.16)
project(OS1)

set(CMAKE_CXX_STANDARD 17)

add_executable(smash
        Commands.cpp
        signals.cpp
        smash.cpp)