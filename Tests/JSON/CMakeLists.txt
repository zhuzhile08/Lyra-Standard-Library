cmake_minimum_required(VERSION 3.24.0)
project(JSON)

add_executable(JSON "main.cpp")

target_link_libraries(JSON LyraStandardLibrary::Headers)
