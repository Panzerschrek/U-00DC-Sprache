cmake_minimum_required(VERSION 2.8)

add_subdirectory( bnf-plus-plus )

project(grammar_validator)
set( CMAKE_CXX_STANDARD 11 )

set( SOURCES
	main.cpp )

add_custom_command(
	COMMAND bnf-plus-plus/bnf_plus_plus  ${CMAKE_SOURCE_DIR}/grammar.bnf --with-template --templates-dir=${CMAKE_SOURCE_DIR}/bnf-plus-plus/templates
	DEPENDS bnf_plus_plus grammar.bnf
	OUTPUT ${CMAKE_BINARY_DIR}/u_sprache_parser.h )

add_executable( u_sprache_grammar_validator ${SOURCES} ${CMAKE_BINARY_DIR}/u_sprache_parser.h  )
add_dependencies( u_sprache_grammar_validator bnf_plus_plus )
target_include_directories( u_sprache_grammar_validator PRIVATE . )
target_include_directories( u_sprache_grammar_validator PRIVATE ${CMAKE_BINARY_DIR} )
