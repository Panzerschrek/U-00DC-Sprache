#pragma once
#include <memory>
#include <boost/optional/optional.hpp>

#include "program_string.hpp"
#include "syntax_analyzer.hpp"

namespace U
{

class IVfs
{
public:
	typedef ProgramString Path;

	virtual ~IVfs()= default;

	virtual boost::optional<ProgramString> LoadFileContent( const Path& path )= 0;
	virtual Path NormalizePath( const Path& path )= 0;
};

typedef  std::shared_ptr<IVfs> IVfsPtr;

// Directed acyclic graph of sources.
struct SourceGraph final
{
	struct Node
	{
		IVfs::Path file_path; // normalized
		std::vector<size_t> child_nodes_indeces;
		SyntaxAnalysisResult ast;
		// Here can be placed cached module.
	};

	std::vector<Node> nodes_storage;
	size_t root_node_index= ~0u;

	LexicalErrorMessages lexical_errors;
	SyntaxErrorMessages syntax_errors;
};

typedef std::unique_ptr<SourceGraph> SourceGraphPtr;

class SourceGraphLoader final
{
public:
	explicit SourceGraphLoader( IVfsPtr vfs );

	// Never returns nullptr.
	SourceGraphPtr LoadSource( const IVfs::Path& root_file_path );

private:
	size_t LoadNode_r( const IVfs::Path& file_path, SourceGraph& result );

private:
	IVfsPtr vfs_;

	std::vector<IVfs::Path> processed_files_stack_;
};

} // namespace U
