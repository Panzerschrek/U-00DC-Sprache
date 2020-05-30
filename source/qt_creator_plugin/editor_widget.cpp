#include <QtWidgets/QMenu>
#include <plugins/texteditor/textdocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include "strings.h"

#include "editor_widget.hpp"

namespace U
{

namespace QtCreatorPlugin
{

OutlineTreeViewComboBox::OutlineTreeViewComboBox( QWidget* const parent )
	: QComboBox(parent)
	, view_( this )
{
	view_.setHeaderHidden(true);
	view_.setItemsExpandable(true);
	view_.expandAll();
	setView(&view_);

	view_.viewport()->installEventFilter(this);
}

bool OutlineTreeViewComboBox::eventFilter( QObject* object, QEvent* event )
{
	if( event->type() == QEvent::MouseButtonPress && object == view()->viewport() )
	{
		QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
		QModelIndex index = view()->indexAt(mouse_event->pos());
		if( !view()->visualRect(index).contains(mouse_event->pos()) )
			skip_next_hide_ = true;
	}
	return false;
}

void OutlineTreeViewComboBox::hidePopup()
{	
	if (skip_next_hide_)
		skip_next_hide_ = false;
	else
		QComboBox::hidePopup();
}

EditorWidget::EditorWidget()
	: timer_(this)
	, combo_box_(this)
	, combo_box_model_(this)
{
	timer_.setInterval(3000);
	timer_.setSingleShot(true);
}

void EditorWidget::finalizeInitialization()
{
	combo_box_.setModel( &combo_box_model_ );

	QSizePolicy policy = combo_box_.sizePolicy();
	policy.setHorizontalPolicy(QSizePolicy::Expanding);
	combo_box_.setSizePolicy(policy);
	combo_box_.setMaxVisibleItems(40);

	insertExtraToolBarWidget( TextEditorWidget::Left, &combo_box_ );

	connect( this, &QPlainTextEdit::textChanged, this, &EditorWidget::OnTextChanged );
	connect( &timer_, &QTimer::timeout, this, &EditorWidget::OnTimerExpired );
	connect( &combo_box_, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &EditorWidget::OnItemActivated );
	connect( this, &EditorWidget::cursorPositionChanged, this, &EditorWidget::OnCursorPositionChanged );

	// Allow adding breakpoints.
	setRequestMarkEnabled(true);
	setMarksVisible(true);
}

Utils::Link EditorWidget::findLinkAt(
	const QTextCursor& cursor,
	bool resolveTarget,
	bool inNextSplit)
{
	return Utils::Link(); // TODO - return link here.
}

void EditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
	const auto menu= createStandardContextMenu(e->pos());

	{
		int line= 0, column= 0;
		convertPosition( position(), &line, &column );
		emit markContextMenuRequested(this, line, menu);
	}

	menu->addSeparator();
	const auto action= new QAction("got to definition", this);
	connect( action, &QAction::triggered, this, &EditorWidget::openLinkUnderCursor );
	menu->addAction(action);

	menu->exec(e->globalPos());
	delete menu;
	e->accept();
}

void EditorWidget::OnTextChanged()
{
	timer_.stop();
	timer_.start();
}

void EditorWidget::OnTimerExpired()
{
	const auto program_model= BuildProgramModel( EditorWidget::textDocument()->plainText() );
	if( program_model != nullptr )
	{
		program_model_= program_model;
		combo_box_model_.Update( program_model );

		OnCursorPositionChanged();
	}
}

void EditorWidget::OnItemActivated()
{
	block_cursor_sync_= true;

	const QModelIndex model_index = combo_box_.view()->currentIndex();
	const auto node_ptr= reinterpret_cast<const ProgramModel::ProgramTreeNode*>(model_index.internalPointer());
	if( node_ptr == nullptr )
		return;

	Core::EditorManager::cutForwardNavigationHistory();
	Core::EditorManager::addCurrentPositionToNavigationHistory();

	setFocus();
	gotoLine( node_ptr->file_pos.GetLine(), node_ptr->file_pos.GetColumn() );

	block_cursor_sync_= false;
}

void EditorWidget::OnCursorPositionChanged()
{
	if( block_cursor_sync_ )
		return;

	if( program_model_ == nullptr )
		return;

	int line= 0, column= 0;
	convertPosition( position(), &line, &column );

	FilePos file_pos( 0u, line, column );

	if( const auto node= program_model_->GetNodeForFilePos( file_pos ) )
	{
		combo_box_.setRootModelIndex( combo_box_model_.parent( combo_box_model_.IndexForNode( node ) ) );
		combo_box_.setCurrentIndex( int(node->number_in_parent) );
		combo_box_.setRootModelIndex( QModelIndex() );
	}
}

} // namespace QtCreatorPlugin

} // namespace U
