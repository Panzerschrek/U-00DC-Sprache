#include "../../lex_synt_lib_common/assert.hpp"
#include "code_builder.hpp"

namespace U
{

llvm::DIFile* CodeBuilder::GetDIFile(const size_t file_index)
{
	U_ASSERT( file_index < debug_info_.source_file_entries.size() );
	return debug_info_.source_file_entries[file_index];
}

void CodeBuilder::CreateVariableDebugInfo(
	const Variable& variable,
	const std::string& variable_name,
	const SrcLoc& src_loc,
	FunctionContext& function_context )
{
	if( !build_debug_info_ )
		return;

	const auto di_local_variable=
		debug_info_.builder->createAutoVariable(
			function_context.current_debug_info_scope,
			variable_name,
			GetDIFile( src_loc.GetFileIndex() ),
			src_loc.GetLine(),
			CreateDIType(variable.type) );

	debug_info_.builder->insertDeclare(
		variable.llvm_value,
		di_local_variable,
		debug_info_.builder->createExpression(),
		llvm::DILocation::get(llvm_context_, src_loc.GetLine(), src_loc.GetColumn(), function_context.current_debug_info_scope),
		function_context.llvm_ir_builder.GetInsertBlock() );
}

void CodeBuilder::CreateReferenceVariableDebugInfo(
	const Variable& variable,
	const std::string& variable_name,
	const SrcLoc& src_loc,
	FunctionContext& function_context )
{
	if( !build_debug_info_ )
		return;

	const auto di_local_variable=
		debug_info_.builder->createAutoVariable(
			function_context.current_debug_info_scope,
			variable_name,
			GetDIFile( src_loc.GetFileIndex() ),
			src_loc.GetLine(),
			debug_info_.builder->createPointerType( CreateDIType(variable.type), data_layout_.getPointerSizeInBits() ) );

	// We needs address for reference, so, move it into stack variable.
	auto address_for_ref= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType()->getPointerTo(), nullptr, variable_name );
	CreateTypedReferenceStore( function_context, variable.type, variable.llvm_value, address_for_ref );

	debug_info_.builder->insertDeclare(
		address_for_ref,
		di_local_variable,
		debug_info_.builder->createExpression(),
		llvm::DILocation::get(llvm_context_, src_loc.GetLine(), src_loc.GetColumn(), function_context.current_debug_info_scope),
		function_context.llvm_ir_builder.GetInsertBlock() );
}

void CodeBuilder::CreateFunctionDebugInfo(
	const FunctionVariable& func_variable,
	const std::string& function_name )
{
	if( !build_debug_info_ )
		return;

	const auto di_function= debug_info_.builder->createFunction(
		GetDIFile( func_variable.body_src_loc.GetFileIndex() ),
		function_name,
		func_variable.llvm_function->getName(),
		GetDIFile( func_variable.body_src_loc.GetFileIndex() ),
		func_variable.body_src_loc.GetLine(),
		CreateDIType( *func_variable.type.GetFunctionType() ),
		func_variable.body_src_loc.GetLine(),
		llvm::DINode::FlagPrototyped,
		llvm::DISubprogram::SPFlagDefinition );
	func_variable.llvm_function->setSubprogram( di_function );
}

void CodeBuilder::SetCurrentDebugLocation(
	const SrcLoc& src_loc,
	FunctionContext& function_context )
{
	if( !build_debug_info_ )
		return;

	function_context.llvm_ir_builder.SetCurrentDebugLocation(
		llvm::DILocation::get(
			llvm_context_,
			src_loc.GetLine(),
			src_loc.GetColumn(),
			function_context.current_debug_info_scope ) );
}

void CodeBuilder::DebugInfoStartBlock( const SrcLoc& src_loc, FunctionContext& function_context )
{
	if( build_debug_info_ )
		function_context.current_debug_info_scope=
			debug_info_.builder->createLexicalBlock(
				function_context.current_debug_info_scope,
				GetDIFile( src_loc.GetFileIndex() ),
				src_loc.GetLine(),
				src_loc.GetColumn() );
}

