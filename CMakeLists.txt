cmake_minimum_required(VERSION 3.24)
set(CMAKE_BUILD_TYPE Debug)
# message(STATUS "${CMAKE_BUILD_TYPE}")

project(fastpoints-native)

set(HEADER_FILES  
	./include/fastpoints-native.h
    ./include/callbacks.h
)

add_library(fastpoints-native MODULE src/fastpoints-native.cpp)
set_target_properties(fastpoints-native PROPERTIES BUNDLE TRUE)

set(POTREE_DIR "${PROJECT_SOURCE_DIR}/libs/Converter")
add_subdirectory(${POTREE_DIR})
target_link_libraries(fastpoints-native PotreeConverter)

# set(LASTOOLS_DIR "${PROJECT_SOURCE_DIR}/libs/LAStools")
# add_subdirectory(${LASTOOLS_DIR})

set(LASZIP_DIR "${PROJECT_SOURCE_DIR}/libs/laszip")
add_subdirectory(${LASZIP_DIR})

set (LASLIB_DIR "${PROJECT_SOURCE_DIR}/libs/laslib")
add_subdirectory(${LASLIB_DIR})

target_link_libraries(laslib laszip)
target_link_libraries(PotreeConverter laszip)
target_link_libraries(fastpoints-native laslib)

target_include_directories(PotreeConverter PRIVATE "./libs")

target_include_directories(fastpoints-native PRIVATE "./include")
# target_include_directories(PotreeConverter PRIVATE "./modules")
target_include_directories(fastpoints-native PRIVATE "./libs")
