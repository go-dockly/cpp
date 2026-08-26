cmake_minimum_required(VERSION 3.20)
project(mserver)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

add_library(proto proto/hello.proto)
target_link_libraries(protolib gRPC::grpc++)

target_include_directories(protolib ${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate(TARGET protolib LANGUAGE cpp)

add_executable(server src/server.cc)
target_link_libraries(server gRPC::grpc++)