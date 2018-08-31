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
		Keyword,
		Number,
		String,
		Regular,
	};

	SyntaxHighlighter();

protected:
	virtual void highlightBlock( const QString& text ) override;
};

} // namespace QtCreatorPlugin

} // namespace U
