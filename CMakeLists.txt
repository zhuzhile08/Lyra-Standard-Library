cmake_minimum_required(VERSION 3.24.0)
set(CMAKE_CXX_STANDARD 23)

project(LyraStandardLibrary VERSION 0.9.1)

add_library(LyraStandardLibrary INTERFACE)
add_library(LyraStandardLibrary_Headers INTERFACE)
add_library(LyraStandardLibrary::Headers ALIAS LyraStandardLibrary_Headers)

target_include_directories(LyraStandardLibrary_Headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
