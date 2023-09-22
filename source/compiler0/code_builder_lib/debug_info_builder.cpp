#include "../../lex_synt_lib_common/assert.hpp"
#include "../../sprache_version/sprache_version.hpp"
#include "../../code_builder_lib_common/string_ref.hpp"
#include "class.hpp"
#include "enum.hpp"
#include "debug_info_builder.hpp"

namespace U
{

DebugInfoBuilder::DebugInfoBuilder(
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const SourceGraph& source_graph,
	llvm::Module& llvm_module,
	const bool build_debug_info )
	: llvm_context_(llvm_context)
	, data_layout_(data_layout)
{
	if( !build_debug_info )
		return;

	for( const auto& node : source_graph.nodes_storage )
		source_file_entries_.push_back( llvm::DIFile::get( llvm_context_, node.file_path, "" ) );

	const uint32_t c_dwarf_language_id= llvm::dwarf::DW_LANG_lo_user + 0xDC /* code of "Ü" letter */;

	builder_= std::make_unique<llvm::DIBuilder>( llvm_module );

	std::string version_str= "U+00DC-Sprache compiler ";
	version_str+= getFullVersion();

	compile_unit_=
		builder_->createCompileUnit(
			c_dwarf_language_id,
			source_file_entries_[0],
			version_str,
			false, // optimized
			"",
			0 /* runtime version */ );

	stub_type_= builder_->createBasicType( "_stub_debug_type", 8, llvm::dwarf::DW_ATE_signed );
}

DebugInfoBuilder::~DebugInfoBuilder()
{
	if( builder_ == nullptr )
		return;

	// Remove temporary class forward declarations (without bodies).
	for( const auto& class_di_type_pair : classes_di_cache_ )
	{
		llvm::DIType* const t= class_di_type_pair.second;
		if( t->isTemporary() )
			llvm::MDNode::deleteTemporary(t);
	}

	builder_->finalize(); // We must finalize it.
}

void DebugInfoBuilder::CreateVariableInfo(
	const Variable& variable,
	const std::string_view variable_name,
	const SrcLoc& src_loc,
	FunctionContext& function_context )
{
	if( builder_ == nullptr )
		return;

	const auto di_local_variable=
		builder_->createAutoVariable(
			function_context.current_debug_info_scope,
			StringViewToStringRef( variable_name ),
			GetDIFile( src_loc ),
			src_loc.GetLine(),
			CreateDIType(variable.type) );

	builder_->insertDeclare(
		variable.llvm_value,
		di_local_variable,
		builder_->createExpression(),
		llvm::DILocation::get(llvm_context_, src_loc.GetLine(), src_loc.GetColumn(), function_context.current_debug_info_scope),
		function_context.llvm_ir_builder.GetInsertBlock() );
}

void DebugInfoBuilder::CreateReferenceVariableInfo(
	const Variable& variable,
	const std::string_view variable_name,
	const SrcLoc& src_loc,
	FunctionContext& function_context )
{
	if( builder_ == nullptr )
		return;

	const auto di_local_variable=
		builder_->createAutoVariable(
			function_context.current_debug_info_scope,
			StringViewToStringRef( variable_name ),
			GetDIFile( src_loc ),
			src_loc.GetLine(),
			builder_->createPointerType( CreateDIType(variable.type), data_layout_.getPointerSizeInBits() ) );

	// We needs address for reference, so, move it into stack variable.
	auto address_for_ref= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType()->getPointerTo(), nullptr, StringViewToStringRef( variable_name ) );
	function_context.llvm_ir_builder.CreateStore( variable.llvm_value, address_for_ref );

	builder_->insertDeclare(
		address_for_ref,
		di_local_variable,
		builder_->createExpression(),
		llvm::DILocation::get(llvm_context_, src_loc.GetLine(), src_loc.GetColumn(), function_context.current_debug_info_scope),
		function_context.llvm_ir_builder.GetInsertBlock() );
}

