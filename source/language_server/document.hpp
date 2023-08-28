#pragma once
#include <ostream>
#include "../compiler0/code_builder_lib/code_builder.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"
#include "document_symbols.hpp"
#include "uri.hpp"

namespace U
{

namespace LangServer
{

struct DocumentBuildOptions
{
	llvm::DataLayout data_layout;
	llvm::Triple target_triple;
	std::string prelude;
};

class Document
{
public:
	Document( IVfs::Path path, DocumentBuildOptions build_options, IVfs& vfs, std::ostream& log );

	Document( const Document& )= delete;
	Document( Document&& )= default;
	Document& operator=( const Document& )= delete;
	Document& operator=( Document&& )= default;

	void SetText( std::string text );
	const std::string& GetText() const;

	LexSyntErrors GetLexErrors() const;
	LexSyntErrors GetSyntErrors() const;
	CodeBuilderErrorsContainer GetCodeBuilderErrors() const;

	std::optional<PositionInDocument> GetDefinitionPoint( const SrcLoc& src_loc );

	// Returns highlights only for this document.
	std::vector<DocumentRange> GetHighlightLocations( const SrcLoc& src_loc );

	std::vector<PositionInDocument> GetAllOccurrences( const SrcLoc& src_loc );

	std::vector<Symbol> GetSymbols();

	std::vector<std::string> Complete( const SrcLoc& src_loc );

	std::optional<DocumentPosition> GetIdentifierEndPosition( const DocumentPosition& start_position ) const;

private:
	void Rebuild();

private:
	struct CompiledState
	{
		LineToLinearPositionIndex line_to_linear_position_index;
		SourceGraph source_graph;
		std::unique_ptr<llvm::LLVMContext> llvm_context;
		std::unique_ptr<CodeBuilder> code_builder;
	};

private:
	const IVfs::Path path_;
	const DocumentBuildOptions build_options_;
	IVfs& vfs_;
	std::ostream& log_;
	std::string text_;
	std::optional<CompiledState> last_valid_state_;
	LexSyntErrors lex_errors_;
	LexSyntErrors synt_errors_;
	CodeBuilderErrorsContainer code_builder_errors_;
};

} // namespace LangServer

} // namespace U
