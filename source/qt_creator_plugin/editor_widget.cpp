#include <QMenu>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <plugins/texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>

#include "editor_widget.hpp"

namespace U
{

namespace QtCreatorPlugin
{

EditorWidget::EditorWidget()
{
}

void EditorWidget::finalizeInitialization()
{
	// Allow adding breakpoints.
	setRequestMarkEnabled(true);
	setMarksVisible(true);
}

void EditorWidget::contextMenuEvent( QContextMenuEvent* const event )
{
	const QPointer<QMenu> menu(new QMenu(this));

	menu->addAction( Core::ActionManager::command( TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR)->action() );
	menu->addAction( Core::ActionManager::command( TextEditor::Constants::FIND_USAGES)->action() );
	menu->addAction( Core::ActionManager::command( TextEditor::Constants::RENAME_SYMBOL)->action() );

	appendStandardContextMenuActions(menu);

	menu->exec( event->globalPos() );
	if( menu != nullptr )
		delete menu; // OK, menu was not already deleted by closed editor widget.
}

} // namespace QtCreatorPlugin

} // namespace U
