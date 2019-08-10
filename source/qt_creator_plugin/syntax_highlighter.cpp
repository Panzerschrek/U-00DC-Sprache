#include <texteditor/texteditorconstants.h>
#include <texteditor/textdocumentlayout.h>

#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "strings.h"

#include "syntax_highlighter.h"

namespace U
{

namespace QtCreatorPlugin
{

static QChar ParenthesCodeForLexem( const Lexem::Type lexem_type )
{
	switch( lexem_type )
	{
	case Lexem::Type::BracketLeft : return '(';
	case Lexem::Type::BracketRight: return ')';
	case Lexem::Type::SquareBracketLeft : return '[';
	case Lexem::Type::SquareBracketRight: return ']';
	case Lexem::Type::BraceLeft : return '{';
	case Lexem::Type::BraceRight: return '}';
	case Lexem::Type::TemplateBracketLeft : return '<';
	case Lexem::Type::TemplateBracketRight: return '>';
	case Lexem::Type::MacroBracketLeft : return '<';
	case Lexem::Type::MacroBracketRight: return '>';
	default: U_ASSERT(false); return ' ';
	}
}

SyntaxHighlighter::SyntaxHighlighter()
{
	static const QVector<TextEditor::TextStyle> categories({
		TextEditor::C_LOCAL,
		TextEditor::C_PREPROCESSOR,
		TextEditor::C_KEYWORD,
		TextEditor::C_NUMBER,
		TextEditor::C_STRING,
		TextEditor::C_COMMENT,
		TextEditor::C_VISUAL_WHITESPACE,
		TextEditor::C_OPERATOR,
		TextEditor::C_ERROR,
	});

	setTextFormatCategories(
		9,
		[&](int category) -> TextEditor::TextStyle
		{
			return TextEditor::TextStyle(categories[category]);
		});
}

void SyntaxHighlighter::highlightBlock( const QString& text )
{
	LexicalAnalysisResult lex_result= LexicalAnalysis( text.utf16(), size_t(text.size()), true );
	if( !lex_result.error_messages.empty() )
	{
		setFormat( 0, text.size(), formatForCategory( int(Formats::LexicalError) ) );
		return;
	}

	QVector<TextEditor::Parenthesis> parentheses;

	setFormat( 0, text.size(), formatForCategory( int(Formats::Whitespace) ) );
	for( size_t i= 0u; i < lex_result.lexems.size(); ++i )
	{
		const Lexem& lexem= lex_result.lexems[i];

		FilePos next_file_pos;
		if( i + 1u < lex_result.lexems.size() )
		{
			next_file_pos= lex_result.lexems[i+1u].file_pos;
		}
		else
		{
			next_file_pos.file_index= 0;
			next_file_pos.line= next_file_pos.pos_in_line= text.size();
		}
		// TODO - what if we have more, then one line?
		const int current_linear_pos= lexem.file_pos.pos_in_line;
		const int next_linear_pos= next_file_pos.pos_in_line;

		// Setup highlighting.
		Formats format;
		switch( lexem.type )
		{
		case Lexem::Type::Comment:
			format= Formats::Comment; break;

		case Lexem::Type::Identifier:
			if( IsKeyword( lexem.text ) )
				format= Formats::Keyword;
			else
				format= Formats::Identifier;
			break;

		case Lexem::Type::MacroIdentifier:
		case Lexem::Type::MacroBracketLeft :
		case Lexem::Type::MacroBracketRight:
		case Lexem::Type::MacroUniqueIdentifier:
			format= Formats::MacroIdentifier; break;

		case Lexem::Type::String:
		case Lexem::Type::LiteralSuffix:
			format= Formats::String; break;

		case Lexem::Type::Number:
			format= Formats::Number; break;

		case Lexem::Type::None:
		default:
			format= Formats::Regular; break;
		}

		setFormat( current_linear_pos, next_linear_pos, formatForCategory( int(format) ) );

		int whitespace_start= next_linear_pos - 1;
		while( whitespace_start >= 0 && text[whitespace_start].isSpace() )
			--whitespace_start;
		++whitespace_start;

		setFormat( whitespace_start, next_linear_pos, formatForCategory( int(Formats::Whitespace) ) );

		// Setup parentheses.
		switch( lexem.type )
		{
		case Lexem::Type::BracketLeft:
		case Lexem::Type::SquareBracketLeft:
		case Lexem::Type::BraceLeft:
		case Lexem::Type::TemplateBracketLeft :
		case Lexem::Type::MacroBracketLeft :
			parentheses.push_back( TextEditor::Parenthesis( TextEditor::Parenthesis::Opened, ParenthesCodeForLexem(lexem.type), current_linear_pos ) );
			break;

		case Lexem::Type::BracketRight:
		case Lexem::Type::SquareBracketRight:
		case Lexem::Type::BraceRight:
		case Lexem::Type::TemplateBracketRight:
		case Lexem::Type::MacroBracketRight:
			parentheses.push_back( TextEditor::Parenthesis( TextEditor::Parenthesis::Closed, ParenthesCodeForLexem(lexem.type), current_linear_pos ) );
			break;
		default: break;
		}
	} // for lexems

	TextEditor::TextDocumentLayout::setParentheses(currentBlock(), parentheses);
}

} // namespace QtCreatorPlugin

} // namespace U