void DebugInfoBuilder::CreateFunctionInfo( const FunctionVariable& func_variable, const std::string_view function_name )
{
	if( builder_ == nullptr )
		return;

	const auto di_function= builder_->createFunction(
		GetDIFile( func_variable.body_src_loc ),
		StringViewToStringRef( function_name ),
		func_variable.llvm_function->name_mangled,
		GetDIFile( func_variable.body_src_loc ),
		func_variable.body_src_loc.GetLine(),
		CreateDIFunctionType( func_variable.type ),
		func_variable.body_src_loc.GetLine(),
		llvm::DINode::FlagPrototyped,
		llvm::DISubprogram::SPFlagDefinition );
	func_variable.llvm_function->function->setSubprogram( di_function );
}

void DebugInfoBuilder::SetCurrentLocation( const SrcLoc& src_loc, FunctionContext& function_context )
{
	if( builder_ == nullptr )
		return;

	function_context.llvm_ir_builder.SetCurrentDebugLocation(
		llvm::DILocation::get(
			llvm_context_,
			src_loc.GetLine(),
			src_loc.GetColumn(),
			function_context.current_debug_info_scope ) );
}

void DebugInfoBuilder::StartBlock( const SrcLoc& src_loc, FunctionContext& function_context )
{
	if( builder_ != nullptr )
		function_context.current_debug_info_scope=
			builder_->createLexicalBlock(
				function_context.current_debug_info_scope,
				GetDIFile( src_loc ),
				src_loc.GetLine(),
				src_loc.GetColumn() );
}

void DebugInfoBuilder::EndBlock( FunctionContext& function_context )
{
	if( builder_ != nullptr )
		function_context.current_debug_info_scope= function_context.current_debug_info_scope->getScope();
}

