cmake_minimum_required(VERSION 3.20)

project(gskit)

add_executable(gskit src/validation.cpp src/cli.cpp src/ply_reader.cpp src/ply_writer.cpp)

target_include_directories(gskit PRIVATE include)

set_target_properties(gskit PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)