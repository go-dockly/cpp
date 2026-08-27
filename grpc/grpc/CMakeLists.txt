cmake_minimum_required(VERSION 3.20)

project(gRPC_CPP_Examples VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(examples hello)

foreach(example ${examples})
    add_subdirectory(${example})
endforeach()