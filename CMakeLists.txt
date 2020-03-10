cmake_minimum_required(VERSION 3.10)
project(covscript_exp)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(third-party/mozart)
include_directories(third-party/mozart/mpp_core)
include_directories(third-party/mozart/mpp_foundation)
include_directories(third-party/mozart/mpp_system)
include_directories(third-party/mozart/mpp_string)

add_executable(covscript-exp main.cpp lexer.cpp lexer.hpp)
target_link_libraries(covscript-exp mpp_core mpp_foundation mpp_system mpp_string)

