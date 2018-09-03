#pragma once
#include <QAbstractItemModel>
#include "program_model.h"

namespace U
{

namespace QtCreatorPlugin
{

class OutlineWidgetModel final : public QAbstractItemModel
{
	Q_OBJECT

public:
	OutlineWidgetModel( QObject* parent= nullptr );

	void Update( ProgramModelPtr program_model );
	QModelIndex IndexForNode( const ProgramModel::ProgramTreeNode* const node );

	virtual QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const override;
	virtual QModelIndex parent( const QModelIndex& element ) const override;
	virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
	virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const override;
	virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;

	Qt::ItemFlags flags( const QModelIndex &index ) const override;

private:
	using Node= ProgramModel::ProgramTreeNode;

private:
	ProgramModelPtr program_model_;
};

} // namespace QtCreatorPlugin

} // namespace U