void CodeBuilder::DebugInfoEndBlock( FunctionContext& function_context )
{
	if( build_debug_info_ )
		function_context.current_debug_info_scope= function_context.current_debug_info_scope->getScope();
}

llvm::DIType* CodeBuilder::CreateDIType( const Type& type )
{
	U_ASSERT(build_debug_info_);

	llvm::DIType* result_type= nullptr;
	if( const auto fundamental_type= type.GetFundamentalType() )
		result_type= CreateDIType( *fundamental_type );
	else if( const auto array_type= type.GetArrayType() )
		result_type= CreateDIType( *array_type );
	else if( const auto tuple_type= type.GetTupleType() )
		result_type= CreateDIType( *tuple_type );
	else if( const auto function_type= type.GetFunctionType() )
		result_type= CreateDIType( *function_type );
	else if( const auto function_pointer_type= type.GetFunctionPointerType() )
		result_type= CreateDIType( *function_pointer_type );
	else if( const auto raw_pointer_type= type.GetRawPointerType() )
		result_type= CreateDIType( *raw_pointer_type );
	else if( const auto class_type= type.GetClassType() )
		result_type= CreateDIType( class_type );
	else if( const auto enum_type= type.GetEnumType() )
		result_type= CreateDIType( enum_type );

	if( result_type != nullptr )
		return result_type;

	return debug_info_.builder->createBasicType( "_stub_debug_type", 8, llvm::dwarf::DW_ATE_signed );
}

llvm::DIType* CodeBuilder::CreateDIType( const FundamentalType& type )
{
	U_ASSERT(build_debug_info_);

	if( type.fundamental_type == U_FundamentalType::void_ )
	{
		// Internal representation of void type is llvm struct with zero elements.
		return debug_info_.builder->createStructType(
			debug_info_.compile_unit,
			GetFundamentalTypeName( type.fundamental_type ),
			GetDIFile(0),
			0u,
			data_layout_.getTypeAllocSizeInBits( type.llvm_type ),
			8u * data_layout_.getABITypeAlignment( type.llvm_type ),
			llvm::DINode::DIFlags(),
			nullptr,
			debug_info_.builder->getOrCreateArray({}).get() );
	}

	unsigned int type_encoding= llvm::dwarf::DW_ATE_unsigned;
	if( type.fundamental_type == U_FundamentalType::bool_ )
		type_encoding= llvm::dwarf::DW_ATE_boolean;
	else if( IsSignedInteger( type.fundamental_type ) )
		type_encoding= llvm::dwarf::DW_ATE_signed;
	else if( IsUnsignedInteger( type.fundamental_type ) )
		type_encoding= llvm::dwarf::DW_ATE_unsigned;
	else if( IsChar( type.fundamental_type ) )
		type_encoding= llvm::dwarf::DW_ATE_unsigned_char;
	else if( IsByte( type.fundamental_type ) )
		type_encoding= llvm::dwarf::DW_ATE_unsigned;
	else if( IsFloatingPoint( type.fundamental_type ) )
		type_encoding= llvm::dwarf::DW_ATE_float;

	return debug_info_.builder->createBasicType(
		GetFundamentalTypeName(type.fundamental_type),
		type.GetSize() * 8u,
		type_encoding );
}

llvm::DICompositeType* CodeBuilder::CreateDIType( const ArrayType& type )
{
	U_ASSERT(build_debug_info_);

	const uint32_t alignment=
		IsTypeComplete( type.type ) ? data_layout_.getABITypeAlignment( type.llvm_type ) : 0u;
	const uint64_t size=
		IsTypeComplete( type.type ) ? data_layout_.getTypeAllocSizeInBits( type.llvm_type ) : uint64_t(0);

	llvm::SmallVector<llvm::Metadata*, 1> subscripts;
	subscripts.push_back( debug_info_.builder->getOrCreateSubrange( 0, int64_t(type.size) ) );

	return
		debug_info_.builder->createArrayType(
			size,
			8u * alignment,
			CreateDIType( type.type ),
			debug_info_.builder->getOrCreateArray(subscripts) );
}

