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
set( LEX_SYNT_LIB_SOURCES_GENERATED ${CMAKE_CURRENT_BINARY_DIR}/keywords.hpp ${CMAKE_CURRENT_BINARY_DIR}/keywords.cpp )
add_library( LexSyntLib ${LEX_SYNT_LIB_SOURCES} ${LEX_SYNT_LIB_HEADERS} ${LEX_SYNT_LIB_SOURCES_GENERATED} )
target_include_directories( LexSyntLib PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/lex_synt_lib )

#
# CodeBuilderLib
#

file( GLOB COMPILER_LIB_SOURCES "code_builder_lib/*" )
add_library( CodeBuilderLib ${COMPILER_LIB_SOURCES} )
target_link_libraries( CodeBuilderLib LexSyntLib SpracheVersion CompilersCommonLib ${LLVM_LIBS_FOR_CODE_BUILDER_LIB} )

#
# Compiler
#
if( U_BUILD_COMPILER )
	file( GLOB COMPILER_SOURCES "compiler/*" )

	function( CompileLLFile ll_file )
		get_filename_component( source_name ${ll_file} NAME_WE )
		set( bc_name ${source_name}.bc )
		set( bc_header_path ${CMAKE_CURRENT_BINARY_DIR}/${source_name}.h )
		add_custom_command(
			OUTPUT ${bc_name}
			DEPENDS ${CMAKE_SOURCE_DIR}/ustlib/src/${ll_file}
			DEPENDS llvm-as
			COMMAND llvm-as ${CMAKE_SOURCE_DIR}/ustlib/src/${ll_file} -o ${bc_name} )
		add_custom_command(
			OUTPUT ${bc_header_path}
			DEPENDS ${bc_name}
			DEPENDS ${CMAKE_SOURCE_DIR}/bin2c.cmake ${bc_name}
			COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/bin2c.cmake ${bc_name} ${bc_header_path} c_${source_name}_file_content
			)
		set( BC_FILE ${bc_header_path} PARENT_SCOPE )
	endfunction()

	CompileLLFile( asm_funcs.ll )
	list( APPEND bc_files ${BC_FILE} )
	CompileLLFile( asm_funcs_32.ll )
	list( APPEND bc_files ${BC_FILE} )
	CompileLLFile( asm_funcs_64.ll )
	list( APPEND bc_files ${BC_FILE} )

	# Show in editor also ustlib and ustlib tests files
	file( GLOB_RECURSE USTLIB_SOURCES "ustlib/*.u" "ustlib/src/*.ll" )
	file( GLOB_RECURSE USTLIB_TEST_SOURCES "ustlib_test/*.u" )

	add_executable( Compiler ${COMPILER_SOURCES} ${bc_files} ${USTLIB_SOURCES} ${USTLIB_TEST_SOURCES} )
	target_link_libraries( Compiler CodeBuilderLib ${LLVM_LIBS_FOR_COMPILER} )
	target_include_directories( Compiler PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )
endif()


#
# CppHeaderConverter
#
if( REALLY_BUILD_CPP_HEADER_CONVERTER )
	file( GLOB CPP_HEADER_CONVERTER_SOURCES "cpp_header_converter/*" )
	add_executable( CppHeaderConverter ${CPP_HEADER_CONVERTER_SOURCES} )
	target_link_libraries( CppHeaderConverter LexSyntLib clangTooling )
	# Hack! Clang libraries does not exports their include directories, set it manually.
	target_include_directories( CppHeaderConverter PRIVATE ${LLVM_EXTERNAL_CLANG_SOURCE_DIR}/include )
	target_include_directories( CppHeaderConverter PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/llvm/tools/clang/include )
endif()

#
# Tests
#
if( U_BUILD_TESTS )
	#
	# C++ tests launcher
	#
	file( GLOB TESTS_LAUNCHER0_SOURCES "cpp_tests_launcher0/*" )
	add_executable( Tests ${TESTS_LAUNCHER0_SOURCES} )
	target_link_libraries( Tests CodeBuilderLib ${TESTS_LIB_WRAPPED} )
	add_custom_command( TARGET Tests POST_BUILD COMMAND Tests )

	if( MSVC )
		set_target_properties( Tests PROPERTIES LINK_FLAGS /WHOLEARCHIVE:TestsLib )
	endif()
endif()

#
# PyTests
#
if( U_BUILD_PY_TESTS )
	add_library( PyTestsLib SHARED py_tests_lib/py_tests_lib.cpp )
	target_link_libraries( PyTestsLib CodeBuilderLib ${LLVM_LIBS_FOR_TESTS} ${PYTHON_LIBRARIES} )
	target_include_directories( PyTestsLib PRIVATE ${PYTHON_INCLUDE_DIRS} )

	# Set proper name for result dynamic library.
	if( WIN32 )
		set_target_properties( PyTestsLib
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.pyd PREFIX "" SUFFIX "" )
	else()
		set_target_properties( PyTestsLib
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.so PREFIX "" SUFFIX "" )
	endif()

	file( GLOB PY_TESTS_SOURCES "../../tests/py_tests/*.py" ) # Add all python files to sources list of target.

	# Run result test.
	add_custom_target(
		PyTestsRun ALL
		COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} ${PYTHON_EXECUTABLE} tests/py_tests/py_tests.py
		DEPENDS PyTestsLib
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		SOURCES ${PY_TESTS_SOURCES} )
endif()
