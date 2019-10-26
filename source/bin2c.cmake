if(${CMAKE_ARGC} LESS 6)
	message( FATAL_ERROR "Not enough arguments. Usage: cmake -P bin2c.cmake <in_file_path> <out_file_path> <out_variable_name>" )
endif()

set( IN_FILE_PATH ${CMAKE_ARGV3} )
set( OUT_FILE_PATH ${CMAKE_ARGV4} )
set( VAR_NAME ${CMAKE_ARGV5} )

file( READ ${IN_FILE_PATH} FILE_CONTENT HEX )
string( REGEX REPLACE "([a-fA-F0-9])([a-fA-F0-9])" "0x\\1\\2u,\n" HEX_LIST ${FILE_CONTENT} )
set( OUT_FILE_CONTENT "static const uint8_t ${VAR_NAME}[]={\n${HEX_LIST}}\;" )
file(WRITE ${OUT_FILE_PATH} ${OUT_FILE_CONTENT} )
