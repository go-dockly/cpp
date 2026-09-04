cmake_minimum_required(VERSION 3.16)
project(stt_proto LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# prefer sys install or fall back to FetchContent
find_path(SHERPA_ONNX_INCLUDE_DIR
  NAMES sherpa-onnx/c-api/cxx-api.h
  PATHS
    /usr/local/include
    /usr/include
    $ENV{SHERPA_ONNX_ROOT}/include
)

find_library(SHERPA_ONNX_LIB
  NAMES sherpa-onnx-c-api sherpa-onnx
  PATHS
    /usr/local/lib
    /usr/lib
    $ENV{SHERPA_ONNX_ROOT}/lib
)

if(NOT SHERPA_ONNX_INCLUDE_DIR OR NOT SHERPA_ONNX_LIB)
  message(STATUS "sherpa-onnx not found on system using FetchContent instead")
  include(FetchContent)
  FetchContent_Declare(
    sherpa-onnx
    GIT_REPOSITORY https://github.com/k2-fsa/sherpa-onnx.git
    GIT_TAG        v1.10.46   # recent stable tag
    GIT_SHALLOW    TRUE
  )
  set(SHERPA_ONNX_ENABLE_PYTHON OFF CACHE BOOL "" FORCE)
  set(SHERPA_ONNX_ENABLE_TESTS  OFF CACHE BOOL "" FORCE)
  set(SHERPA_ONNX_ENABLE_C_API  ON  CACHE BOOL "" FORCE)
  set(SHERPA_ONNX_ENABLE_BINARY OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS ON)
  FetchContent_MakeAvailable(sherpa-onnx)
  set(SHERPA_ONNX_INCLUDE_DIR ${sherpa-onnx_SOURCE_DIR})
  set(SHERPA_ONNX_LIB sherpa-onnx-c-api)
endif()

# portaudio for mic capture
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PORTAUDIO portaudio-2.0)
endif()

if(NOT PORTAUDIO_FOUND)
  find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
  find_library(PORTAUDIO_LIBRARY NAMES portaudio)
  if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
    set(PORTAUDIO_FOUND TRUE)
    set(PORTAUDIO_INCLUDE_DIRS ${PORTAUDIO_INCLUDE_DIR})
    set(PORTAUDIO_LIBRARIES ${PORTAUDIO_LIBRARY})
  endif()
endif()

if(NOT PORTAUDIO_FOUND)
  message(FATAL_ERROR
    "PortAudio not found.\n"
    "  Ubuntu:        sudo apt install libportaudio2 portaudio19-dev\n"
    "  macOS:         brew install portaudio\n")
endif()

# target
add_executable(stt_mic src/stt_mic.cpp)

target_include_directories(stt_mic PRIVATE
  ${SHERPA_ONNX_INCLUDE_DIR}
  ${PORTAUDIO_INCLUDE_DIRS}
)

target_link_libraries(stt_mic PRIVATE
  ${SHERPA_ONNX_LIB}
  ${PORTAUDIO_LIBRARIES}
  pthread
)

if(UNIX AND NOT APPLE)
  set_target_properties(stt_mic PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
  )
endif()

message(STATUS "stt_proto configured")
message(STATUS "  sherpa-onnx include: ${SHERPA_ONNX_INCLUDE_DIR}")
message(STATUS "  PortAudio:           ${PORTAUDIO_LIBRARIES}")