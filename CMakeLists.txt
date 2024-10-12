cmake_minimum_required(VERSION 3.24.0)
set(CMAKE_CXX_STANDARD 23)

project(LyraStandardLibrary VERSION 0.8.0)


# add headers
add_library(LyraStandardLibrary INTERFACE)
add_library(LyraStandardLibrary_Headers INTERFACE)
add_library(LyraStandardLibrary::Headers ALIAS LyraStandardLibrary_Headers)

target_include_directories(LyraStandardLibrary_Headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


# tests
option(LSD_BUILD_TESTS "Build tests for LSD" OFF)
if (LSD_BUILD_TESTS)
	add_subdirectory("Tests")
endif()
