if (!MSVC)
	add_compile_options(-Wno-deprecated -Wno-write-strings -Wno-unused-result -std=c++11)
endif()
add_definitions(-DNDEBUG -DUNORDERED -DHAVE_UNORDERED_MAP)

include(CMakePrintHelpers)

cmake_print_variables(CMAKE_SOURCE_DIR)



foreach(file ${LAZ_SRC})
	list(APPEND LAZ_SRC_FULL ../../laszip/src/${file})
endforeach(file)

file(GLOB_RECURSE LAS_INCLUDES
	${CMAKE_SOURCE_DIR}/libs/laszip/src/*.hpp 
	${CMAKE_SOURCE_DIR}/libs/laslib/inc/*.hpp
)

add_library(laslib ${LAS_SRC} ${LAZ_SRC_FULL} ${LAS_INCLUDES})
set_property(TARGET laslib PROPERTY POSITION_INDEPENDENT_CODE ON)

if (BUILD_SHARED_LIBS)
	target_compile_definitions(laslib PRIVATE "COMPILE_AS_DLL")
endif()

target_include_directories(laslib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/laslib/inc>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/laszip/src>
  $<INSTALL_INTERFACE:include/laslib>
)


cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR)
set_target_properties(laslib PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../lib
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../lib
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../lib
)

if(MSVC)
   target_compile_options(laslib PRIVATE /Zi)

   # Tell linker to include symbol data
    set_target_properties(laslib PROPERTIES 
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
    )

    # Set file name & location
    set_target_properties(laslib PROPERTIES 
        COMPILE_PDB_NAME laslib 
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
    )
endif()

install(FILES ${LAS_INCLUDES} DESTINATION include/laslib)

target_include_directories(laslib PRIVATE "../libs")

if (MSVC)
	foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
		install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../lib/${OUTPUTCONFIG} DESTINATION lib/LASlib)
	endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
else()
	install(TARGETS laslib EXPORT laslib-targets
		ARCHIVE DESTINATION lib/laslib
		LIBRARY DESTINATION lib/laslib
		RUNTIME DESTINATION lib/laslib)
	install(EXPORT laslib-targets DESTINATION lib/cmake/laslib)
	install(FILES ${CMAKE_SOURCE_DIR}/laslib/src/laslib-config.cmake DESTINATION lib/cmake/laslib)
endif(MSVC)
