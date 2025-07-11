if( TARGET CppHeaderConverter AND U_BUILD_CPP_HEADER_CONVERTER )

	if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
		set( SPRACHE_COMPILER_OPT_OPTIONS -O0 -g )
	else()
		set( SPRACHE_COMPILER_OPT_OPTIONS -O2 )
	endif()

	set( CPP_HEADER_CONVERTER_SYSROOT_OPTION "" )
	if( NOT ${CMAKE_OSX_SYSROOT} STREQUAL "" )
		set( CPP_HEADER_CONVERTER_SYSROOT_OPTION --sysroot ${CMAKE_OSX_SYSROOT} )
	endif()

	#
	# Basic CppHeaderConverter test - for a manually-crafter header file.
	#

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/c_converted.u
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/c_test.h CppHeaderConverter
		COMMAND
			CppHeaderConverter -o ${CMAKE_CURRENT_BINARY_DIR}/c_converted.u
			${CMAKE_CURRENT_SOURCE_DIR}/c_test.h -- -std=c2x )

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cpp_header_converter_test.o
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cpp_header_converter_test.u ${CMAKE_CURRENT_BINARY_DIR}/c_converted.u Compiler
		COMMAND
			Compiler${CURRENT_COMPILER_GENERATION}
			${CMAKE_CURRENT_SOURCE_DIR}/cpp_header_converter_test.u -o ${CMAKE_CURRENT_BINARY_DIR}/cpp_header_converter_test.o
			${SPRACHE_COMPILER_PIC_OPTIONS}
			${SPRACHE_COMPILER_OPT_OPTIONS}
			--allow-unused-names # Allow unused variables - in this test we only check initialization for them.
			--verify-module
			--include-dir ${CMAKE_CURRENT_BINARY_DIR} )

	add_executable(
		CppHeaderConverterTest${CURRENT_COMPILER_GENERATION}
			${CMAKE_CURRENT_BINARY_DIR}/cpp_header_converter_test.o
			${CMAKE_CURRENT_SOURCE_DIR}/cpp_header_converter_test.u
			${CMAKE_CURRENT_SOURCE_DIR}/c_test.cpp )
	add_dependencies( CppHeaderConverterTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

	# Run the test
	add_custom_command( TARGET CppHeaderConverterTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND CppHeaderConverterTest${CURRENT_COMPILER_GENERATION} )

	if(WIN32)

		#
		# A test for convertion of <windows.h> and its usage for a simple program.
		#

		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/windows_headers.u
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/windows_headers.h CppHeaderConverter
			COMMAND
				CppHeaderConverter -o ${CMAKE_CURRENT_BINARY_DIR}/windows_headers.u
				${CMAKE_CURRENT_SOURCE_DIR}/windows_headers.h -- )

		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/windows_headers_test.o
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/windows_headers_test.u ${CMAKE_CURRENT_BINARY_DIR}/windows_headers.u Compiler
			COMMAND
				Compiler${CURRENT_COMPILER_GENERATION}
				${CMAKE_CURRENT_SOURCE_DIR}/windows_headers_test.u -o ${CMAKE_CURRENT_BINARY_DIR}/windows_headers_test.o
				${SPRACHE_COMPILER_PIC_OPTIONS}
				${SPRACHE_COMPILER_OPT_OPTIONS}
				--verify-module
				--include-dir ${CMAKE_CURRENT_BINARY_DIR} )

		add_executable(
			WindowsHeadersConversionTest${CURRENT_COMPILER_GENERATION}
				${CMAKE_CURRENT_BINARY_DIR}/windows_headers_test.o
				${CMAKE_CURRENT_SOURCE_DIR}/windows_headers_test.u
				${CMAKE_CURRENT_SOURCE_DIR}/windows_headers.h
				../dummy.cpp )
		add_dependencies( WindowsHeadersConversionTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

	else()

		#
		# A test for convertion of <unistd.h> and its usage for a simple program.
		#

		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/unistd_headers.u
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/unistd_headers.h CppHeaderConverter
			COMMAND
				CppHeaderConverter -o ${CMAKE_CURRENT_BINARY_DIR}/unistd_headers.u
				${CMAKE_CURRENT_SOURCE_DIR}/unistd_headers.h --
				-I${CLANG_BUILTIN_HEADERS_FOR_CPP_HEADER_CONVERTER} ${CPP_HEADER_CONVERTER_SYSROOT_OPTION}
			COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/unistd_headers.u
				)

		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/unistd_headers_test.o
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/unistd_headers_test.u ${CMAKE_CURRENT_BINARY_DIR}/unistd_headers.u Compiler
			COMMAND
				Compiler${CURRENT_COMPILER_GENERATION}
				${CMAKE_CURRENT_SOURCE_DIR}/unistd_headers_test.u -o ${CMAKE_CURRENT_BINARY_DIR}/unistd_headers_test.o
				${SPRACHE_COMPILER_PIC_OPTIONS}
				${SPRACHE_COMPILER_OPT_OPTIONS}
				--allow-unused-names # Allow unused variables - in this test we only check initialization for them.
				--verify-module
				--include-dir ${CMAKE_CURRENT_BINARY_DIR} )

		add_executable(
			UnistdHeadersConversionTest${CURRENT_COMPILER_GENERATION}
				${CMAKE_CURRENT_BINARY_DIR}/unistd_headers_test.o
				${CMAKE_CURRENT_SOURCE_DIR}/unistd_headers_test.u
				${CMAKE_CURRENT_SOURCE_DIR}/unistd_headers.h
				../dummy.cpp )
		add_dependencies( UnistdHeadersConversionTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

		# Run the test
		add_custom_command( TARGET UnistdHeadersConversionTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND UnistdHeadersConversionTest${CURRENT_COMPILER_GENERATION} )

	endif()

	#
	# Test for converting almost the whole C standard library.
	#

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/c_standard_library_headers.u
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/c_standard_library_headers.h CppHeaderConverter
		COMMAND
			CppHeaderConverter -o ${CMAKE_CURRENT_BINARY_DIR}/c_standard_library_headers.u
			${CMAKE_CURRENT_SOURCE_DIR}/c_standard_library_headers.h --
			-Wno-implicit-function-declaration
			-I${CLANG_BUILTIN_HEADERS_FOR_CPP_HEADER_CONVERTER} ${CPP_HEADER_CONVERTER_SYSROOT_OPTION}
			)

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/c_standard_library_headers_test.o
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/c_standard_library_headers_test.u ${CMAKE_CURRENT_BINARY_DIR}/c_standard_library_headers.u Compiler
		COMMAND
			Compiler${CURRENT_COMPILER_GENERATION}
			${CMAKE_CURRENT_SOURCE_DIR}/c_standard_library_headers_test.u -o ${CMAKE_CURRENT_BINARY_DIR}/c_standard_library_headers_test.o
			${SPRACHE_COMPILER_PIC_OPTIONS}
			${SPRACHE_COMPILER_OPT_OPTIONS}
			--allow-unused-names # Allow unused variables - in this test we only check initialization for them.
			--verify-module
			--include-dir ${CMAKE_CURRENT_BINARY_DIR} )

	add_executable(
		CStandardLibraryHeadersConversionTest${CURRENT_COMPILER_GENERATION}
			${CMAKE_CURRENT_BINARY_DIR}/c_standard_library_headers_test.o
			${CMAKE_CURRENT_SOURCE_DIR}/c_standard_library_headers_test.u
			${CMAKE_CURRENT_SOURCE_DIR}/c_standard_library_headers.h
			../dummy.cpp )
	add_dependencies( CStandardLibraryHeadersConversionTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

	# Run the test
	add_custom_command( TARGET CStandardLibraryHeadersConversionTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND CStandardLibraryHeadersConversionTest${CURRENT_COMPILER_GENERATION} )

endif()
