#include <libs/cplusplus/Icons.h>

#include "outline_widget_model.hpp"

namespace U
{

namespace QtCreatorPlugin
{

static Utils::CodeModelIcon::Type GetIconType( const ProgramModel::ProgramTreeNode& node )
{
	switch( node.kind )
	{
		case ProgramModel::ElementKind::Unknown:
			return Utils::CodeModelIcon::Unknown;

		case ProgramModel::ElementKind::Macro:
			return Utils::CodeModelIcon::Macro;

		case ProgramModel::ElementKind::Namespace:
			return Utils::CodeModelIcon::Namespace;

		case ProgramModel::ElementKind::Struct:
			return Utils::CodeModelIcon::Struct;

		case ProgramModel::ElementKind::Class:
		case ProgramModel::ElementKind::ClassTemplate:
		case ProgramModel::ElementKind::TypeAlias:
		case ProgramModel::ElementKind::TypeAliasTemplate:
			return Utils::CodeModelIcon::Class;

		case ProgramModel::ElementKind::Enum:
			return Utils::CodeModelIcon::Enum;

		case ProgramModel::ElementKind::EnumElement:
			return Utils::CodeModelIcon::Enumerator;

		case ProgramModel::ElementKind::Function:
		case ProgramModel::ElementKind::FunctionTemplate:
			switch( node.visibility )
			{
			case ProgramModel::Visibility::Public   : return Utils::CodeModelIcon::FuncPublic   ;
			case ProgramModel::Visibility::Protected: return Utils::CodeModelIcon::FuncProtected;
			case ProgramModel::Visibility::Private  : return Utils::CodeModelIcon::FuncPrivate  ;
			};

		case ProgramModel::ElementKind::ClassField:
		case ProgramModel::ElementKind::Variable:
			switch( node.visibility )
			{
			case ProgramModel::Visibility::Public   : return Utils::CodeModelIcon::VarPublic   ;
			case ProgramModel::Visibility::Protected: return Utils::CodeModelIcon::VarProtected;
			case ProgramModel::Visibility::Private  : return Utils::CodeModelIcon::VarPrivate  ;
			};
	};

	Q_ASSERT(false);
	return Utils::CodeModelIcon::Unknown;
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
		return createIndex( row, 0, const_cast<Node*>(&parent_ptr->children[row]) );
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
	return int(element_ptr->children.size());
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
		return Utils::CodeModelIcon::iconForType( GetIconType( *ptr ) );
		break;
	};
	return QVariant();
}

Qt::ItemFlags OutlineWidgetModel::flags(const QModelIndex &index) const
{
	if( !index.isValid() )
		return 0;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

} // namespace QtCreatorPlugin

} // namespace U
