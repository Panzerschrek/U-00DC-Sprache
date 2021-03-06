#pragma once
#include <memory>
#include "syntax_analyzer.hpp"

namespace U
{

class IVfs
{
public:
	using Path= std::string;
	using FileContent= std::string;

	virtual ~IVfs()= default;

	virtual std::optional<FileContent> LoadFileContent( const Path& full_file_path)= 0;

	// Empty "full_parent_file_path" means root file.
	virtual Path GetFullFilePath( const Path& file_path, const Path& full_parent_file_path )= 0;
};

using IVfsPtr= std::shared_ptr<IVfs>;

// Directed acyclic graph of sources.
struct SourceGraph final
{
	struct Node
	{
		IVfs::Path file_path; // normalized
		std::vector<size_t> child_nodes_indeces;
		Synt::SyntaxAnalysisResult ast;
		// Here can be placed cached module.
	};

	std::vector<Node> nodes_storage; // first element is root

	// Use common storage of macro expansion context because macro declared in one file may be used in another one.
	Synt::MacroExpansionContextsPtr macro_expansion_contexts;

	LexSyntErrors errors;
};

using SourceGraphPtr= std::unique_ptr<SourceGraph>;

class SourceGraphLoader final
{
public:
	explicit SourceGraphLoader( IVfsPtr vfs );

	// Never returns nullptr.
	SourceGraphPtr LoadSource( const IVfs::Path& root_file_path );

private:
	size_t LoadNode_r( const IVfs::Path& file_path, const IVfs::Path& parent_file_path, SourceGraph& result );

private:
	const Synt::MacrosPtr built_in_macros_;
	const IVfsPtr vfs_;

	std::vector<IVfs::Path> processed_files_stack_;
};

} // namespace U
