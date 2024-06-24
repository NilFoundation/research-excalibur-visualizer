cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(excalibur-gui)

find_package(crypto3 REQUIRED)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/src/")
