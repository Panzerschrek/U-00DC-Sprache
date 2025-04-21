#pragma once
#include "i_vfs.hpp"
#include "syntax_analyzer.hpp"

namespace U
{

// Directed acyclic graph of sources.
struct SourceGraph
{
	struct Node
	{
		enum class Category : uint8_t
		{
			SourceOrInternalImport,
			OtherImport,
			BuiltInPrelude,
		};

		IVfs::Path file_path; // normalized
		std::string file_path_hash;
		std::vector<size_t> child_nodes_indices;
		Synt::SyntaxAnalysisResult ast;
		Category category= Category::SourceOrInternalImport;
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