void DebugInfoBuilder::BuildClassTypeDebugInfo( const ClassPtr class_type )
{
	if( builder_ == nullptr )
		return;

	const Class& the_class= *class_type;
	if( !class_type->is_complete )
		return;

	if( classes_di_cache_.count( class_type ) == 0 )
		return; // Ignore this class - debug info for it was never created.

	const auto di_file= GetDIFile( the_class.body_src_loc );

	const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( the_class.llvm_type );

	ClassFieldsVector<llvm::Metadata*> fields;
	if( the_class.typeinfo_type == std::nullopt ) // Skip typeinfo, because it may contain recursive structures.
	{
		for( const ClassFieldPtr& class_field : the_class.fields_order )
		{
			if( class_field == nullptr )
				continue;

			llvm::Type* field_type_llvm= class_field->type.GetLLVMType();
			llvm::DIType* field_type_di= CreateDIType( class_field->type );
			if( class_field->is_reference )
			{
				field_type_llvm= field_type_llvm->getPointerTo();
				field_type_di=
					builder_->createPointerType(
						field_type_di,
						data_layout_.getTypeAllocSizeInBits(field_type_llvm),
						uint32_t(8u * data_layout_.getABITypeAlignment(field_type_llvm)) );
			}

			// It will be fine - use here data layout queries, because for complete struct type non-reference fields are complete too.
			const auto member =
				builder_->createMemberType(
					di_file,
					class_field->GetName(),
					di_file,
					0u, // TODO - src_loc
					data_layout_.getTypeAllocSizeInBits( field_type_llvm ),
					uint32_t(8u * data_layout_.getABITypeAlignment( field_type_llvm )),
					struct_layout.getElementOffsetInBits(class_field->index),
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
				builder_->createMemberType(
					di_file,
					parent.class_->members->GetThisNamespaceName(),
					di_file,
					0u, // TODO - src_loc
					data_layout_.getTypeAllocSizeInBits( parent_type_llvm ),
					uint32_t(8u * data_layout_.getABITypeAlignment( parent_type_llvm )),
					struct_layout.getElementOffsetInBits( parent.field_number ),
					llvm::DINode::DIFlags(),
					parent_type_di );
			fields.push_back(member);
		}
	}

	const auto result=
		builder_->createClassType(
			di_file,
			Type(class_type).ToString(),
			di_file,
			the_class.body_src_loc.GetLine(),
			data_layout_.getTypeAllocSizeInBits( the_class.llvm_type ),
			uint32_t(8u * data_layout_.getABITypeAlignment( the_class.llvm_type )),
			0u,
			llvm::DINode::DIFlags(),
			nullptr,
			builder_->getOrCreateArray(fields).get(),
			nullptr,
			nullptr);

	const auto cache_value= classes_di_cache_.find( class_type );
	cache_value->second->replaceAllUsesWith( result );
	cache_value->second= result;
}

llvm::DIFile* DebugInfoBuilder::GetDIFile( const SrcLoc& src_loc )
{
	const uint32_t file_index= src_loc.GetFileIndex();
	U_ASSERT( file_index < source_file_entries_.size() );
	return source_file_entries_[file_index];
}

llvm::DIFile* DebugInfoBuilder::GetRootDIFile()
{
	U_ASSERT( !source_file_entries_.empty() );
	return source_file_entries_[0];
}

llvm::DIType* DebugInfoBuilder::CreateDIType( const Type& type )
{
	U_ASSERT(builder_ != nullptr);

	llvm::DIType* result_type= nullptr;
	if( const auto fundamental_type= type.GetFundamentalType() )
		result_type= CreateDIType( *fundamental_type );
	else if( const auto array_type= type.GetArrayType() )
		result_type= CreateDIType( *array_type );
	else if( const auto tuple_type= type.GetTupleType() )
		result_type= CreateDIType( *tuple_type );
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

	return stub_type_;
}

llvm::DIType* DebugInfoBuilder::CreateDIType( const FundamentalType& type )
{
	U_ASSERT(builder_ != nullptr);

	if( type.fundamental_type == U_FundamentalType::void_ )
	{
		// Internal representation of void type is llvm struct with zero elements.
		return builder_->createStructType(
			compile_unit_,
			StringViewToStringRef( GetFundamentalTypeName( type.fundamental_type ) ),
			GetRootDIFile(),
			0u,
			data_layout_.getTypeAllocSizeInBits( type.llvm_type ),
			8u * uint32_t(data_layout_.getABITypeAlignment( type.llvm_type )),
			llvm::DINode::DIFlags(),
			nullptr,
			builder_->getOrCreateArray({}).get() );
	}

	uint32_t type_encoding= llvm::dwarf::DW_ATE_unsigned;
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

	return builder_->createBasicType(
		StringViewToStringRef( GetFundamentalTypeName(type.fundamental_type) ),
		type.GetSize() * 8u,
		type_encoding );
}

llvm::DICompositeType* DebugInfoBuilder::CreateDIType( const ArrayType& type )
{
	U_ASSERT(builder_ != nullptr);

	const uint32_t alignment=
		type.llvm_type->isSized() ? uint32_t(data_layout_.getABITypeAlignment( type.llvm_type )) : 0u;
	const uint64_t size=
		type.llvm_type->isSized() ? data_layout_.getTypeAllocSizeInBits( type.llvm_type ) : uint64_t(0);

	llvm::SmallVector<llvm::Metadata*, 1> subscripts;
	subscripts.push_back( builder_->getOrCreateSubrange( 0, int64_t(type.element_count) ) );

	return
		builder_->createArrayType(
			size,
			8u * alignment,
			CreateDIType( type.element_type ),
			builder_->getOrCreateArray(subscripts) );
}

llvm::DIType* DebugInfoBuilder::CreateDIType( const TupleType& type )
{
	U_ASSERT(builder_ != nullptr);

	if( !type.llvm_type->isSized() )
		return stub_type_;

	const llvm::StructLayout& struct_layout= *data_layout_.getStructLayout( type.llvm_type );

	llvm::SmallVector<llvm::Metadata*, 16> elements;
	elements.reserve( type.element_types.size() );

	const auto di_file= GetRootDIFile();

	for( const Type& element_type : type.element_types )
	{
		if( !element_type.GetLLVMType()->isSized() )
			continue;

		const size_t element_index= size_t(&element_type - type.element_types.data());
		const auto element =
			builder_->createMemberType(
				compile_unit_,
				std::to_string( element_index ),
				di_file,
				0u, // TODO - src_loc
				data_layout_.getTypeAllocSizeInBits( element_type.GetLLVMType() ),
				uint32_t(8u * data_layout_.getABITypeAlignment( element_type.GetLLVMType() )),
				struct_layout.getElementOffsetInBits( uint32_t(element_index) ),
				llvm::DINode::DIFlags(),
				CreateDIType( element_type ) );

		elements.push_back( element );
	}

	return builder_->createStructType(
		compile_unit_,
		Type(type).ToString(),
		di_file,
		0u, // TODO - src_loc
		data_layout_.getTypeAllocSizeInBits( type.llvm_type ),
		uint32_t(8u * data_layout_.getABITypeAlignment( type.llvm_type )),
		llvm::DINode::DIFlags(),
		nullptr,
		builder_->getOrCreateArray(elements).get() );
}

llvm::DIDerivedType* DebugInfoBuilder::CreateDIType( const RawPointerType& type )
{
	U_ASSERT(builder_ != nullptr);

	return
		builder_->createPointerType(
			CreateDIType(type.element_type),
			data_layout_.getTypeAllocSizeInBits(type.llvm_type) );
}

llvm::DIDerivedType* DebugInfoBuilder::CreateDIType( const FunctionPointerType& type )
{
	U_ASSERT(builder_ != nullptr);

	return
		builder_->createPointerType(
			CreateDIFunctionType(type.function_type),
			data_layout_.getTypeAllocSizeInBits(type.llvm_type) );
}

llvm::DIType* DebugInfoBuilder::CreateDIType( const ClassPtr type )
{
	U_ASSERT(builder_ != nullptr);

	const Class& the_class= *type;

	if( const auto it= classes_di_cache_.find(type); it != classes_di_cache_.end() )
		return it->second;

	const auto di_file= GetDIFile( the_class.body_src_loc );

	const auto stub=
		builder_->createReplaceableCompositeType(
			llvm::dwarf::DW_TAG_class_type,
			Type(type).ToString(),
			di_file,
			di_file,
			the_class.body_src_loc.GetLine() );

	classes_di_cache_.insert( std::make_pair( type, stub ) );

	return stub;
}

llvm::DIType* DebugInfoBuilder::CreateDIType( const EnumPtr type )
{
	U_ASSERT(builder_ != nullptr);

	if( type->syntax_element != nullptr ) // Incomplete
		return stub_type_;

	if( const auto it= enums_di_cache_.find(type); it != enums_di_cache_.end() )
		return it->second;

	llvm::SmallVector<llvm::Metadata*, 16> elements;
	type->members.ForEachInThisScope(
		[&]( const std::string_view name, const NamesScopeValue& value )
		{
			const VariablePtr variable= value.value.GetVariable();
			if( variable == nullptr )
				return;

			U_ASSERT( variable->constexpr_value != nullptr );

			elements.push_back(
				builder_->createEnumerator(
					StringViewToStringRef( name ),
					variable->constexpr_value->getUniqueInteger().getLimitedValue(),
					true ) );
		} );

	// TODO - get SrcLoc for enum
	const auto di_file= GetRootDIFile();

	const auto result=
		builder_->createEnumerationType(
			di_file,
			type->members.GetThisNamespaceName(),
			di_file,
			0u, // TODO - src_loc
			8u * type->underlaying_type.GetSize(),
			uint32_t(data_layout_.getABITypeAlignment( type->underlaying_type.llvm_type )),
			builder_->getOrCreateArray(elements),
			CreateDIType( type->underlaying_type ) );

	enums_di_cache_.insert( std::make_pair( type, result ) );
	return result;
}


llvm::DISubroutineType* DebugInfoBuilder::CreateDIFunctionType( const FunctionType& type )
{
	U_ASSERT(builder_ != nullptr);

	llvm::SmallVector<llvm::Metadata*, 16> args;
	args.reserve( type.params.size() + 1u );

	{
		llvm::DIType* di_type= CreateDIType( type.return_type );
		if( type.return_value_type != ValueType::Value )
			di_type= builder_->createPointerType( di_type, data_layout_.getTypeAllocSizeInBits( type.return_type.GetLLVMType()->getPointerTo() ) );
		args.push_back( di_type );
	}

	for( const FunctionType::Param& param : type.params )
	{
		llvm::DIType* di_type= CreateDIType( param.type );
		if( param.value_type != ValueType::Value )
			di_type= builder_->createPointerType( di_type, data_layout_.getTypeAllocSizeInBits( param.type.GetLLVMType()->getPointerTo() ) );
		args.push_back( di_type );
	}

	return builder_->createSubroutineType( builder_->getOrCreateTypeArray(args) );
}

} // namespace U
