cmake_minimum_required(VERSION 3.0.0)
project(Engine VERSION 0.1.0)

include(CMakePackageConfigHelpers)

add_executable(Debug 
document.cpp main.cpp read_input_functions.cpp request_queue.cpp search_server.cpp string_processing.cpp remove_duplicates.cpp)

set_target_properties(Debug PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)