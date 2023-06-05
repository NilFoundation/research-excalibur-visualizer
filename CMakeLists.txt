cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(excalibur-gui)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libs/crypto3/")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/src/")
