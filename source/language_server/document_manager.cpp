#include "../compilers_support_lib/vfs.hpp"
#include "document_manager.hpp"

namespace U
{

namespace LangServer
{

DocumentManager::DocumentManagerVfs::DocumentManagerVfs( DocumentManager& document_manager )
	: document_manager_(document_manager)
	// TODO - specify include directories.
	, base_vfs_( CreateVfsOverSystemFS( {} ) )
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

	return unmanaged_file;
}

IVfs::Path DocumentManager::DocumentManagerVfs::GetFullFilePath( const Path& file_path, const Path& full_parent_file_path )
{
	return base_vfs_->GetFullFilePath( file_path, full_parent_file_path );
}

DocumentManager::DocumentManager( std::ostream& log )
	: log_(log)
	// TODO - use individual VFS for different files.
	, vfs_( *this )
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
				Document( std::move( *file_path ), vfs_, log_ ) ) );

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
