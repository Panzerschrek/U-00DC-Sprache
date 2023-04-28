#include <texteditor/tabsettings.h>
#include "indenter.hpp"

namespace U
{

namespace QtCreatorPlugin
{

namespace
{

int getBlockIndent( const QTextBlock& block, const TextEditor::TabSettings &tab_settings )
{
	if( !block.isValid() )
		return 0;

	int indent= 0;
	for( const QChar c : block.text() )
	{
		if( c == ' ' )
			++indent;
		else if( c == '\t' )
			indent+= tab_settings.m_tabSize;
		else
			break;
	}

	return indent;
}

} // namespace

bool Indenter::isElectricCharacter(const QChar &ch) const
{
	switch (ch.toLatin1())
	{
	case '{':
	case '}':
	case ';':
		return true;
	}
	return false;
}

void Indenter::indentBlock(
	QTextDocument* const doc,
	const QTextBlock& block,
	const QChar& typed_char,
	const TextEditor::TabSettings& tab_settings )
{
	(void)doc;
	(void) typed_char;

	int indent= getBlockIndent( block.previous(), tab_settings ) + getBlockIndent( block, tab_settings );
	int padding= 0;

	tab_settings.indentLine( block, indent + padding, padding );
}

void Indenter::indent(
	QTextDocument* const doc,
	const QTextCursor& cursor,
	const QChar& typed_char,
	const TextEditor::TabSettings& tab_settings )
{
	indentBlock(doc, cursor.block(), typed_char, tab_settings);
}

} // namespace QtCreatorPlugin

} // namespace U
