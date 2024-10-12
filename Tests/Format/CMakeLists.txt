cmake_minimum_required(VERSION 3.24.0)
project(Format)

add_executable(Format "main.cpp")

target_link_libraries(Format LyraStandardLibrary::Headers)
