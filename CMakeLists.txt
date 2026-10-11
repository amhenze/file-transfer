cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
  file_transfer
  VERSION 1.0.0
  LANGUAGES CXX)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)
