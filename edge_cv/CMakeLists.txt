cmake_minimum_required(VERSION 3.16)
project(edge_cv_pipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# deps
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio highgui)

# onnx runtime in path
if(NOT DEFINED ONNXRUNTIME_ROOT)
    if(DEFINED ENV{ONNXRUNTIME_ROOT})
        set(ONNXRUNTIME_ROOT $ENV{ONNXRUNTIME_ROOT})
    else()
        set(ONNXRUNTIME_ROOT "/usr/local")
    endif()
endif()

find_path(ONNXRUNTIME_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    HINTS ${ONNXRUNTIME_ROOT}/include
          ${ONNXRUNTIME_ROOT}/include/onnxruntime
          /usr/include/onnxruntime
)

find_library(ONNXRUNTIME_LIB
    NAMES onnxruntime
    HINTS ${ONNXRUNTIME_ROOT}/lib
          ${ONNXRUNTIME_ROOT}/lib64
          /usr/lib
          /usr/local/lib
)

if(NOT ONNXRUNTIME_INCLUDE_DIR OR NOT ONNXRUNTIME_LIB)
    message(FATAL_ERROR
        "ONNX Runtime not found.\n"
        "Set -DONNXRUNTIME_ROOT=/path/to/onnxruntime or install the package.\n"
        "Download from https://github.com/microsoft/onnxruntime/releases")
endif()

message(STATUS "OpenCV          : ${OpenCV_VERSION}")
message(STATUS "ONNX Runtime    : ${ONNXRUNTIME_LIB}")
message(STATUS "ONNX include    : ${ONNXRUNTIME_INCLUDE_DIR}")

# target
add_executable(edge_cv_pipeline
    src/main.cpp
    src/capture.cpp
    src/detector.cpp
    src/postprocess.cpp
    src/pipeline.cpp
)

target_include_directories(edge_cv_pipeline
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_INCLUDE_DIR}
)

target_link_libraries(edge_cv_pipeline
    PRIVATE
        ${OpenCV_LIBS}
        ${ONNXRUNTIME_LIB}
        pthread
)

# enable more warnings
target_compile_options(edge_cv_pipeline PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

install(TARGETS edge_cv_pipeline RUNTIME DESTINATION bin)
