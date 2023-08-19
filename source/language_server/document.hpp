#pragma once
#include "../compiler0/lex_synt_lib/lexical_analyzer.hpp"
#include "../compiler0/lex_synt_lib/syntax_elements.hpp"
#include "../code_builder_lib_common/code_builder_errors.hpp"
#include "../lex_synt_lib/source_graph_loader.hpp"

namespace U
{

namespace LangServer
{

class Document
{
public:
	explicit Document( std::string text );

	Document( const Document& )= delete;
	Document( Document&& )= default;
	Document& operator=( const Document& )= delete;
	Document& operator=( Document&& )= default;

	void SetText( std::string text );

	LexSyntErrors GetLexErrors() const;
	LexSyntErrors GetSyntErrors() const;
	CodeBuilderErrorsContainer GetCodeBuilderErrors() const;

	// TODO - return also URI for file
	std::optional<SrcLoc> GetDefinitionPoint( const SrcLoc& src_loc );

private:
	struct CompiledState
	{
		Lexems lexems;
		SourceGraph source_graph;
	};

private:
	std::string text_;
	std::optional<CompiledState> last_valid_state_;
	LexSyntErrors lex_errors_;
	LexSyntErrors synt_errors_;
	CodeBuilderErrorsContainer code_builder_errors_;
};

using DocumentURI= std::string;

} // namespace LangServer

} // namespace U
