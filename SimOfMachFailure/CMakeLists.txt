cmake_minimum_required(VERSION 3.29)
project(SimOfMachFailure)

set(CMAKE_CXX_STANDARD 20)

add_executable(OneExperiment OneExper.cpp)
add_executable(LotsExperiments LotsExper.cpp)
