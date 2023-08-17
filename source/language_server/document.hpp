#pragma once
#include <string>
#include "../compiler0/lex_synt_lib/lexical_analyzer.hpp"
#include "../compiler0/lex_synt_lib/syntax_elements.hpp"

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

private:
	std::string text_;
	Lexems lexems_; // Last successful parse lexical analysis result.
	Synt::ProgramElements program_elements_;
};

using DocumentURI= std::string;

} // namespace LangServer

} // namespace U
