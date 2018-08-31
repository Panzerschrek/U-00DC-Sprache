#include <texteditor/texteditorconstants.h>

#include "../Compiler/src/lexical_analyzer.hpp"
#include "../Compiler/src/keywords.hpp"

#include "syntax_highlighter.h"

namespace U
{

namespace QtCreatorPlugin
{

SyntaxHighlighter::SyntaxHighlighter()
{
	static const QVector<TextEditor::TextStyle> categories({
		TextEditor::C_LOCAL,
		TextEditor::C_KEYWORD,
		TextEditor::C_NUMBER,
		TextEditor::C_STRING,
		TextEditor::C_COMMENT,
		TextEditor::C_FUNCTION,

	});
	setTextFormatCategories(categories);
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
	LexicalAnalysisResult lex_result= LexicalAnalysis( DecodeUTF8( text.toUtf8().data() ), true );
	if( !lex_result.error_messages.empty() )
	{
		setFormat( 0, text.size(), QColor(Qt::red ) );
		return;
	}

	for( size_t i= 0u; i < lex_result.lexems.size(); ++i )
	{
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

		const Lexem& lexem= lex_result.lexems[i];

		Formats format;
		switch( lexem.type )
		{
		case Lexem::Type::None:
			format= Formats::Regular; break;

		case Lexem::Type::Comment:
			format= Formats::Comment; break;

		case Lexem::Type::Identifier:
			if( IsKeyword( lexem.text ) )
				format= Formats::Keyword;
			else
				format= Formats::Identifier;
			break;

		case Lexem::Type::String:
			format= Formats::String; break;

		case Lexem::Type::Number:
			format= Formats::Number; break;

		case Lexem::Type::LiteralSuffix:
		case Lexem::Type::BracketLeft:
		case Lexem::Type::BracketRight:
		case Lexem::Type::SquareBracketLeft:
		case Lexem::Type::SquareBracketRight:
		case Lexem::Type::BraceLeft:
		case Lexem::Type::BraceRight:
		case Lexem::Type::TemplateBracketLeft :
		case Lexem::Type::TemplateBracketRight:
		case Lexem::Type::Scope:
		case Lexem::Type::Comma:
		case Lexem::Type::Dot:
		case Lexem::Type::Colon:
		case Lexem::Type::Semicolon:
		case Lexem::Type::Assignment:
		case Lexem::Type::Plus:
		case Lexem::Type::Minus:
		case Lexem::Type::Star:
		case Lexem::Type::Slash:
		case Lexem::Type::Percent:
		case Lexem::Type::And:
		case Lexem::Type::Or:
		case Lexem::Type::Xor:
		case Lexem::Type::Tilda:
		case Lexem::Type::Not:
		case Lexem::Type::Apostrophe:
		case Lexem::Type::Increment:
		case Lexem::Type::Decrement:
		case Lexem::Type::CompareLess:
		case Lexem::Type::CompareGreater:
		case Lexem::Type::CompareEqual:
		case Lexem::Type::CompareNotEqual:
		case Lexem::Type::CompareLessOrEqual:
		case Lexem::Type::CompareGreaterOrEqual:
		case Lexem::Type::Conjunction:
		case Lexem::Type::Disjunction:
		case Lexem::Type::AssignAdd:
		case Lexem::Type::AssignSub:
		case Lexem::Type::AssignMul:
		case Lexem::Type::AssignDiv:
		case Lexem::Type::AssignRem:
		case Lexem::Type::AssignAnd:
		case Lexem::Type::AssignOr :
		case Lexem::Type::AssignXor:
		case Lexem::Type::ShiftLeft :
		case Lexem::Type::ShiftRight:
		case Lexem::Type::AssignShiftLeft :
		case Lexem::Type::AssignShiftRight:
		case Lexem::Type::LeftArrow:
		case Lexem::Type::Ellipsis:
		case Lexem::Type::EndOfFile:
			format= Formats::Regular; break;
		}

		// TODO - what if we have more, then one line?
		setFormat( lexem.file_pos.pos_in_line, next_file_pos.pos_in_line, formatForCategory( int(format) ) );
	} // for lexems
}

} // namespace QtCreatorPlugin

} // namespace U
