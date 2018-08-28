#include "uspracheplugin.h"
#include "uspracheconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <coreplugin/editormanager/editormanager.h>

#include <plugins/texteditor/texteditor.h>
#include <plugins/texteditor/textdocument.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

namespace USprache
{

namespace Internal
{

const char g_editor_id[]= "sprache_editor";
//const char g_mime_type[]= "text/u-spr";
const char g_mime_type[]= "text/x-pascal"; // TODO - create own MIME-type

class USpracheEditorWidget final : public TextEditor::TextEditorWidget
{
	//Q_OBJECT

public:
	USpracheEditorWidget()
	{
	}

	virtual ~USpracheEditorWidget() override
	{
	}

private:
	void finalizeInitialization() override
	{
		// TODO - add here out cool U-Sprache widget.
		auto toolbar_widget= new QLabel( QString("dummy stub") );

		insertExtraToolBarWidget(TextEditorWidget::Left, toolbar_widget );
	}
};

class USpracheEditorDocument final : public TextEditor::TextDocument
{
	//Q_OBJECT

public:
	USpracheEditorDocument()
	{
		setId(g_editor_id);

		// setSyntaxHighlighter(new CppHighlighter);
		// setIndenter(new CppTools::CppQtStyleIndenter);
	}
};

class USpracheEditor final : public TextEditor::BaseTextEditor
{
	//Q_OBJECT

public:
	USpracheEditor()
	{
	}
};

class USpracheEditorFactory final : public TextEditor::TextEditorFactory
{
public:
	USpracheEditorFactory()
	{
		setId(g_editor_id);
		setDisplayName(u8"Ü sprache editor");
		addMimeType(g_mime_type);

		setDocumentCreator([]() { return new USpracheEditorDocument; });
		setEditorWidgetCreator([]() { return new USpracheEditorWidget; });
		setEditorCreator([]() { return new USpracheEditor; });
	}

private:
};

USprachePlugin::USprachePlugin()
{
	// Create your members
}

USprachePlugin::~USprachePlugin()
{
	// Unregister objects from the plugin manager's object pool
	// Delete members
}

bool USprachePlugin::initialize(const QStringList &arguments, QString *errorString)
{
	// Register objects in the plugin manager's object pool
	// Load settings
	// Add actions to menus
	// Connect to other plugins' signals
	// In the initialize function, a plugin can be sure that the plugins it
	// depends on have initialized their members.

	Q_UNUSED(arguments)
	Q_UNUSED(errorString)

	addAutoReleasedObject(new USpracheEditorFactory);

	return true;
}

void USprachePlugin::extensionsInitialized()
{
	// Retrieve objects from the plugin manager's object pool
	// In the extensionsInitialized function, a plugin can be sure that all
	// plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag USprachePlugin::aboutToShutdown()
{
	// Save settings
	// Disconnect from signals that are not needed during shutdown
	// Hide UI (if you add UI that is not in the main window directly)
	return SynchronousShutdown;
}

} // namespace Internal

} // namespace USprache
