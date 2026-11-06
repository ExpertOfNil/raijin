cmake_minimum_required(VERSION 3.16)

project(raijin_starter LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(Raijin 0.2.0 EXACT CONFIG REQUIRED)

add_executable(raijin-starter
    src/main.c
)

target_link_libraries(raijin-starter
    PRIVATE
        Raijin::Raijin
)
