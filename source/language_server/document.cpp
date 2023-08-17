#include "../compiler0/lex_synt_lib/syntax_analyzer.hpp"
#include "../code_builder_lib_common/source_file_contents_hash.hpp"
#include "document.hpp"

namespace U
{

namespace LangServer
{

Document::Document( std::string text )
	: text_( std::move(text) )
{}

void Document::SetText( std::string text )
{
	if( text == text_ )
		return;

	text_= text;

	auto lex_result= LexicalAnalysis( text_ );
	if( !lex_result.errors.empty() )
		return;

	lexems_= std::move( lex_result.lexems );

	// TODO - parse imports and read files or request another opended documents.
	// TODO - provide options for import directories.
	// TODO - fill macros from imported files.

	auto synt_result=
		Synt::SyntaxAnalysis(
			lexems_,
			Synt::MacrosByContextMap(),
			std::make_shared<Synt::MacroExpansionContexts>(),
			CalculateSourceFileContentsHash( text_ ) );

	if( !synt_result.error_messages.empty() )
		return;

	// TODO - add also generated prelude.

	program_elements_= std::move(synt_result.program_elements);
}

} // namespace LangServer

} // namespace U
