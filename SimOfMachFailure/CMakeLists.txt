cmake_minimum_required(VERSION 3.29)
project(SimOfMachFailure)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenMP REQUIRED)

add_executable(OneExperiment src/OneExper.cpp)

add_executable(exponential_experiments src/main_exponential.cpp)

add_executable(lognormal_experiments src/main_lognormal.cpp)

add_executable(weibull_experiments src/main_weibull.cpp)

add_executable(LotsExperimentsParallel
        parallelVersion/LotsWeibulParallelOpenMP.cpp)

target_link_libraries(OneExperiment OpenMP::OpenMP_CXX)
target_link_libraries(exponential_experiments OpenMP::OpenMP_CXX)
target_link_libraries(lognormal_experiments OpenMP::OpenMP_CXX)
target_link_libraries(weibull_experiments OpenMP::OpenMP_CXX)
target_link_libraries(LotsExperimentsParallel OpenMP::OpenMP_CXX)