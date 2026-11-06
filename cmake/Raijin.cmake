include_guard(GLOBAL)

get_filename_component(
    _raijin_bundle_root
    "${CMAKE_CURRENT_LIST_DIR}/.."
    ABSOLUTE
)

set(_raijin_bundle_include "${_raijin_bundle_root}/include")
set(_raijin_bundle_lib "${_raijin_bundle_root}/lib")

foreach(_required_file IN ITEMS
    "${_raijin_bundle_include}/raijin/raijin.h"
    "${_raijin_bundle_lib}/libraijin.a"
    "${_raijin_bundle_lib}/libwgpu_native.a"
    "${_raijin_bundle_lib}/libcglm.a"
    "${_raijin_bundle_lib}/liblogging.a"
)
    if(NOT EXISTS "${_required_file}")
        message(FATAL_ERROR "Incomplete Raijin bundle: ${_required_file}")
    endif()
endforeach()

find_package(Threads REQUIRED)
find_package(SDL3 CONFIG REQUIRED)

add_library(Raijin::Raijin STATIC IMPORTED GLOBAL)

set_target_properties(Raijin::Raijin PROPERTIES
    IMPORTED_LOCATION "${_raijin_bundle_lib}/libraijin.a"
    INTERFACE_INCLUDE_DIRECTORIES "${_raijin_bundle_include}"
)

target_link_libraries(Raijin::Raijin INTERFACE
    "${_raijin_bundle_lib}/libwgpu_native.a"
    "${_raijin_bundle_lib}/libcglm.a"
    "${_raijin_bundle_lib}/liblogging.a"
    SDL3::SDL3
    Threads::Threads
    m
    ${CMAKE_DL_LIBS}
)
