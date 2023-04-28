#pragma once
#include <texteditor/indenter.h>

namespace U
{

namespace QtCreatorPlugin
{

// Simpliest indeter. Just indents new line with padding of previous line.
class Indenter final : public TextEditor::Indenter
{
	bool isElectricCharacter( const QChar &ch ) const override;

	void indentBlock(
		QTextDocument *doc,
		const QTextBlock &block,
		const QChar &typed_char,
		const TextEditor::TabSettings &tab_settings ) override;

	void indent(
		QTextDocument *doc,
		const QTextCursor &cursor,
		const QChar &typed_char,
		const TextEditor::TabSettings &tab_settings ) override;
};

} // namespace QtCreatorPlugin

} // namespace U
