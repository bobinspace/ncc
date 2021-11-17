cmake_minimum_required (VERSION 3.8)

project ("ncc"
    VERSION 0.0.1
    LANGUAGES CXX
    DESCRIPTION "TCP server and client"
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory ("src" "out")

target_compile_options("ncc" PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-missing-field-initializers)
