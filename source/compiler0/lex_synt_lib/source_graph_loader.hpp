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

// Directed acyclic graph of sources.
struct SourceGraph
{
	struct Node
	{
		IVfs::Path file_path; // normalized
		std::string contents_hash;
		std::vector<size_t> child_nodes_indeces;
		Synt::SyntaxAnalysisResult ast;
		// Here can be placed cached module.
	};

	std::vector<Node> nodes_storage; // first element is root

	// Use common storage of macro expansion context because macro declared in one file may be used in another one.
	Synt::MacroExpansionContextsPtr macro_expansion_contexts;

	LexSyntErrors errors;
};

using SourceFileContentsHashigFunction= std::string(*)( std::string_view );

SourceGraph LoadSourceGraph(
	IVfs& vfs,
	SourceFileContentsHashigFunction source_file_contents_hashing_function,
	const IVfs::Path& root_file_path,
	std::string_view prelude_code = "" );

} // namespace U
