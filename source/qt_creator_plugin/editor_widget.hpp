#pragma once
#include <plugins/texteditor/texteditor.h>

namespace U
{

namespace QtCreatorPlugin
{

class EditorWidget final : public TextEditor::TextEditorWidget
{
	Q_OBJECT

public:
	EditorWidget();

private:
	virtual void finalizeInitialization() override;

protected:
	virtual void contextMenuEvent( QContextMenuEvent* event ) override;
};

} // namespace QtCreatorPlugin

} // namespace U
