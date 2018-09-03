#include <libs/cplusplus/Icons.h>
#include <utils/dropsupport.h>

#include "outline_widget_model.h"

namespace U
{

namespace QtCreatorPlugin
{

static CPlusPlus::Icons::IconType GetIconType( const ProgramModel::ProgramTreeNode& node )
{
	switch( node.kind )
	{
		case ProgramModel::ElementKind::Unknown:
			return CPlusPlus::Icons::UnknownIconType;
		case ProgramModel::ElementKind::Namespace:
			return CPlusPlus::Icons::NamespaceIconType;
		case ProgramModel::ElementKind::Class:
			return CPlusPlus::Icons::ClassIconType;
		case ProgramModel::ElementKind::Enum:
			return CPlusPlus::Icons::ClassIconType;

		case ProgramModel::ElementKind::Function:
			switch( node.visibility )
			{
			case ProgramModel::Visibility::Public   : return CPlusPlus::Icons::FuncPublicIconType   ;
			case ProgramModel::Visibility::Protected: return CPlusPlus::Icons::FuncProtectedIconType;
			case ProgramModel::Visibility::Private  : return CPlusPlus::Icons::FuncPrivateIconType  ;
			};

		case ProgramModel::ElementKind::ClassFiled:
		case ProgramModel::ElementKind::Variable:
			switch( node.visibility )
			{
			case ProgramModel::Visibility::Public   : return CPlusPlus::Icons::VarPublicIconType   ;
			case ProgramModel::Visibility::Protected: return CPlusPlus::Icons::VarProtectedIconType;
			case ProgramModel::Visibility::Private  : return CPlusPlus::Icons::VarPrivateIconType  ;
			};

		case ProgramModel::ElementKind::ClassTemplate:
			return CPlusPlus::Icons::ClassIconType;
		case ProgramModel::ElementKind::Typedef:
			return CPlusPlus::Icons::ClassIconType;
		case ProgramModel::ElementKind::TypedefTemplate:
			return CPlusPlus::Icons::ClassIconType;
		case ProgramModel::ElementKind::FunctionTemplate:
			return CPlusPlus::Icons::FuncPublicIconType;
	};

	Q_ASSERT(false);
	return CPlusPlus::Icons::UnknownIconType;
}

OutlineWidgetModel::OutlineWidgetModel( QObject *const parent )
	: QAbstractItemModel(parent)
{}

void OutlineWidgetModel::Update( ProgramModelPtr program_model )
{
	beginResetModel();
	program_model_= std::move(program_model);
	endResetModel();
}

QModelIndex OutlineWidgetModel::IndexForNode( const ProgramModel::ProgramTreeNode* const node )
{
	if( node == nullptr )
		return QModelIndex();

	return createIndex( node->number_in_parent, 0, const_cast<Node*>(node) );
}

QModelIndex OutlineWidgetModel::index( const int row, const int column, const QModelIndex& parent ) const
{
	Q_UNUSED(column);
	if( program_model_ == nullptr )
		return QModelIndex();

	if( !parent.isValid() )
		return createIndex( row, 0, const_cast<Node*>(&program_model_->program_elements[row]) );
	else
	{
		const Node* const parent_ptr= reinterpret_cast<const Node*>(parent.internalPointer());
		return createIndex( row, 0, const_cast<Node*>(&parent_ptr->childs[row]) );
	}
}

QModelIndex OutlineWidgetModel::parent( const QModelIndex& element ) const
{
	if( !element.isValid() )
		return QModelIndex();

	const Node* const element_ptr= reinterpret_cast<const Node*>(element.internalPointer());
	if( element_ptr->parent == nullptr )
		return QModelIndex();

	return createIndex( element_ptr->parent->number_in_parent, 0, const_cast<Node*>(element_ptr->parent) );
}

int OutlineWidgetModel::rowCount( const QModelIndex& parent ) const
{
	if( !parent.isValid() )
		return program_model_ == nullptr ? 0 : int(program_model_->program_elements.size());

	const Node* const element_ptr= reinterpret_cast<const Node*>(parent.internalPointer());
	return int(element_ptr->childs.size());

}

int OutlineWidgetModel::columnCount( const QModelIndex& parent ) const
{
	Q_UNUSED(parent);
	return 1;
}

QVariant OutlineWidgetModel::data( const QModelIndex& index, const int role ) const
{
	const Node* const ptr= reinterpret_cast<const Node*>(index.internalPointer());
	if( ptr == nullptr )
		return QVariant();

	switch(role)
	{
	case Qt::DisplayRole:
		return ptr->name;

	case Qt::DecorationRole:
		return  CPlusPlus::Icons::iconForType( GetIconType( *ptr ) );

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

Qt::ItemFlags OutlineWidgetModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

Qt::DropActions OutlineWidgetModel::supportedDragActions() const
{
	return Qt::MoveAction;
}

QStringList OutlineWidgetModel::mimeTypes() const
{
	return Utils::DropSupport::mimeTypesForFilePaths();
}

} // namespace QtCreatorPlugin

} // namespace U
