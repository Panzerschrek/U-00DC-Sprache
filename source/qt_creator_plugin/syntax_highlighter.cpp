#include <texteditor/texteditorconstants.h>
#include <texteditor/textdocumentlayout.h>

#include "../lex_synt_lib_common/assert.hpp"
#include "../compiler0/lex_synt_lib/lexical_analyzer.hpp"
#include "keywords.hpp"

#include "syntax_highlighter.hpp"

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
		TextEditor::C_BINDING,
		TextEditor::C_ENUMERATION,
		TextEditor::C_NUMBER,
		TextEditor::C_STRING,
		TextEditor::C_COMMENT,
		TextEditor::C_VISUAL_WHITESPACE,
		TextEditor::C_OPERATOR,
		TextEditor::C_ERROR,
	});

	setTextFormatCategories(
		10,
		[&](int category) -> TextEditor::TextStyle
		{
			return TextEditor::TextStyle(categories[category]);
		});
}

void SyntaxHighlighter::highlightBlock( const QString& text )
{
	const std::string text_utf8= text.toStdString();
	LexicalAnalysisResult lex_result= LexicalAnalysis( text_utf8, true );
	if( !lex_result.errors.empty() )
	{
		setFormat( 0, text.size(), formatForCategory( int(Formats::LexicalError) ) );
		return;
	}

	const int prev_comments_state= std::max( 0, previousBlockState() );
	int cur_comments_state= prev_comments_state;

	QVector<TextEditor::Parenthesis> parentheses;

	setFormat( 0, text.size(), formatForCategory( int(Formats::Whitespace) ) );
	for( size_t i= 0u; i < lex_result.lexems.size(); ++i )
	{
		const Lexem& lexem= lex_result.lexems[i];

		SrcLoc next_src_loc;
		if( i + 1u < lex_result.lexems.size() )
		{
			next_src_loc= lex_result.lexems[i+1u].src_loc;
		}
		else
		{
			next_src_loc= SrcLoc( 0u, text.size(), text.size() );
		}
		// TODO - what if we have more, then one line?
		const int current_linear_pos= lexem.src_loc.GetColumn();
		const int next_linear_pos= next_src_loc.GetColumn();

		// Setup highlighting.
		Formats format;
		switch( lexem.type )
		{
		case Lexem::Type::Comment:
			format= Formats::Comment;
			if( lexem.text == "/*" )
				++cur_comments_state;
			else if( lexem.text == "*/" )
				--cur_comments_state;
			break;

		case Lexem::Type::Identifier:
			if( IsKeyword( lexem.text ) )
			{
				if( lexem.text == Keywords::unsafe_ ||
					lexem.text == Keywords::cast_mut_ ||
					lexem.text== Keywords::cast_ref_unsafe_ ||
					lexem.text== Keywords::uninitialized_ )
					format= Formats::UnsafeConstruction;
			else if(lexem.text == Keywords::i8_   || lexem.text == Keywords::u8_   || lexem.text == Keywords::byte8_   ||
					lexem.text == Keywords::i16_  || lexem.text == Keywords::u16_  || lexem.text == Keywords::byte16_  ||
					lexem.text == Keywords::i32_  || lexem.text == Keywords::u32_  || lexem.text == Keywords::byte32_  ||
					lexem.text == Keywords::i64_  || lexem.text == Keywords::u64_  || lexem.text == Keywords::byte64_  ||
					lexem.text == Keywords::i128_ || lexem.text == Keywords::u128_ || lexem.text == Keywords::byte128_ ||
					lexem.text == Keywords::size_type_ ||
					lexem.text == Keywords::void_ ||
					lexem.text == Keywords::bool_ ||
					lexem.text == Keywords::char8_ || lexem.text == Keywords::char16_ || lexem.text == Keywords::char32_ ||
					lexem.text == Keywords::f32_ || lexem.text == Keywords::f64_ )
					format= Formats::FundamentalType;
				else
					format= Formats::Keyword;
			}
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
		if( cur_comments_state != 0 )
			format= Formats::Comment;

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

	setCurrentBlockState( cur_comments_state );
	TextEditor::TextDocumentLayout::setParentheses(currentBlock(), parentheses);
}

} // namespace QtCreatorPlugin

} // namespace U