llvm::DICompositeType* CodeBuilder::CreateDIType( const TupleType& type )
{
	U_ASSERT(build_debug_info_);

	if( !IsTypeComplete( type ) )
		return nullptr;

	const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( type.llvm_type );

	std::vector<llvm::Metadata*> elements;
	elements.reserve( type.elements.size() );

	const auto di_file= GetDIFile(0);

	for( const Type& element_type : type.elements )
	{
		if( !IsTypeComplete( element_type ) )
			continue;

		const size_t element_index= size_t(&element_type - type.elements.data());
		const auto element =
			debug_info_.builder->createMemberType(
				debug_info_.compile_unit,
				std::to_string( element_index ),
				di_file,
				0u, // TODO - src_loc
				data_layout_.getTypeAllocSizeInBits( element_type.GetLLVMType() ),
				8u * data_layout_.getABITypeAlignment( element_type.GetLLVMType() ),
				struct_layout.getElementOffsetInBits( uint32_t(element_index) ),
				llvm::DINode::DIFlags(),
				CreateDIType( element_type ) );

		elements.push_back( element );
	}

	return debug_info_.builder->createStructType(
		debug_info_.compile_unit,
		Type(type).ToString(),
		di_file,
		0u, // TODO - src_loc
		data_layout_.getTypeAllocSizeInBits( type.llvm_type ),
		8u * data_layout_.getABITypeAlignment( type.llvm_type ),
		llvm::DINode::DIFlags(),
		nullptr,
		debug_info_.builder->getOrCreateArray(elements).get() );
}

llvm::DISubroutineType* CodeBuilder::CreateDIType( const FunctionType& type )
{
	U_ASSERT(build_debug_info_);

	ArgsVector<llvm::Metadata*> args;
	args.reserve( type.params.size() + 1u );

	{
		llvm::DIType* di_type= CreateDIType( type.return_type );
		if( type.return_value_type != ValueType::Value )
			di_type= debug_info_.builder->createPointerType( di_type, data_layout_.getTypeAllocSizeInBits( type.return_type.GetLLVMType()->getPointerTo() ) );
		args.push_back( di_type );
	}

	for( const FunctionType::Param& param : type.params )
	{
		llvm::DIType* di_type= CreateDIType( param.type );
		if( param.value_type != ValueType::Value )
			di_type= debug_info_.builder->createPointerType( di_type, data_layout_.getTypeAllocSizeInBits( param.type.GetLLVMType()->getPointerTo() ) );
		args.push_back( di_type );
	}

	return debug_info_.builder->createSubroutineType( debug_info_.builder->getOrCreateTypeArray(args) );
}

llvm::DIDerivedType* CodeBuilder::CreateDIType( const RawPointerType& type )
{
	U_ASSERT(build_debug_info_);

	return
		debug_info_.builder->createPointerType(
			CreateDIType(type.type),
			data_layout_.getTypeAllocSizeInBits(type.llvm_type) );
}

llvm::DIDerivedType* CodeBuilder::CreateDIType( const FunctionPointerType& type )
{
	U_ASSERT(build_debug_info_);

	return
		debug_info_.builder->createPointerType(
			CreateDIType(type.function),
			data_layout_.getTypeAllocSizeInBits(type.llvm_type) );
}

