if( TARGET CppHeaderConverter AND U_BUILD_CPP_HEADER_CONVERTER )
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/c_converted.u
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/c_test.h CppHeaderConverter
		COMMAND
			CppHeaderConverter -o ${CMAKE_CURRENT_BINARY_DIR}/c_converted.u
			${CMAKE_CURRENT_SOURCE_DIR}/c_test.h -- -std=c11 )

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cpp_header_converter_test.o
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cpp_header_converter_test.u ${CMAKE_CURRENT_BINARY_DIR}/c_converted.u Compiler
		COMMAND
			Compiler${CURRENT_COMPILER_GENERATION}
			${CMAKE_CURRENT_SOURCE_DIR}/cpp_header_converter_test.u -o ${CMAKE_CURRENT_BINARY_DIR}/cpp_header_converter_test.o
			${SPRACHE_COMPILER_PIC_OPTIONS}
			--allow-unused-names # Allow unused variables - in this test we only check initialization for them.
			--verify-module
			--include-dir ${CMAKE_CURRENT_BINARY_DIR} )

	add_executable(
		CppHeaderConverterTest${CURRENT_COMPILER_GENERATION}
			${CMAKE_CURRENT_BINARY_DIR}/cpp_header_converter_test.o
			${CMAKE_CURRENT_SOURCE_DIR}/cpp_header_converter_test.u
			${CMAKE_CURRENT_SOURCE_DIR}/c_test.cpp )
	add_dependencies( CppHeaderConverterTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )
endif()
