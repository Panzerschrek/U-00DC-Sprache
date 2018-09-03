#include <QObject>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include "editor_widget.h"
#include "syntax_highlighter.h"

#include "plugin.h"

namespace U
{

namespace QtCreatorPlugin
{

const char g_editor_id[]= "sprache_editor";
//const char g_mime_type[]= "text/u-spr";
const char g_mime_type[]= "text/x-pascal"; // TODO - create own MIME-type

EditorDocument::EditorDocument()
{
	setId(g_editor_id);

	 setSyntaxHighlighter(new SyntaxHighlighter );
	// setIndenter(new CppTools::CppQtStyleIndenter);
}

EditorFactory::EditorFactory()
{
	setId(g_editor_id);
	setDisplayName(u8"Ü sprache editor");
	addMimeType(g_mime_type);

	setDocumentCreator([]() { return new EditorDocument; });
	setEditorWidgetCreator([]() { return new EditorWidget; });
	setEditorCreator([]() { return new Editor; });
}

bool Plugin::initialize( const QStringList& arguments, QString* const error_string )
{
	Q_UNUSED(arguments)
	Q_UNUSED(error_string)

	addAutoReleasedObject(new EditorFactory);
	return true;
}

void Plugin::extensionsInitialized()
{}

ExtensionSystem::IPlugin::ShutdownFlag Plugin::aboutToShutdown()
{
	return SynchronousShutdown;
}

} // namespace UQtCreatorPlugin

} // namespace U
