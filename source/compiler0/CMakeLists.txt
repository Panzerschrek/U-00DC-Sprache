#
# LexSyntLib
#

string(
	REGEX REPLACE
	${KEYWORDS_LIST_REGEXP}
	"	\\0_,"
	KEYWORDS_LIST
	${KEYWORDS_LIST_CONTENT} )

string(
	REGEX REPLACE
	${KEYWORDS_LIST_REGEXP}
	"	\"\\0\","
	KEYWORDS_LIST_STRINGS
	${KEYWORDS_LIST_CONTENT} )

configure_file( lex_synt_lib/keywords.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/keywords.hpp )
configure_file( lex_synt_lib/keywords.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/keywords.cpp )

file( GLOB LEX_SYNT_LIB_SOURCES "lex_synt_lib/*.cpp" )
file( GLOB LEX_SYNT_LIB_HEADERS "lex_synt_lib/*.hpp" )
set( LEX_SYNT_LIB_SOURCES_GENERATED
	${CMAKE_CURRENT_BINARY_DIR}/keywords.hpp
	${CMAKE_CURRENT_BINARY_DIR}/keywords.cpp
	)
add_library( LexSyntLib ${LEX_SYNT_LIB_SOURCES} ${LEX_SYNT_LIB_HEADERS} ${LEX_SYNT_LIB_SOURCES_GENERATED} )
target_link_libraries( LexSyntLib LexSyntLibCommon )
target_include_directories( LexSyntLib PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/lex_synt_lib )

#
# CodeBuilderLib
#

file( GLOB COMPILER_LIB_SOURCES "code_builder_lib/*" )
add_library( CodeBuilderLib ${COMPILER_LIB_SOURCES} )
target_link_libraries( CodeBuilderLib LexSyntLib SpracheVersion CodeBuilderLibCommon )

#
# Compiler
#
if( U_BUILD_COMPILER )
	file( GLOB COMPILER_SOURCES "compiler/*" )

	add_executable( Compiler ${COMPILER_SOURCES} )
	target_link_libraries( Compiler CodeBuilderLib CodeBuilderLib CompilersCommonLib )

	install( PROGRAMS $<TARGET_FILE:Compiler> TYPE BIN RENAME u.._compiler${CMAKE_EXECUTABLE_SUFFIX} )

endif()

#
# Tests
#
if( U_BUILD_TESTS )
	#
	# C++ tests launcher
	#
	file( GLOB TESTS_LAUNCHER0_SOURCES "tests/*" )
	add_executable( Tests ${TESTS_LAUNCHER0_SOURCES} )
	target_link_libraries( Tests CodeBuilderLib ${CPP_TESTS_LIB_WRAPPED} )
	add_custom_command( TARGET Tests POST_BUILD COMMAND Tests )

	if( MSVC )
		set_target_properties( Tests PROPERTIES LINK_FLAGS /WHOLEARCHIVE:CppTestsLib )
	endif()
endif()

#
# PyTests
#
if( U_BUILD_PY_TESTS )
	message( STATUS "Build py_tests" )

	add_library( PyTestsLib SHARED py_tests_lib/py_tests_lib.cpp )
	target_link_libraries( PyTestsLib PRIVATE CodeBuilderLib ${PYTHON_LIBRARIES} )
	target_include_directories( PyTestsLib PRIVATE ${PYTHON_INCLUDE_DIRS} )

	# Set proper name for result dynamic library.
	if( WIN32 )
		set_target_properties( PyTestsLib
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.pyd PREFIX "" SUFFIX "" )
	else()
		set_target_properties( PyTestsLib
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.so PREFIX "" SUFFIX "" )
	endif()

	file( GLOB PY_TESTS_SOURCES "../tests/py_tests/*.py" ) # Add all python files to sources list of target.

	# Run result test.
	add_custom_target(
		PyTestsRun ALL
		COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tests/py_tests/py_tests.py
		DEPENDS PyTestsLib
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		SOURCES ${PY_TESTS_SOURCES} )
endif()
