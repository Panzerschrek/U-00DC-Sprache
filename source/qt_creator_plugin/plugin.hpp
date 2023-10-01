#pragma once
#include <extensionsystem/iplugin.h>
#include <plugins/texteditor/textdocument.h>
#include <plugins/texteditor/texteditor.h>

namespace U
{

namespace QtCreatorPlugin
{

class Plugin : public ExtensionSystem::IPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "USprache.json")

public:
	virtual bool initialize( const QStringList& arguments, QString* error_string ) override;
	virtual void extensionsInitialized() override;
	virtual ShutdownFlag aboutToShutdown() override;
};

class EditorDocument final : public TextEditor::TextDocument
{
	Q_OBJECT

public:
	EditorDocument();
};

class Editor final : public TextEditor::BaseTextEditor
{
	Q_OBJECT
};

class EditorFactory final : public TextEditor::TextEditorFactory
{
	Q_OBJECT

public:
	EditorFactory();
};

} // namespace QtCreatorPlugin

} // namespace U
