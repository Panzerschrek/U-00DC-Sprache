#include "../lex_synt_lib/assert.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

namespace
{

// For debug info purposes do not ensure type completeness, just check it.
IsTypeComplete( const Type& type )
{
	if( const auto fundamental_type= type.GetFundamentalType() )
		return fundamental_type->fundamental_type != U_FundamentalType::Void;
	else if( const auto array_type= type.GetArrayType() )
		return IsTypeComplete( array_type->type );
	else if( const auto tuple_type= type.GetTupleType() )
	{
		bool all_complete= true;
		for( const Type& element_type : tuple_type->elements )
			all_complete= all_complete && IsTypeComplete( element_type );
	}
	else if( const auto class_type= type.GetClassType() )
		return class_type->completeness == TypeCompleteness::Complete;
	else if(
		type.GetEnumType() != nullptr ||
		type.GetFunctionType() != nullptr ||
		type.GetFunctionPointerType() != nullptr )
		return true;
	else
		U_ASSERT(false);

	return true;
}

} // namespace

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

	return debug_info_.builder->createBasicType( "_stub_debug_type", 8, llvm::dwarf::DW_ATE_signed );
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
	const uint32_t alignment=
		IsTypeComplete( type.type ) ? data_layout_.getABITypeAlignment( type.llvm_type ) : 0u;

	llvm::SmallVector<llvm::Metadata*, 1> subscripts;
	subscripts.push_back( debug_info_.builder->getOrCreateSubrange( 0, type.size ) );

	return
		debug_info_.builder->createArrayType(
			type.size,
			8u * alignment,
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
		if( !IsTypeComplete( element_type ) )
			continue;

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

	// Ignore incomplete type - do not create debug info for it.
	if( the_class.completeness != TypeCompleteness::Complete )
		return nullptr;

	const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( the_class.llvm_type );

	std::vector<llvm::Metadata*> fields;
	the_class.members.ForEachInThisScope(
		[&]( const std::string& name, const Value& value )
		{
			const ClassField* const class_field= value.GetClassField();
			if( class_field == nullptr || class_field->class_.lock() != type )
				return;

			llvm::Type* field_type_llvm= class_field->type.GetLLVMType();
			llvm::DIType* field_type_di= CreateDIType( class_field->type );
			if( class_field->is_reference )
			{
				field_type_llvm= field_type_llvm->getPointerTo();
				field_type_di=
					debug_info_.builder->createPointerType(
						field_type_di,
						data_layout_.getTypeAllocSizeInBits(field_type_llvm),
						8u * data_layout_.getABITypeAlignment(field_type_llvm) );
			}

			// It will be fine - use here data layout queries, because for complete struct type non-reference fields are complete too.
			const auto member =
				debug_info_.builder->createMemberType(
					debug_info_.compile_unit,
					name,
					debug_info_.file,
					0u, // TODO - file_pos
					data_layout_.getTypeAllocSizeInBits( field_type_llvm ),
					8u * data_layout_.getABITypeAlignment( field_type_llvm ),
					struct_layout.getElementOffsetInBits(class_field->index),
					llvm::DINode::DIFlags(),
					field_type_di );
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
