#include "../compilers_support_lib/prelude.hpp"
#include "../compilers_support_lib/vfs.hpp"
#include "../tests/tests_common.hpp"
#include "options.hpp"
#include "document_manager.hpp"

namespace U
{

namespace LangServer
{

namespace
{

std::unique_ptr<IVfs> CreateBaseVfs( std::ostream& log )
{
	auto vfs_with_includes= CreateVfsOverSystemFS( Options::include_dir );
	if( vfs_with_includes != nullptr )
		return vfs_with_includes;

	log << "Failed to create VFS." << std::endl;

	// Something went wrong. Create fallback.
	return CreateVfsOverSystemFS( {} );
}

DocumentBuildOptions CreateBuildOptions()
{
	DocumentBuildOptions build_options
	{
		// TODO - create proper target machine.
		llvm::DataLayout( GetTestsDataLayout() ),
		// TODO - use target triple, dependent on compilation options.
		llvm::Triple( llvm::sys::getDefaultTargetTriple() ),
		"",
	};

	// TODO - read params from options or some kind of config file.
	const llvm::StringRef features;
	const llvm::StringRef cpu_name;
	const char optimization_level= '0';
	const bool generate_debug_info= 0;
	const uint32_t compiler_generation= 0;
	build_options.prelude=
		GenerateCompilerPreludeCode(
			build_options.target_triple,
			build_options.data_layout,
			features,
			cpu_name,
			optimization_level,
			generate_debug_info,
			compiler_generation );

	return build_options;
}

} // namespace

DocumentManager::DocumentManagerVfs::DocumentManagerVfs( DocumentManager& document_manager, std::ostream& log )
	: document_manager_(document_manager)
	, base_vfs_( CreateBaseVfs( log ) )
{
}

std::optional<IVfs::FileContent> DocumentManager::DocumentManagerVfs::LoadFileContent( const Path& full_file_path )
{
	const Uri file_uri= Uri::FromFilePath( full_file_path );

	if( const auto it= document_manager_.documents_.find( file_uri ); it != document_manager_.documents_.end() )
		return it->second.GetText();
	if( const auto it= document_manager_.unmanaged_files_.find( file_uri ); it != document_manager_.unmanaged_files_.end() )
	{
		// TODO - detect changes in unmanaged files and reload them if it is necessary.
		if( it->second == std::nullopt )
			return nullptr;
		return *it->second;
	}

	// Load unmanaged file.
	document_manager_.log_ << "Load unmanaged file " << full_file_path << std::endl;

	std::optional<IVfs::FileContent>& unmanaged_file= document_manager_.unmanaged_files_[file_uri];

	unmanaged_file= base_vfs_->LoadFileContent( full_file_path );

	if( unmanaged_file == std::nullopt )
		document_manager_.log_ << "Failed to load unmanaged file " << full_file_path << std::endl;

	return unmanaged_file;
}

IVfs::Path DocumentManager::DocumentManagerVfs::GetFullFilePath( const Path& file_path, const Path& full_parent_file_path )
{
	return base_vfs_->GetFullFilePath( file_path, full_parent_file_path );
}

DocumentManager::DocumentManager( std::ostream& log )
	: log_(log)
	// TODO - use individual VFS for different files.
	, vfs_( *this, log_ )
	// TODO - create different build options for different files.
	, build_options_( CreateBuildOptions() )
{}

Document* DocumentManager::Open( const Uri& uri, std::string text )
{
	std::optional<std::string> file_path= uri.AsFilePath();
	if( file_path == std::nullopt )
	{
		log_ << "Can't convert URI into file path!" << std::endl;
		return nullptr;
	}

	unmanaged_files_.erase( uri ); // Now we manage this file.

	// First add docuemnt into a map.
	const auto it_bool_pair=
		documents_.insert(
			std::make_pair(
				uri,
				Document( std::move( *file_path ), build_options_, vfs_, log_ ) ) );

	// Than set text.
	// This is needed, because document itself may be requested during "SetText" call - via DocumentManagerVfs.
	Document& document= it_bool_pair.first->second;
	document.SetText( std::move(text) );

	return &it_bool_pair.first->second;
}

Document* DocumentManager::GetDocument( const Uri& uri )
{
	const auto it= documents_.find( uri );
	if( it == documents_.end() )
		return nullptr;
	return &it->second;
}

void DocumentManager::Close( const Uri& uri )
{
	documents_.erase( uri );
}

} // namespace LangServer

} // namespace U
