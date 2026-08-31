cmake_minimum_required(VERSION 3.20)
project(jet-clickhouse VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# Protobuf (works with Ubuntu pkgs)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

# gRPC: try CONFIG then fall back to pkg-config (Ubuntu)
find_package(gRPC CONFIG QUIET)
if(gRPC_FOUND)
  set(_GRPC_GRPCPP gRPC::grpc++)
  set(_GRPC_REFLECTION gRPC::grpc++_reflection)
  if(TARGET gRPC::grpc_cpp_plugin)
    set(_GRPC_CPP_PLUGIN $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
  else()
    find_program(_GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
  endif()
  message(STATUS "Using gRPC (CONFIG) ${gRPC_VERSION}")
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GRPC REQUIRED IMPORTED_TARGET grpc++)
  pkg_check_modules(GRPC_REFLECTION QUIET IMPORTED_TARGET grpc++_reflection)
  set(_GRPC_GRPCPP PkgConfig::GRPC)
  if(GRPC_REFLECTION_FOUND)
    set(_GRPC_REFLECTION PkgConfig::GRPC_REFLECTION)
  else()
    set(_GRPC_REFLECTION "")
  endif()
  find_program(_GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
  message(STATUS "Using gRPC via pkg-config + plugin ${_GRPC_CPP_PLUGIN}")
endif()

if(TARGET protobuf::protoc)
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
else()
  find_program(_PROTOBUF_PROTOC protoc REQUIRED)
endif()

# nlohmann_json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
  FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(json)
endif()

# spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
  )
  set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
  set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(spdlog)
endif()

# NATS C client
find_package(NATS QUIET)
if(NOT NATS_FOUND)
  FetchContent_Declare(
    nats
    GIT_REPOSITORY https://github.com/nats-io/nats.c.git
    GIT_TAG        v3.9.1
    GIT_SHALLOW    TRUE
  )
  set(NATS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(NATS_BUILD_STREAMING OFF CACHE BOOL "" FORCE)
  set(NATS_BUILD_LIB_SHARED OFF CACHE BOOL "" FORCE)
  set(NATS_BUILD_LIB_STATIC ON CACHE BOOL "" FORCE)
  set(NATS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(nats)
  set(NATS_LIBRARIES nats_static)
  set(NATS_INCLUDE_DIRS ${nats_SOURCE_DIR}/src)
else()
  set(NATS_LIBRARIES NATS::nats)
endif()

# ClickHouse client
option(BUILD_CLICKHOUSE_CONSUMER "Build the ClickHouse consumer" ON)
if(BUILD_CLICKHOUSE_CONSUMER)
  find_package(clickhouse-cpp QUIET)
  if(NOT clickhouse-cpp_FOUND)
    FetchContent_Declare(
      clickhouse-cpp
      GIT_REPOSITORY https://github.com/ClickHouse/clickhouse-cpp.git
      GIT_TAG        v2.5.1
      GIT_SHALLOW    TRUE
    )
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
    set(WITH_OPENSSL ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(clickhouse-cpp)
  endif()
endif()

# Protobuf
set(PROTO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(PROTO_FILES
  ${PROTO_ROOT}/common/v1/error.proto
  ${PROTO_ROOT}/meter/v1/meter.proto
  ${PROTO_ROOT}/meter/v1/ingest_service.proto
  ${PROTO_ROOT}/nats/v1/publisher_service.proto
)

set(GENERATED_SRCS)
set(GENERATED_HDRS)

foreach(PROTO ${PROTO_FILES})
  get_filename_component(PROTO_NAME ${PROTO} NAME_WE)
  get_filename_component(PROTO_DIR  ${PROTO} DIRECTORY)
  file(RELATIVE_PATH REL_DIR ${PROTO_ROOT} ${PROTO_DIR})

  set(OUT_DIR ${GENERATED_DIR}/${REL_DIR})
  file(MAKE_DIRECTORY ${OUT_DIR})

  set(PB_CC   ${OUT_DIR}/${PROTO_NAME}.pb.cc)
  set(PB_H    ${OUT_DIR}/${PROTO_NAME}.pb.h)
  set(GRPC_CC ${OUT_DIR}/${PROTO_NAME}.grpc.pb.cc)
  set(GRPC_H  ${OUT_DIR}/${PROTO_NAME}.grpc.pb.h)

  add_custom_command(
    OUTPUT  ${PB_CC} ${PB_H} ${GRPC_CC} ${GRPC_H}
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS    --proto_path=${PROTO_ROOT}
            --cpp_out=${GENERATED_DIR}
            --grpc_out=${GENERATED_DIR}
            --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN}
            ${PROTO}
    DEPENDS ${PROTO}
    COMMENT "Generating C++ from ${REL_DIR}/${PROTO_NAME}.proto"
    VERBATIM
  )

  list(APPEND GENERATED_SRCS ${PB_CC} ${GRPC_CC})
  list(APPEND GENERATED_HDRS ${PB_H}  ${GRPC_H})
endforeach()

add_library(balanz_proto STATIC ${GENERATED_SRCS} ${GENERATED_HDRS})
target_include_directories(balanz_proto PUBLIC
  ${GENERATED_DIR}
  ${Protobuf_INCLUDE_DIRS}
)
target_link_libraries(balanz_proto PUBLIC
  ${Protobuf_LIBRARIES}
  ${_GRPC_GRPCPP}
  ${_GRPC_REFLECTION}
  Threads::Threads
)

# Shared
add_library(balanz_common INTERFACE)
target_include_directories(balanz_common INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/shim
  ${NATS_INCLUDE_DIRS}
)
# Ensure nats headers are visible even if NATS_INCLUDE_DIRS was empty at configure time
if(DEFINED nats_SOURCE_DIR)
  target_include_directories(balanz_common INTERFACE ${nats_SOURCE_DIR}/src)
endif()
target_link_libraries(balanz_common INTERFACE
  balanz_proto
  nlohmann_json::nlohmann_json
  spdlog::spdlog
  ${NATS_LIBRARIES}
  Threads::Threads
)

# Binaries
add_subdirectory(src/nats_publisher)
add_subdirectory(src/ingest)
add_subdirectory(src/client)
add_subdirectory(src/consumer)

if(BUILD_CLICKHOUSE_CONSUMER)
  add_subdirectory(src/clickhouse_consumer)
endif()

message(STATUS "jet-clickhouse configured (C++${CMAKE_CXX_STANDARD})")
