cmake_minimum_required(VERSION 3.16)
project(raijin VERSION 0.2.0 LANGUAGES CXX C)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
        "/usr/local/raijin-${PROJECT_VERSION}"
        CACHE PATH "Raijin installation prefix"
        FORCE
    )
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/
)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(Threads REQUIRED)

# SDL3 windowing backend enabled by default
option(RAIJIN_ENABLE_SDL3 "Enable the SDL3 window backend" ON)
if(RAIJIN_ENABLE_SDL3)
    find_package(SDL3 REQUIRED CONFIG)
    set(
        RAIJIN_WINDOW_BACKEND_SOURCE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/raijin_sdl3.c"
    )
else()
    set(
        RAIJIN_WINDOW_BACKEND_SOURCE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/raijin_headless.c"
    )
endif()

# Shader compile
set(
    RAIJIN_GENERATED_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

set(
    RAIJIN_EMBEDDED_SHADERS_C
    "${RAIJIN_GENERATED_DIR}/raijin_embedded_shaders.c"
)

add_custom_command(
    OUTPUT "${RAIJIN_EMBEDDED_SHADERS_C}"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${RAIJIN_GENERATED_DIR}"
    COMMAND
        "${CMAKE_COMMAND}"
        "-DSOLID=${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/solid_shader.wgsl"
        "-DEDGES=${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/edges_shader.wgsl"
        "-DOUTPUT=${RAIJIN_EMBEDDED_SHADERS_C}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_shaders.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/solid_shader.wgsl"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/edges_shader.wgsl"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_shaders.cmake"
    COMMENT "Embedding Raijin WGSL shaders"
    VERBATIM
)

add_library(logging STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/logging/logging.c"
)
add_library(Logging::Logging ALIAS logging)
set_target_properties(logging PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_include_directories(logging
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/logging/include"
)
target_compile_definitions(logging
    PRIVATE
    _POSIX_C_SOURCE=200809L
)
target_link_libraries(logging
    PUBLIC
    Threads::Threads
)

add_library(raijin SHARED
    "${CMAKE_CURRENT_SOURCE_DIR}/src/raijin.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/renderer.c"
    "${RAIJIN_WINDOW_BACKEND_SOURCE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mesh.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/camera.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/state.c"
    "${RAIJIN_EMBEDDED_SHADERS_C}"
)
add_library(Raijin::Raijin ALIAS raijin)

set_target_properties(raijin PROPERTIES
    EXPORT_NAME Raijin
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION}"
)

target_compile_features(raijin PUBLIC c_std_11)
target_compile_definitions(raijin PRIVATE CGLM_FORCE_DEPTH_ZERO_TO_ONE)

target_include_directories(raijin
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cimpl/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cglm/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/wgpu/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/logging/include"
)

target_link_libraries(raijin
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/wgpu/libwgpu_native.a"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cglm/libcglm.a"
        Logging::Logging
        m
        dl
)
if(RAIJIN_ENABLE_SDL3)
    target_link_libraries(raijin PRIVATE SDL3::SDL3)
endif()

add_executable(example-minimal
    "examples/minimal.c"
)

target_link_libraries(example-minimal
    PRIVATE
        Raijin::Raijin
)

# Testing

if(BUILD_TESTING)
    add_executable(raijin-test-validation
        tests/validation.c
    )

    target_link_libraries(raijin-test-validation
        PRIVATE
            Raijin::Raijin
    )

    add_test(
        NAME raijin-validation
        COMMAND raijin-test-validation
    )

    add_executable(raijin-test-headless
        tests/headless.c
    )

    target_link_libraries(raijin-test-headless
        PRIVATE
            Raijin::Raijin
    )

    add_test(
        NAME raijin-headless
        COMMAND raijin-test-headless
    )

    add_executable(raijin-test-mesh
        tests/mesh.c
    )
    target_include_directories(raijin-test-mesh
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cimpl/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cglm/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/wgpu/include"
    )

    target_link_libraries(raijin-test-mesh
        PRIVATE
        Raijin::Raijin
    )

    add_test(
        NAME raijin-mesh
        COMMAND raijin-test-mesh
    )

    add_executable(raijin-test-camera
        tests/camera.c
    )
    target_include_directories(raijin-test-camera
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cimpl/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cglm/include"
    )

    target_link_libraries(raijin-test-camera
        PRIVATE
        Raijin::Raijin
    )

    add_test(
        NAME raijin-camera
        COMMAND raijin-test-camera
    )

    add_executable(raijin-test-state
        tests/state.c
    )

    target_link_libraries(raijin-test-state
        PRIVATE
        Raijin::Raijin
    )

    add_test(
        NAME raijin-state
        COMMAND raijin-test-state
    )
endif()

# Install

install (TARGETS raijin
    EXPORT RaijinTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/raijin/raijin.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/raijin"
)

set(RAIJIN_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/Raijin"
)

install(EXPORT RaijinTargets
    FILE RaijinTargets.cmake
    NAMESPACE Raijin::
    DESTINATION "${RAIJIN_INSTALL_CMAKEDIR}"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RaijinConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/RaijinConfig.cmake"
    INSTALL_DESTINATION "${RAIJIN_INSTALL_CMAKEDIR}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/RaijinConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY ExactVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/RaijinConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/RaijinConfigVersion.cmake"
    DESTINATION "${RAIJIN_INSTALL_CMAKEDIR}"
)
