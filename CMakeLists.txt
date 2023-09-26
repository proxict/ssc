cmake_minimum_required(VERSION 3.0)
project(ssc VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(ssc INTERFACE)

set_property(TARGET ssc PROPERTY CXX_STANDARD 17)
set_property(TARGET ssc PROPERTY CXX_STANDARD_REQUIRED TRUE)
set_property(TARGET ssc PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ssc PROPERTY POSITION_INDEPENDENT_CODE ON)

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
    INTERFACE external/cereal/include
)

if (BUILD_TESTS)
    add_subdirectory(test)
endif()

