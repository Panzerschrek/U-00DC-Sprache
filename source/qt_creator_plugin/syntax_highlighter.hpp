#pragma once
#include <texteditor/syntaxhighlighter.h>

namespace U
{

namespace QtCreatorPlugin
{

class SyntaxHighlighter : public TextEditor::SyntaxHighlighter
{
public:
	enum class Formats
	{
		Identifier,
		MacroIdentifier,
		Keyword,
		UnsafeConstruction,
		FundamentalType,
		Number,
		String,
		Comment,
		Whitespace,
		Regular,
		LexicalError,
	};

	SyntaxHighlighter();

protected:
	virtual void highlightBlock( const QString& text ) override;
};

} // namespace QtCreatorPlugin

} // namespace U
