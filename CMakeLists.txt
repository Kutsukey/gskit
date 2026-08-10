cmake_minimum_required(VERSION 3.15)

project(gskit)

add_executable(gskit src/validation.cpp src/cli.cpp)

target_include_directories(gskit PRIVATE include)

set_target_properties(gskit PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)