#pragma once

#include "usprache_global.h"

#include <QAbstractItemModel>
#include <QComboBox>
#include <QTimer>
#include <QTreeView>

#include <coreplugin/messagemanager.h>
#include <extensionsystem/iplugin.h>
#include <plugins/texteditor/textdocument.h>
#include <plugins/texteditor/texteditor.h>
#include <utils/dropsupport.h>


namespace USprache
{

namespace Internal
{

class USprachePlugin : public ExtensionSystem::IPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "USprache.json")

public:
	USprachePlugin();
	~USprachePlugin();

	bool initialize(const QStringList &arguments, QString *errorString);
	void extensionsInitialized();
	ShutdownFlag aboutToShutdown();

private:
	void triggerAction();
};

struct ProgramTreeNode
{
	QString name;
	std::vector<ProgramTreeNode> childs;
	ProgramTreeNode* parent= nullptr;
	size_t number_in_parent= 0;
	int line= 0, pos_in_line= 0;
};

class USpracheModel final : public QAbstractItemModel
{
	Q_OBJECT

public:
	enum Role {
		FileNameRole = Qt::UserRole + 1,
		LineNumberRole
	};

	USpracheModel( QObject *parent = nullptr )
		: QAbstractItemModel(parent)
	{
		tree_.name= QString( "<no data>" );
	}

	~USpracheModel(){}

	void Update( ProgramTreeNode tree )
	{
		beginResetModel();
		tree_= std::move(tree);
		FixParents( tree_ );
		endResetModel();
	}

	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
	{
		Q_UNUSED(column);
		if( !parent.isValid() )
			return createIndex( row, 0, const_cast<ProgramTreeNode*>(&tree_.childs[row]) );
		else
		{
			const ProgramTreeNode* const parent_ptr= reinterpret_cast<const ProgramTreeNode*>(parent.internalPointer());
			return createIndex( row, 0, const_cast<ProgramTreeNode*>(&parent_ptr->childs[row]) );
		}
	}

	virtual QModelIndex parent(const QModelIndex& element) const override
	{
		if( !element.isValid() )
			return QModelIndex();

		const ProgramTreeNode* const element_ptr= reinterpret_cast<const ProgramTreeNode*>(element.internalPointer());
		if( element_ptr->parent == nullptr )
			return QModelIndex();

		return createIndex( element_ptr->parent->number_in_parent, 0, const_cast<ProgramTreeNode*>(element_ptr->parent) ); // TODO - select correct row
	}

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override
	{
		if( !parent.isValid() )
			return tree_.childs.size();

		const ProgramTreeNode* const element_ptr= reinterpret_cast<const ProgramTreeNode*>(parent.internalPointer());
		return int(element_ptr->childs.size());

	}
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override
	{
		Q_UNUSED(parent);
		return 1;
	}

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
	{
		const ProgramTreeNode* const ptr= reinterpret_cast<const ProgramTreeNode*>(index.internalPointer());
		if( ptr == nullptr )
			return QVariant();

		switch(role)
		{
		case Qt::DisplayRole:
			return ptr->name;

		case Qt::DecorationRole:
			// TODO - return icon
			//return QString( "info.ico" );

		case FileNameRole:
			return QString("some_file");

		case LineNumberRole:
			return QString("666");

		case Qt::EditRole:
		default:
			break;
		};
		return QVariant();
	}

	Qt::ItemFlags flags(const QModelIndex &index) const override
	{
		if (!index.isValid())
			return 0;

		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
	}

	Qt::DropActions supportedDragActions() const override
	{
		return Qt::MoveAction;
	}

	QStringList mimeTypes() const override
	{
		return Utils::DropSupport::mimeTypesForFilePaths();
	}

private:
	void FixParents( ProgramTreeNode& tree )
	{
		for( ProgramTreeNode& child : tree.childs )
		{
			child.parent= &tree;
			FixParents(child);
		}
	}

private:
	ProgramTreeNode tree_;
};

class TreeViewComboBox : public QComboBox
{
	Q_OBJECT

public:
	TreeViewComboBox(QWidget *parent = 0)
		: QComboBox(parent)
	{
		view_ = new QTreeView;
		view_->setHeaderHidden(true);
		view_->setItemsExpandable(true);
		view_->expandAll();
		setView(view_);

		 view_->viewport()->installEventFilter(this);
	}

	bool eventFilter(QObject *object, QEvent *event)
	{
		if (event->type() == QEvent::MouseButtonPress && object == view()->viewport()) {
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			QModelIndex index = view()->indexAt(mouseEvent->pos());
			if (!view()->visualRect(index).contains(mouseEvent->pos()))
				skip_next_hide_ = true;
		}
		return false;
	}

	void hidePopup()
	{
		if (skip_next_hide_)
			skip_next_hide_ = false;
		else
			QComboBox::hidePopup();
	}

private:
	QTreeView *view_;
	bool skip_next_hide_= false;
};

class USpracheEditorWidget final : public TextEditor::TextEditorWidget
{
	Q_OBJECT

public:
	USpracheEditorWidget();

private:
	void finalizeInitialization() override;

private:
	void OnTextChanged();
	void OnTimerExpired();
	void OnItemActivated( int index );

private:
	QTimer timer_;
	TreeViewComboBox combo_box_;
	USpracheModel combo_box_model_;
};

class USpracheEditorDocument final : public TextEditor::TextDocument
{
	Q_OBJECT

public:
	USpracheEditorDocument();
};

class USpracheEditor final : public TextEditor::BaseTextEditor
{
	Q_OBJECT

public:
	USpracheEditor()
	{
	}
};

class USpracheEditorFactory final : public TextEditor::TextEditorFactory
{
public:
	USpracheEditorFactory();
private:
};


} // namespace Internal
} // namespace USprache
