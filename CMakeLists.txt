cmake_minimum_required(VERSION 3.24)

project(fastpoints-native)

add_library(fastpoints-native MODULE src/alt.cpp)
set_target_properties(fastpoints-native PROPERTIES BUNDLE TRUE)

set(POTREE_DIR "${PROJECT_SOURCE_DIR}/libs/Converter")
add_subdirectory(${POTREE_DIR})
target_link_libraries(fastpoints-native PotreeConverter)

# target_include_directories(PotreeConverter PRIVATE "./include")
# target_include_directories(PotreeConverter PRIVATE "./modules")
target_include_directories(fastpoints-native PRIVATE "./libs")
