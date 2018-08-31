#include "uspracheplugin.h"

#include <QObject>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include "../Compiler/src/lexical_analyzer.hpp"
#include "../Compiler/src/program_string.hpp"
#include "../Compiler/src/syntax_analyzer.hpp"

namespace U
{

namespace QtCreatorPlugin
{

const char g_editor_id[]= "sprache_editor";
//const char g_mime_type[]= "text/u-spr";
const char g_mime_type[]= "text/x-pascal"; // TODO - create own MIME-type

ProgramTreeNode ConvertProgramForOutline( const Synt::ProgramElements& elements )
{
	ProgramTreeNode result;
	result.number_in_parent= 0;
	result.name= QString( "global namespace" );

	for( const Synt::IProgramElementPtr& program_element : elements )
	{
		if( const auto namespace_= dynamic_cast<const Synt::Namespace*>( program_element.get() ) )
		{
			ProgramTreeNode element;
			element.name= QString::fromUtf8( ToUTF8( namespace_->name_).data() );
			element.childs= ConvertProgramForOutline( namespace_->elements_ ).childs;
			element.number_in_parent= result.childs.size();
			result.childs.push_back(element);
		}
		else if( const auto class_= dynamic_cast<const Synt::Class*>( program_element.get() ) )
		{
			ProgramTreeNode element;
			element.name= QString::fromUtf8( ToUTF8( class_->name_ ).data() );
			element.number_in_parent= result.childs.size();
			result.childs.push_back(element);
		}
		else if( const auto function_= dynamic_cast<const Synt::Function*>( program_element.get() ) )
		{
			ProgramTreeNode element;
			element.name= QString::fromUtf8( ToUTF8( function_->name_.components.back().name ).data() );
			element.number_in_parent= result.childs.size();
			result.childs.push_back(element);
		}
		else if( const auto variables_= dynamic_cast<const Synt::VariablesDeclaration*>( program_element.get() ) )
		{
			for( const auto& variable : variables_->variables )
			{
				ProgramTreeNode element;
				element.name= QString::fromUtf8( ToUTF8( variable.name ).data() );
				element.number_in_parent= result.childs.size();
				result.childs.push_back(element);
			}
		}
		else if( const auto auto_variable_= dynamic_cast<const Synt::AutoVariableDeclaration*>( program_element.get() ) )
		{
			ProgramTreeNode element;
			element.name= QString::fromUtf8( ToUTF8( auto_variable_->name ).data() );
			element.number_in_parent= result.childs.size();
			result.childs.push_back(element);
		}
	}

	return result;
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

	//connect( &combo_box_, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &USpracheEditorWidget::OnItemActivated );
}

void USpracheEditorWidget::OnTextChanged()
{
	//Core::MessageManager::write( QString( "text changed" ) );
	timer_.stop();
	timer_.start();
}

void USpracheEditorWidget::OnTimerExpired()
{
	Core::MessageManager::write( QString( "timer expired" ) );

	const U::ProgramString program_text= U::DecodeUTF8( USpracheEditorWidget::textDocument()->contents().toStdString() );

	const U::LexicalAnalysisResult lex_result= U::LexicalAnalysis( program_text );
	if( !lex_result.error_messages.empty() )
		return;

	U::Synt::SyntaxAnalysisResult synt_result= U::Synt::SyntaxAnalysis( lex_result.lexems );
	if( !synt_result.error_messages.empty() )
		return;

	combo_box_model_.Update( ConvertProgramForOutline( synt_result.program_elements ) );
	//combo_box_.view_->expandAll();
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

void OnItemActivated( int index )
{
	/*
	if( index < 0 || index >= static_cast<int>(current_program_parsed_->size()) )
		return;

	const U::FilePos& file_pos= dynamic_cast<const U::Synt::SyntaxElementBase*>((*current_program_parsed_)[index].get())->file_pos_;

	Core::EditorManager::cutForwardNavigationHistory();
	Core::EditorManager::addCurrentPositionToNavigationHistory();

	setFocus();
	gotoLine( file_pos.line, file_pos.pos_in_line );
	*/
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
