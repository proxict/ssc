cmake_minimum_required(VERSION 3.20)
project(ssc VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(ssc INTERFACE)

set_property(TARGET ssc PROPERTY CXX_STANDARD 17)
set_property(TARGET ssc PROPERTY CXX_STANDARD_REQUIRED TRUE)
set_property(TARGET ssc PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ssc PROPERTY POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

set(SKIP_PERFORMANCE_COMPARISON ON CACHE BOOL "disable cereal performance comparison" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "disable cereal tests" FORCE)
set(BUILD_SANDBOX OFF CACHE BOOL "disable cereal sandbox" FORCE)
set(BUILD_DOC OFF CACHE BOOL "disable cereal docs" FORCE)
FetchContent_Declare(cereal GIT_REPOSITORY https://github.com/USCiLab/cereal.git
                            GIT_TAG v1.3.2)
FetchContent_MakeAvailable(cereal)

target_compile_options(ssc INTERFACE
    -Wall
    -Wextra
    -pedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wnull-dereference
    -Wformat=2
)

target_include_directories(ssc
    INTERFACE include
)

target_link_libraries(ssc INTERFACE cereal)

if (BUILD_TESTS)
    add_subdirectory(test)
endif()

