cmake_minimum_required(VERSION 3.16)
project(raijin LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/
)

find_package(SDL3 REQUIRED CONFIG)

add_library(raijin STATIC
    "src/raijin.c"
)

add_library(Raijin::Raijin ALIAS raijin)

target_compile_features(raijin PUBLIC c_std_11)

target_include_directories(raijin
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cimpl/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cglm/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/wgpu/include"
)

target_link_libraries(raijin
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/wgpu/libwgpu_native.a"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cglm/libcglm.a"
        SDL3::SDL3
        m
        dl
)

add_executable(example-minimal
    "examples/minimal.c"
)

target_link_libraries(example-minimal
    PRIVATE
        Raijin::Raijin
)
