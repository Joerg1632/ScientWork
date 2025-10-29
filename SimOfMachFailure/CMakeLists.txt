cmake_minimum_required(VERSION 3.29)
project(SimOfMachFailure)

set(CMAKE_CXX_STANDARD 20)

add_executable(OneExperiment src/OneExper.cpp)
add_executable(LotsExperiments src/LotsExperExp.cpp)
add_executable(LotsExperimentsLogNormal src/LotsExperLogNormal.cpp)
add_executable(LotsExperimentsParallel src/LotsExperParallel.cpp)
add_executable(LotsExperimentVeibulla src/LotsExperVeibula.cpp)