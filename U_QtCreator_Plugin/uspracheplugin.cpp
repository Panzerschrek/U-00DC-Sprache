#include "uspracheplugin.h"

#include <QObject>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

namespace U
{

namespace QtCreatorPlugin
{

const char g_editor_id[]= "sprache_editor";
//const char g_mime_type[]= "text/u-spr";
const char g_mime_type[]= "text/x-pascal"; // TODO - create own MIME-type

CPlusPlus::Icons::IconType ProgramElementKindToIcon( const ProgramModel::ElementKind element_kind )
{
	switch( element_kind )
	{
		case ProgramModel::ElementKind::Unknown: return CPlusPlus::Icons::UnknownIconType;
		case ProgramModel::ElementKind::Namespace: return CPlusPlus::Icons::NamespaceIconType;
		case ProgramModel::ElementKind::Class: return CPlusPlus::Icons::ClassIconType;
		case ProgramModel::ElementKind::Function: return CPlusPlus::Icons::FuncPublicIconType;
		case ProgramModel::ElementKind::ClassFiled: return CPlusPlus::Icons::VarPublicIconType;
		case ProgramModel::ElementKind::Variable: return CPlusPlus::Icons::VarPublicIconType;
		case ProgramModel::ElementKind::ClassTemplate: return CPlusPlus::Icons::ClassIconType;
		case ProgramModel::ElementKind::Typedef: return CPlusPlus::Icons::ClassIconType;
		case ProgramModel::ElementKind::TypedefTemplate: return CPlusPlus::Icons::ClassIconType;
		case ProgramModel::ElementKind::FunctionTemplate: return CPlusPlus::Icons::FuncPublicIconType;
	};

	Q_ASSERT(false);
	return CPlusPlus::Icons::ClassIconType;
}

USpracheEditorWidget::USpracheEditorWidget()
	: timer_(this)
	, combo_box_(this)
	, combo_box_model_(this)
{
	timer_.setInterval(3000);
	timer_.setSingleShot(true);
}

void USpracheEditorWidget::finalizeInitialization()
{
	insertExtraToolBarWidget(TextEditorWidget::Left, new QLabel( QString("Ü editor"), this ) );

	combo_box_.setModel( &combo_box_model_ );

	combo_box_.setMinimumContentsLength(20);

	QSizePolicy policy = combo_box_.sizePolicy();
	policy.setHorizontalPolicy(QSizePolicy::Expanding);
	combo_box_.setSizePolicy(policy);
	combo_box_.setMaxVisibleItems(40);

	insertExtraToolBarWidget( TextEditorWidget::Left, &combo_box_ );

	connect( this, &QPlainTextEdit::textChanged, this, &USpracheEditorWidget::OnTextChanged );
	connect( &timer_, &QTimer::timeout, this, &USpracheEditorWidget::OnTimerExpired );

	connect( &combo_box_, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &USpracheEditorWidget::OnItemActivated );

	connect( this, &USpracheEditorWidget::cursorPositionChanged, this, &USpracheEditorWidget::OnCursorPositionChanged );
}

void USpracheEditorWidget::OnTextChanged()
{
	//Core::MessageManager::write( QString( "text changed" ) );
	timer_.stop();
	timer_.start();
}

void USpracheEditorWidget::OnTimerExpired()
{
	const U::ProgramString program_text= U::DecodeUTF8( USpracheEditorWidget::textDocument()->contents().toStdString() );

	const auto program_model= BuildProgramModel( program_text );
	if( program_model != nullptr )
	{
		program_model_= program_model;
		combo_box_model_.Update( program_model );
	}
}

void USpracheEditorWidget::OnItemActivated()
{
	block_cursor_sync_= true;

	const QModelIndex model_index = combo_box_.view()->currentIndex();
	const auto node_ptr= reinterpret_cast<const ProgramModel::ProgramTreeNode*>(model_index.internalPointer());
	if( node_ptr == nullptr )
		return;

	Core::EditorManager::cutForwardNavigationHistory();
	Core::EditorManager::addCurrentPositionToNavigationHistory();

	setFocus();
	gotoLine( node_ptr->file_pos.line, node_ptr->file_pos.pos_in_line );

	block_cursor_sync_= false;
}

void USpracheEditorWidget::OnCursorPositionChanged()
{
	if( block_cursor_sync_ )
		return;

	if( program_model_ == nullptr )
		return;

	int line= 0, pos_in_line= 0;
	convertPosition( position(), &line, &pos_in_line );

	FilePos file_pos;
	file_pos.line= static_cast<unsigned short>(line);
	file_pos.pos_in_line= static_cast<unsigned short>(pos_in_line);
	file_pos.file_index= 0;

	if( const auto node= program_model_->GetNodeForFilePos( file_pos ) )
	{
		combo_box_.setRootModelIndex( combo_box_model_.parent( combo_box_model_.IndexForNode( node ) ) );
		combo_box_.setCurrentIndex( int(node->number_in_parent) );
		combo_box_.setRootModelIndex( QModelIndex() );
	}
}

USpracheEditorDocument::USpracheEditorDocument()
{
	setId(g_editor_id);

	// setSyntaxHighlighter(new CppHighlighter);
	// setIndenter(new CppTools::CppQtStyleIndenter);
}

USpracheEditorFactory::USpracheEditorFactory()
{
	setId(g_editor_id);
	setDisplayName(u8"Ü sprache editor");
	addMimeType(g_mime_type);

	setDocumentCreator([]() { return new USpracheEditorDocument; });
	setEditorWidgetCreator([]() { return new USpracheEditorWidget; });
	setEditorCreator([]() { return new USpracheEditor; });
}

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

} // namespace UQtCreatorPlugin

} // namespace U
