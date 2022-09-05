cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_SUPPRESS_REGENERATION true)
project("LASlib")

if (!MSVC)
	add_compile_options(-O3 -Wall -Wno-strict-aliasing)
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(LAS_SRC
	./src/geoprojectionconverter.cpp
	./src/las2las.cpp
	./src/lasreader.cpp
	./src/lasignore.cpp
	./src/laswriter.cpp
	./src/lasreader_las.cpp
	./src/lasreader_bin.cpp
	./src/lasreader_shp.cpp
	./src/lasreader_asc.cpp
	./src/lasreader_bil.cpp
	./src/lasreader_dtm.cpp
	./src/lasreader_ply.cpp
	./src/lasreader_qfit.cpp
	./src/lasreader_txt.cpp
	./src/lasreadermerged.cpp
	./src/lasreaderbuffered.cpp
	./src/lasreaderstored.cpp
	./src/lasreaderpipeon.cpp
	./src/laswriter_las.cpp
	./src/laswriter_bin.cpp
	./src/laswriter_qfit.cpp
	./src/laswriter_wrl.cpp
	./src/laswriter_txt.cpp
	./src/laswritercompatible.cpp
	./src/laswaveform13reader.cpp
	./src/laswaveform13writer.cpp
	./src/lasutility.cpp
	./src/lasfilter.cpp
	./src/lastransform.cpp
	./src/laskdtree.cpp
	./src/fopen_compressed.cpp
)

set(LAZ_SRC
	lasreadpoint.cpp
	lasreaditemcompressed_v1.cpp
	lasreaditemcompressed_v2.cpp
	lasreaditemcompressed_v3.cpp
	lasreaditemcompressed_v4.cpp
	laswritepoint.cpp
	laswriteitemcompressed_v1.cpp
	laswriteitemcompressed_v2.cpp
	laswriteitemcompressed_v3.cpp
	laswriteitemcompressed_v4.cpp
	integercompressor.cpp
	arithmeticdecoder.cpp
	arithmeticencoder.cpp
	arithmeticmodel.cpp
	lasindex.cpp
	lasquadtree.cpp
	lasinterval.cpp
	laszip.cpp
	mydefs.cpp
)

foreach(file ${LAZ_SRC})
	list(APPEND LAZ_SRC_FULL ../laszip/src/${file})
endforeach(file)

file(GLOB_RECURSE LAS_INCLUDES
	${CMAKE_SOURCE_DIR}/libs/laszip/src/*.hpp 
	${CMAKE_SOURCE_DIR}/libs/laslib/inc/*.hpp
)

# message(STATUS "las includes ${LAS_INCLUDES}")

add_library(laslib ${LAS_SRC} ${LAS_INCLUDES} ${LAZ_SRC_FULL})

target_include_directories(laslib PUBLIC "./inc")

# add_subdirectory(./src)
# add_subdirectory(./example)
