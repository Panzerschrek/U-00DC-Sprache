#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include "u_sprache_parser.h"
#include "u_sprache_parser.cc"


static bool ReadFile( const char* const name, std::string& out_file_content )
{
	std::FILE* const f= std::fopen( name, "rb" );
	if( f == nullptr )
		return false;

	std::fseek( f, 0, SEEK_END );
	const unsigned int file_size= std::ftell( f );
	std::fseek( f, 0, SEEK_SET );

	out_file_content.resize( file_size );

	unsigned int read_total= 0u;
	bool read_error= false;
	do
	{
		const int read= std::fread( static_cast<char*>(&out_file_content[0]) + read_total, 1, file_size - read_total, f );
		if( read < 0 )
		{
			read_error= true;
			break;
		}
		if( read == 0 )
			break;

		read_total+= read;
	} while( read_total < file_size );

	std::fclose(f);

	if( read_error )
		return false;
	return true;
}

int main( const int argc, const char* const argv[] )
{
	if( argc != 2 )
	{
		std::cout << "Invalid arg count. expected single file name" << std::endl;
		return 1;
	}
	const char* const file_name= argv[1u];

	std::string file_content;
	if( !ReadFile( file_name, file_content ) )
	{
		std::cout << "Error, reading file " << file_name << "." << std::endl;
		return 1;
	}

	u_sprache_parser<std::string> parser( file_content, 0u, file_content.size() );

	const auto s= parser.program( 0u );
	if( s == file_content.size() )
	{
		std::cout << "Parse ok" << std::endl;
	}
	else
	{
		std::cout << "Parse not ok. " << "Parsed " << s << " of " << file_content.size() << " bytes." << std::endl;
	}

	return 0;
}
