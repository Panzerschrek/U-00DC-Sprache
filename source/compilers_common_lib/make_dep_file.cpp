#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"


#include "make_dep_file.hpp"

namespace U
{

namespace
{

std::string QuoteDepTargetString( const std::string& str )
{
	std::string result;
	result.reserve( str.size() * 2u );

	for( const char c : str )
	{
		if( c == ' ' || c == '\t' || c == '\\' || c == '#' )
			result.push_back('\\');
		if( c == '$' )
			result.push_back('$');
		result.push_back(c);
	}

	return result;
}

} // namespace

void DeduplicateAndFilterDepsList( std::vector<IVfs::Path>& deps_list )
{
	std::sort( deps_list.begin(), deps_list.end() );
	deps_list.erase( std::unique( deps_list.begin(), deps_list.end() ), deps_list.end() );

	// Remove prelude pseudo-file from dependencies list.
	for( size_t i= 0; i < deps_list.size(); )
	{
		if( deps_list[i] == "compiler_generated_prelude" )
		{
			if( i + 1 < deps_list.size() )
				deps_list[i]= std::move(deps_list.back());
			deps_list.pop_back();
		}
		else
			++i;
	}
}

bool WriteDepFile(
	const std::string& out_file_path,
	const std::vector<IVfs::Path>& deps_list, // Files list should not contain duplicates.
	const std::string& dep_file_path )
{
	std::string str= QuoteDepTargetString(out_file_path) + ":";
	for( const IVfs::Path& path : deps_list )
	{
		str+= " ";
		str+= QuoteDepTargetString(path);
		if( &path != &deps_list.back() )
			str+= " \\\n";
	}

	std::error_code file_error_code;
	llvm::raw_fd_ostream deps_file_stream( dep_file_path, file_error_code );
	deps_file_stream << str;

	deps_file_stream.flush();
	if( deps_file_stream.has_error() )
	{
		std::cerr << "Error while writing dep file \"" << dep_file_path << "\": " << file_error_code.message() << std::endl;
		return false;
	}

	return true;
}

}
