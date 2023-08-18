#pragma once
#include <QComboBox>
#include <QTimer>
#include <QTreeView>

#include <plugins/texteditor/texteditor.h>

#include "program_model.hpp"
#include "outline_widget_model.hpp"

namespace U
{

namespace QtCreatorPlugin
{

class OutlineTreeViewComboBox : public QComboBox
{
	Q_OBJECT

public:
	explicit OutlineTreeViewComboBox( QWidget* parent = nullptr );

	virtual bool eventFilter( QObject* object, QEvent* event ) override;
	virtual void hidePopup() override;

private:
	QTreeView view_;
	bool skip_next_hide_= false;
};

class EditorWidget final : public TextEditor::TextEditorWidget
{
	Q_OBJECT

public:
	EditorWidget();

private:
	virtual void finalizeInitialization() override;

protected:
	virtual void contextMenuEvent( QContextMenuEvent* event ) override;

private:
	void OnTextChanged();
	void OnTimerExpired();
	void OnItemActivated();
	void OnCursorPositionChanged();

private:
	QTimer timer_;
	OutlineTreeViewComboBox combo_box_;
	OutlineWidgetModel combo_box_model_;
	ProgramModelPtr program_model_;
	bool block_cursor_sync_= false;
};

} // namespace QtCreatorPlugin

} // namespace U
