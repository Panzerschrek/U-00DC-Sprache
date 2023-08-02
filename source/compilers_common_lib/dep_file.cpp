#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../code_builder_lib_common/string_ref.hpp"
#include "../sprache_version/sprache_version.hpp"
#include "dep_file.hpp"

namespace U
{

namespace DepFile
{

namespace
{

const char c_version[]= "version";
const char c_args[]= "args";
const char c_deps[]= "deps";

const char c_file_prefix[]= ".u_deps";

} // namespace

bool NothingChanged(
	const std::string& out_file_path,
	const int argc, const char* const argv[] )
{
	llvm::sys::fs::file_status out_file_status;
	if( llvm::sys::fs::status( out_file_path, out_file_status ) )
		return false;
	const auto out_file_modification_time= out_file_status.getLastModificationTime();

	const llvm::ErrorOr< std::unique_ptr<llvm::MemoryBuffer> > file_mapped=
		llvm::MemoryBuffer::getFile( out_file_path + c_file_prefix );

	if( !file_mapped || *file_mapped == nullptr )
		return false;

	llvm::Expected<llvm::json::Value> json_parsed= llvm::json::parse( (*file_mapped)->getBuffer() );
	if( !json_parsed || json_parsed->kind() != llvm::json::Value::Object )
		return false;

	const llvm::json::Object& json_root= *json_parsed->getAsObject();

	if( const llvm::json::Value* const version= json_root.get(c_version) )
	{
		if( version->kind() != llvm::json::Value::String )
			return false;
		if( *(version->getAsString()) != StringViewToStringRef( getFullVersion() ) )
			return false;
	}
	else
		return false;

	if( const llvm::json::Value* const args= json_root.get(c_args) )
	{
		if( args->kind() != llvm::json::Value::Array )
			return false;

		const llvm::json::Array& args_array= *args->getAsArray();
		if( args_array.size() != size_t(argc) )
			return false;

		for( int i= 0; i < argc; ++i )
		{
			const llvm::json::Value& arg= args_array[size_t(i)];
			if( arg.kind() != llvm::json::Value::String )
				return false;
			if( *(arg.getAsString()) != argv[i] )
				return false;
		}
	}
	else
		return false;

	if( const llvm::json::Value* const depends= json_root.get(c_deps) )
	{
		if( depends->kind() != llvm::json::Value::Array )
			return false;

		for( const llvm::json::Value& dependency : *(depends->getAsArray()) )
		{
			if( dependency.kind() != llvm::json::Value::String )
				return false;

			llvm::sys::fs::file_status file_status;
			if( llvm::sys::fs::status( *(dependency.getAsString()), file_status ) )
				return false;
			if( file_status.getLastModificationTime() > out_file_modification_time )
				return false;
		}
	}
	else
		return false;

	return true;
}

void Write(
	const std::string& out_file_path,
	const int argc, const char* const argv[],
	const std::vector<IVfs::Path>& deps_list )
{
	llvm::json::Object doc;
	doc[c_version]= std::string( getFullVersion() );

	{
		llvm::json::Array args;
		args.reserve(size_t(argc));
		for( int i= 0; i < argc; ++i )
			args.push_back(argv[i]);
		doc[c_args]= std::move(args);
	}

	{
		llvm::json::Array paths_arr;
		paths_arr.reserve( deps_list.size() );
		for( const IVfs::Path& path : deps_list )
			paths_arr.push_back( path );
		doc[c_deps]= std::move(paths_arr);
	}

	std::error_code file_error_code;
	llvm::raw_fd_ostream out_file_stream( out_file_path + c_file_prefix, file_error_code );

	out_file_stream << llvm::json::Value(std::move(doc));
	out_file_stream.flush();
}

} // namespace DepFile

} // namespace U
