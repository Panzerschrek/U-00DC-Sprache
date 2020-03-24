#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

llvm::DIType* CodeBuilder::CreateDIType( const Type& type )
{
	llvm::DIType* result_type= nullptr;
	if( const auto fundamental_type= type.GetFundamentalType() )
		result_type= CreateDIFundamentalType( *fundamental_type );
	else if( const auto array_type= type.GetArrayType() )
		result_type= CreateDIArrayType( *array_type );

	if( result_type == nullptr )
		return debug_info_.builder->createBasicType( "i32", 32, llvm::dwarf::DW_ATE_signed );
}

llvm::DIBasicType* CodeBuilder::CreateDIFundamentalType( const FundamentalType& type )
{
	unsigned int type_encoding= llvm::dwarf::DW_ATE_unsigned;
	if( type.fundamental_type == U_FundamentalType::Bool )
		type_encoding= llvm::dwarf::DW_ATE_boolean;
	else if( IsSignedInteger( type.fundamental_type ) )
		type_encoding= llvm::dwarf::DW_ATE_signed;
	else if( IsUnsignedInteger( type.fundamental_type ) )
		type_encoding= llvm::dwarf::DW_ATE_unsigned;
	else if( IsFloatingPoint( type.fundamental_type ) )
		type_encoding= llvm::dwarf::DW_ATE_float;

	return debug_info_.builder->createBasicType(
		GetFundamentalTypeName(type.fundamental_type),
		type.GetSize() * 8u,
		type_encoding );
}

llvm::DICompositeType* CodeBuilder::CreateDIArrayType( const Array& type )
{
	return
		debug_info_.builder->createArrayType(
			type.size,
			data_layout_.getABITypeAlignment( type.llvm_type ), // TODO - what if it is incomplete?
			CreateDIType( type.type ),
			llvm::DINodeArray() );
}

llvm::DICompositeType* CodeBuilder::CreateDIClassType( const Class& type )
{
	// TODO
	return nullptr;
}

} // namespace CodeBuilderPrivate

} // namespace U
