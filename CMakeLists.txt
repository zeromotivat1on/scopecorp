cmake_minimum_required(VERSION 3.16)

project(scopecorp VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(RUN_TREE_DIR "${CMAKE_SOURCE_DIR}/run_tree")

set(GFX_API_LIST OPEN_GL DX12)
set(GFX_API      OPEN_GL CACHE STRING "Graphics API to use")

option(INCLUDE_TRACY "Include tracy profiler" ON)
option(DEVELOPER     "Development mode"       ON)

add_subdirectory(src)
