#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void CodeBuilder::CreateVariableDebugInfo(
	const Variable& variable,
	const std::string& variable_name,
	const FilePos& file_pos,
	FunctionContext& function_context )
{
	const auto di_local_variable=
		debug_info_.builder->createAutoVariable(
			function_context.function->getSubprogram(),
			variable_name,
			debug_info_.file,
			file_pos.line,
			CreateDIType(variable.type) );

	debug_info_.builder->insertDeclare(
		variable.llvm_value,
		di_local_variable,
		debug_info_.builder->createExpression(),
		llvm::DebugLoc::get(file_pos.line, file_pos.pos_in_line, function_context.function->getSubprogram()),
		function_context.llvm_ir_builder.GetInsertBlock() );
}

llvm::DIType* CodeBuilder::CreateDIType( const Type& type )
{
	llvm::DIType* result_type= nullptr;
	if( const auto fundamental_type= type.GetFundamentalType() )
		result_type= CreateDIType( *fundamental_type );
	else if( const auto array_type= type.GetArrayType() )
		result_type= CreateDIType( *array_type );
	else if( const auto tuple_type= type.GetTupleType() )
		result_type= CreateDIType( *tuple_type );
	else if( const auto class_type= type.GetClassTypeProxy() )
		result_type= CreateDIType( class_type );

	if( result_type != nullptr )
		return result_type;

	return debug_info_.builder->createBasicType( "i32", 32, llvm::dwarf::DW_ATE_signed );
}

llvm::DIBasicType* CodeBuilder::CreateDIType( const FundamentalType& type )
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

llvm::DICompositeType* CodeBuilder::CreateDIType( const Array& type )
{
	llvm::SmallVector<llvm::Metadata*, 1> subscripts;
	subscripts.push_back( debug_info_.builder->getOrCreateSubrange( 0, type.size ) );

	return
		debug_info_.builder->createArrayType(
			type.size,
			8u * data_layout_.getABITypeAlignment( type.llvm_type ), // TODO - what if it is incomplete?
			CreateDIType( type.type ),
			debug_info_.builder->getOrCreateArray(subscripts) );
}

llvm::DICompositeType* CodeBuilder::CreateDIType( const Tuple& type )
{
	const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( type.llvm_type );

	std::vector<llvm::Metadata*> elements;
	elements.reserve( type.elements.size() );

	for( const Type& element_type : type.elements )
	{
		const size_t element_index= size_t(&element_type - type.elements.data());
		const auto element =
			debug_info_.builder->createMemberType(
				debug_info_.compile_unit,
				std::to_string( element_index ),
				debug_info_.file,
				0u, // TODO - file_pos
				data_layout_.getTypeAllocSizeInBits( element_type.GetLLVMType() ),
				8u * data_layout_.getABITypeAlignment( element_type.GetLLVMType() ),
				struct_layout.getElementOffsetInBits( uint32_t(element_index) ),
				llvm::DINode::DIFlags(),
				CreateDIType( element_type ) );

		elements.push_back( element );
	}

	return debug_info_.builder->createStructType(
		debug_info_.compile_unit,
		"", // TODO - name
		debug_info_.file,
		0u, // TODO - file_pos
		data_layout_.getTypeAllocSizeInBits( type.llvm_type ),
		8u * data_layout_.getABITypeAlignment( type.llvm_type ),
		llvm::DINode::DIFlags(),
		nullptr,
		llvm::MDTuple::get( llvm_context_, elements ) );
}

llvm::DICompositeType* CodeBuilder::CreateDIType( const ClassProxyPtr& type )
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
					0u, // TODO - file_pos
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
