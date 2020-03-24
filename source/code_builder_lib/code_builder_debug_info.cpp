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
	else if( const auto class_type= type.GetClassTypeProxy() )
		result_type= CreateDIClassType( class_type );

	if( result_type != nullptr )
		return result_type;

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

llvm::DICompositeType* CodeBuilder::CreateDIClassType( const ClassProxyPtr& type )
{
	const Class& the_class= *type->class_;
	if( the_class.completeness != TypeCompleteness::Complete )
		return debug_info_.builder->createStructType(
			debug_info_.compile_unit,
			"__StubStruct",
			debug_info_.file,
			0u,
			8u,
			8u,
			llvm::DINode::DIFlags(),
			nullptr,
			llvm::DINodeArray());

	const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( the_class.llvm_type );

	std::vector<llvm::Metadata*> fields;
	the_class.members.ForEachInThisScope(
		[&]( const std::string& name, const Value& value )
		{
			const ClassField* const class_field= value.GetClassField();
			if( class_field == nullptr || class_field->class_.lock() != type )
				return;

			// Skip references. TODO - implement it.
			if( class_field->is_reference )
				return;

			llvm::DIType* const field_type= CreateDIType( class_field->type );

			const auto member =
				debug_info_.builder->createMemberType(
					debug_info_.compile_unit,
					name,
					debug_info_.file,
					0u, // TODO
					data_layout_.getTypeAllocSizeInBits( class_field->type.GetLLVMType() ),
					8u * data_layout_.getABITypeAlignment( class_field->type.GetLLVMType() ),
					struct_layout.getElementOffsetInBits(class_field->index),
					llvm::DINode::DIFlags(),
					field_type );
			fields.push_back(member);
		});

	return debug_info_.builder->createStructType(
		debug_info_.compile_unit,
		the_class.members.GetThisNamespaceName(),
		debug_info_.file,
		the_class.body_file_pos.line,
		data_layout_.getTypeAllocSizeInBits( the_class.llvm_type ),
		8u * data_layout_.getABITypeAlignment( the_class.llvm_type ),
		llvm::DINode::DIFlags(),
		nullptr,
		llvm::MDTuple::get(llvm_context_, fields) );
}

} // namespace CodeBuilderPrivate

} // namespace U
