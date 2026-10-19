cmake_minimum_required(VERSION 3.16)

project(raijin_starter LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

get_filename_component(
    RAIJIN_DEFAULT_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.."
    ABSOLUTE
)

set(
    RAIJIN_ROOT
    "${RAIJIN_DEFAULT_ROOT}"
    CACHE PATH
    "Path to the Raijin source tree"
)

set(
    RAIJIN_BUILD_DIR
    "${RAIJIN_ROOT}/build"
    CACHE PATH
    "Path containing the compiled Raijin libraries"
)

set(RAIJIN_LIBRARY "${RAIJIN_BUILD_DIR}/libraijin.a")
set(RAIJIN_LOGGING_LIBRARY "${RAIJIN_BUILD_DIR}/liblogging.a")
set(RAIJIN_WGPU_LIBRARY "${RAIJIN_ROOT}/lib/wgpu/libwgpu_native.a")
set(RAIJIN_CGLM_LIBRARY "${RAIJIN_ROOT}/lib/cglm/libcglm.a")

foreach(required_file
    "${RAIJIN_LIBRARY}"
    "${RAIJIN_LOGGING_LIBRARY}"
    "${RAIJIN_WGPU_LIBRARY}"
    "${RAIJIN_CGLM_LIBRARY}"
    "${RAIJIN_ROOT}/include/raijin/raijin.h"
)
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR "Required Raijin file not found: ${required_file}")
    endif()
endforeach()

find_package(SDL3 REQUIRED CONFIG)

# Simulate the imported target that an installed Raijin package should provide.
add_library(Raijin::Raijin STATIC IMPORTED GLOBAL)

set_target_properties(Raijin::Raijin PROPERTIES
    IMPORTED_LOCATION "${RAIJIN_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${RAIJIN_ROOT}/include"
    INTERFACE_LINK_LIBRARIES
        "${RAIJIN_WGPU_LIBRARY};${RAIJIN_CGLM_LIBRARY};${RAIJIN_LOGGING_LIBRARY};SDL3::SDL3;m;dl"
)

add_executable(raijin-starter
    src/main.c
)

target_link_libraries(raijin-starter
    PRIVATE
        Raijin::Raijin
)
