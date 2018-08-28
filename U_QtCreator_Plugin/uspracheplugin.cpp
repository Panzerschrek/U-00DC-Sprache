#include "uspracheplugin.h"
#include "uspracheconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/messagemanager.h>

#include <plugins/texteditor/texteditor.h>
#include <plugins/texteditor/textdocument.h>

#include <utils/treeviewcombobox.h>
#include <utils/dropsupport.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>
#include <QTimer>
#include <QAbstractItemModel>
#include <QStringListModel>

#include "../Compiler/src/lexical_analyzer.hpp"
#include "../Compiler/src/program_string.hpp"
#include "../Compiler/src/syntax_analyzer.hpp"

namespace USprache
{

namespace Internal
{

const char g_editor_id[]= "sprache_editor";
//const char g_mime_type[]= "text/u-spr";
const char g_mime_type[]= "text/x-pascal"; // TODO - create own MIME-type

/* UNFINISHED
class USpracheModel final : public QAbstractItemModel
{
	//Q_OBJECT

public:
	USpracheModel( QObject *parent = nullptr )
		: QAbstractItemModel(parent)
	{
		data_.push_back("@class foo");
		data_.push_back("@class bar");
		data_.push_back("@class baz");
		data_.push_back("@function lol");
	}

	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
	{
		if( column != 0 || row < 0 || row >= data_.size() )
			return QModelIndex();

		return createIndex( row, column );
	}

	virtual QModelIndex parent(const QModelIndex &child) const
	{
		if( child.row() == 0 && child.column() == 0 )
			return QModelIndex();

		return createIndex( 0, 0 );
	}

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const
	{
	}
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const
	{
		return 1;
	}

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
	{
		if( index.row() >= data_.size() )
			return QString("");

		switch(role)
		{
		default:
		case Qt::DisplayRole:
		case Qt::EditRole:
			return data_[ index.row() ];

		case Qt::DecorationRole:
			// TODO - return icon
			return data_[ index.row() ];

		};
	}

	Qt::ItemFlags flags(const QModelIndex &index) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
	}

	Qt::DropActions supportedDragActions() const
	{
		return Qt::MoveAction;
	}

	QStringList mimeTypes() const
	{
		//return Utils::DropSupport::mimeTypesForFilePaths();
		return QStringList{ g_mime_type };
	}

	QMimeData *mimeData(const QModelIndexList &indexes) const
	{
		auto mime_data = new Utils::DropMimeData;
		return mime_data;
	}

private:
	QStringList data_;
};
*/

class USpracheEditorWidget final : public TextEditor::TextEditorWidget
{
	//Q_OBJECT

public:
	USpracheEditorWidget()
		: timer_(this)
	{
		timer_.setInterval(3000);
		timer_.setSingleShot(true);
	}

private:
	void finalizeInitialization() override
	{
		insertExtraToolBarWidget(TextEditorWidget::Left, new QLabel( QString("Ü editor"), this ) );

		combo_box_.setMinimumContentsLength(20);

		QSizePolicy policy = combo_box_.sizePolicy();
		policy.setHorizontalPolicy(QSizePolicy::Expanding);
		combo_box_.setSizePolicy(policy);
		combo_box_.setMaxVisibleItems(40);

		insertExtraToolBarWidget( TextEditorWidget::Left, &combo_box_ );

		connect( this, &QPlainTextEdit::textChanged, this, &USpracheEditorWidget::OnTextChanged );
		connect( &timer_, &QTimer::timeout, this, &USpracheEditorWidget::OnTimerExpired );

		connect( &combo_box_, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &USpracheEditorWidget::OnItemActivated );
	}

private:
	void OnTextChanged()
	{
		//Core::MessageManager::write( QString( "text changed" ) );
		timer_.stop();
		timer_.start();
	}

	void OnTimerExpired()
	{
		// TODO - use custom model.

		const U::ProgramString program_text= U::DecodeUTF8( USpracheEditorWidget::textDocument()->contents().toStdString() );

		const U::LexicalAnalysisResult lex_result= U::LexicalAnalysis( program_text );
		if( !lex_result.error_messages.empty() )
			return;

		U::Synt::SyntaxAnalysisResult synt_result= U::Synt::SyntaxAnalysis( lex_result.lexems );
		if( !synt_result.error_messages.empty() )
			return;

		combo_box_.clear();
		current_program_parsed_= std::move(synt_result.program_elements);

		for( const U::Synt::IProgramElementPtr& program_element : current_program_parsed_ )
		{
			if( const auto namespace_= dynamic_cast<const U::Synt::Namespace*>( program_element.get() ) )
				combo_box_.addItem( QString::fromUtf8( U::ToUTF8( namespace_->name_).data() ) );
			else if( const auto class_= dynamic_cast<const U::Synt::Class*>( program_element.get() ) )
				combo_box_.addItem( QString::fromUtf8( U::ToUTF8( class_->name_ ).data() ) );
			else if( const auto function_= dynamic_cast<const U::Synt::Function*>( program_element.get() ) )
				combo_box_.addItem( QString::fromUtf8( U::ToUTF8( function_->name_.components.back().name ).data() ) );
			else if( const auto variables_= dynamic_cast<const U::Synt::VariablesDeclaration*>( program_element.get() ) )
			{
				for( const auto& variable : variables_->variables )
					combo_box_.addItem( QString::fromUtf8( U::ToUTF8( variable.name ).data() ) );
			}
			else if( const auto auto_variable_= dynamic_cast<const U::Synt::AutoVariableDeclaration*>( program_element.get() ) )
				combo_box_.addItem( QString::fromUtf8( U::ToUTF8( auto_variable_->name ).data() ) );
		}
	}

	void OnItemActivated( int index )
	{
		if( index < 0 || index >= static_cast<int>(current_program_parsed_.size()) )
			return;

		const U::FilePos& file_pos= dynamic_cast<const U::Synt::SyntaxElementBase*>(current_program_parsed_[index].get())->file_pos_;

		Core::EditorManager::cutForwardNavigationHistory();
		Core::EditorManager::addCurrentPositionToNavigationHistory();

		setFocus();
		gotoLine( file_pos.line, file_pos.pos_in_line );
	}

private:
	QTimer timer_;
	Utils::TreeViewComboBox combo_box_;
	//USpracheModel combo_box_model_;

	U::Synt::ProgramElements current_program_parsed_;
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
