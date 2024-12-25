#include <cstring>
#include <fstream>
#include <iostream>

int main( const int argc, const char* const argv[] )
{
	const char* input_file_path= nullptr;
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
			if( input_file_path != nullptr )
			{
				std::cerr << "Duplicated input file \"" << argv[i] << "\"!" << std::endl;
				return -1;
			}
			input_file_path= argv[i];
			++i;
		}
	}

	if( input_file_path == nullptr )
	{
		std::cerr << "No input file!" << std::endl;
		return -1;
	}

	if( output_file_path == nullptr )
	{
		std::cerr << "No output file!" << std::endl;
		return -1;
	}

	if( num_repeats == 128 )
	{
		std::cerr << "Number of repeats is too heigh!" << std::endl;
		return -1;
	}

	std::cout << "Read file \"" << input_file_path << "\" and repeat it " << num_repeats << " times into \"" << output_file_path << "\"." << std::endl;

	std::string in_file_contents;

	{
		std::ifstream in_file( input_file_path, std::ios::binary );
		if( in_file.fail() )
		{
			std::cerr << "Failed to open file \"" << input_file_path << "\" for reading!" << std::endl;
			return -1;
		}

		in_file.seekg( 0, std::ios::end );
		const ssize_t size = in_file.tellg();
		if( size < 0 )
		{
			std::cerr << "Failed to get file \"" << input_file_path << "\"size!" << std::endl;
			return -1;
		}

		in_file_contents.resize( size_t(size) );
		in_file.seekg(0);
		in_file.read( in_file_contents.data(), size );

		if( in_file.fail() )
		{
			std::cerr << "Faild to read from \"" << input_file_path << "\"!" << std::endl;
			return -1;
		}
	}

	std::ofstream out_file( output_file_path, std::ios::binary );
	if( out_file.fail() )
	{
		std::cerr << "Failed to open file \"" << output_file_path << "\" for writing!" << std::endl;
		return -1;
	}

	for( int32_t i= 0; i < num_repeats && !out_file.fail(); ++i )
		out_file << in_file_contents;

	if( out_file.fail() )
	{
		std::cerr << "Faild to write into \"" << output_file_path << "\"!" << std::endl;
		return -1;
	}

	return 0;
}
