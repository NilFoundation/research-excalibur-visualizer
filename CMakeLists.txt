cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(excalibur-gui)

# unsure why we need this, but we do
find_package(Boost COMPONENTS unit_test_framework)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libs/crypto3/")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/src/")
