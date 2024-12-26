#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

int main( const int argc, const char* const argv[] )
{
	std::vector<const char*> input_files;
	const char* output_file_path= nullptr;
	int32_t num_repeats= 1;

	for( int i= 1; i < argc; )
	{
		const std::string_view arg( argv[i], std::strlen(argv[i]) );
		if( arg == "-o" )
		{
			if( i + 1 >= argc )
			{
				std::cerr << "Error, expected an argument afeter \"-o\"!" << std::endl;
				return -1;
			}
			if( output_file_path != nullptr )
			{
				std::cerr << "Duplicated output file \"" << argv[i + 1] << std::endl;
				return -1;
			}
			output_file_path= argv[i + 1];
			i+= 2;
		}
		else if( arg == "-n" )
		{
			if( i + 1 >= argc )
			{
				std::cerr << "Error, expected an argument afeter \"-n\"!" << std::endl;
				return -1;
			}
			num_repeats= std::atoi( argv[i + 1] );
			if( num_repeats <= 0 )
			{
				std::cerr << "Failed to parse number \"" << argv[i + 1] << "\"!" << std::endl;
				return -1;
			}
			i+= 2;
		}
		else
		{
			input_files.push_back( argv[i] );
			++i;
		}
	}

	if( input_files.empty() )
	{
		std::cerr << "No input files!" << std::endl;
		return -1;
	}

	if( output_file_path == nullptr )
	{
		std::cerr << "No output file!" << std::endl;
		return -1;
	}

	if( num_repeats >= 128 )
	{
		std::cerr << "Number of repeats is too high!" << std::endl;
		return -1;
	}

	std::vector<std::string> in_files_contents;
	in_files_contents.reserve( input_files.size() );

	for( const auto input_file : input_files )
	{
		std::ifstream in_file( input_file, std::ios::binary );
		if( in_file.fail() )
		{
			std::cerr << "Failed to open file \"" << input_file << "\" for reading!" << std::endl;
			return -1;
		}

		in_file.seekg( 0, std::ios::end );
		const auto size = in_file.tellg();
		if( size < 0 )
		{
			std::cerr << "Failed to get file \"" << input_file << "\"size!" << std::endl;
			return -1;
		}

		std::string in_file_conents;

		in_file_conents.resize( size_t(size) );
		in_file.seekg(0);
		in_file.read( in_file_conents.data(), std::streamsize( size ) );

		if( in_file.fail() )
		{
			std::cerr << "Faild to read from \"" << input_file << "\"!" << std::endl;
			return -1;
		}

		in_files_contents.push_back( std::move(in_file_conents) );
	}

	std::ofstream out_file( output_file_path, std::ios::binary );
	if( out_file.fail() )
	{
		std::cerr << "Failed to open file \"" << output_file_path << "\" for writing!" << std::endl;
		return -1;
	}

	for( int32_t i= 0; i < num_repeats && !out_file.fail(); ++i )
	{
		for( const std::string& in_file_contents : in_files_contents )
			out_file << in_file_contents;
	}

	if( out_file.fail() )
	{
		std::cerr << "Faild to write into \"" << output_file_path << "\"!" << std::endl;
		return -1;
	}

	return 0;
}