llvm::DICompositeType* CodeBuilder::CreateDIType( const ClassPtr& type )
{
	U_ASSERT(build_debug_info_);

	const Class& the_class= *type;

	// Ignore incomplete type - do not create debug info for it.
	if( !the_class.is_complete )
		return nullptr;

	if( const auto it= debug_info_.classes_di_cache.find(type); it != debug_info_.classes_di_cache.end() )
		return it->second;

	// Insert nullptr first, to prevent loops.
	debug_info_.classes_di_cache.insert( std::make_pair( type, nullptr ) );

	const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( the_class.llvm_type );

	// TODO - get SrcLoc for enum
	const auto di_file= GetDIFile( the_class.body_src_loc.GetFileIndex() );

	std::vector<llvm::Metadata*> fields;
	if( the_class.typeinfo_type == std::nullopt ) // Skip typeinfo, because it may contain recursive structures.
	{
		for( const std::string& name : the_class.fields_order )
		{
			if( name.empty() )
				continue;

			const ClassField& class_field= *the_class.members->GetThisScopeValue( name )->GetClassField();

			llvm::Type* field_type_llvm= class_field.type.GetLLVMType();
			llvm::DIType* field_type_di= CreateDIType( class_field.type );
			if( class_field.is_reference )
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
					di_file,
					name,
					di_file,
					0u, // TODO - src_loc
					data_layout_.getTypeAllocSizeInBits( field_type_llvm ),
					8u * data_layout_.getABITypeAlignment( field_type_llvm ),
					struct_layout.getElementOffsetInBits(class_field.index),
					llvm::DINode::DIFlags(),
					field_type_di );
			fields.push_back(member);
		}

		for( const Class::Parent& parent : the_class.parents )
		{
			llvm::Type* const parent_type_llvm= parent.class_->llvm_type;
			llvm::DIType* parent_type_di= CreateDIType( parent.class_ );

			// If this type is complete, parent types are complete too.
			const auto member =
				debug_info_.builder->createMemberType(
					di_file,
					parent.class_->members->GetThisNamespaceName(),
					di_file,
					0u, // TODO - src_loc
					data_layout_.getTypeAllocSizeInBits( parent_type_llvm ),
					8u * data_layout_.getABITypeAlignment( parent_type_llvm ),
					struct_layout.getElementOffsetInBits( parent.field_number ),
					llvm::DINode::DIFlags(),
					parent_type_di );
			fields.push_back(member);
		}
	}

	const auto result=
		debug_info_.builder->createClassType(
			di_file,
			Type(type).ToString(),
			di_file,
			the_class.body_src_loc.GetLine(),
			data_layout_.getTypeAllocSizeInBits( the_class.llvm_type ),
			8u * data_layout_.getABITypeAlignment( the_class.llvm_type ),
			0u,
			llvm::DINode::DIFlags(),
			nullptr,
			debug_info_.builder->getOrCreateArray(fields).get(),
			nullptr,
			nullptr);

	debug_info_.classes_di_cache[ type ]= result;
	return result;
}

llvm::DICompositeType* CodeBuilder::CreateDIType( const EnumPtr& type )
{
	U_ASSERT(build_debug_info_);

	if( type->syntax_element != nullptr ) // Incomplete
		return nullptr;

	if( const auto it= debug_info_.enums_di_cache.find(type); it != debug_info_.enums_di_cache.end() )
		return it->second;

	std::vector<llvm::Metadata*> elements;
	type->members.ForEachInThisScope(
		[&]( const std::string& name, const Value& value )
		{
			const Variable* const variable= value.GetVariable();
			if( variable == nullptr )
				return;

			U_ASSERT( variable->constexpr_value != nullptr );

			elements.push_back(
				debug_info_.builder->createEnumerator(
					name,
					int64_t(variable->constexpr_value->getUniqueInteger().getLimitedValue()),
					true ) );
		} );

	// TODO - get SrcLoc for enum
	const auto di_file= GetDIFile(0);

	const auto result=
		debug_info_.builder->createEnumerationType(
			di_file,
			type->members.GetThisNamespaceName(),
			di_file,
			0u, // TODO - src_loc
			8u * type->underlaying_type.GetSize(),
			data_layout_.getABITypeAlignment( type->underlaying_type.llvm_type ),
			debug_info_.builder->getOrCreateArray(elements),
			CreateDIType( type->underlaying_type ) );

	debug_info_.enums_di_cache.insert( std::make_pair( type, result ) );
	return result;
}

} // namespace U
