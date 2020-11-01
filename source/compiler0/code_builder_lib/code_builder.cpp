#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "../../sprache_version/sprache_version.hpp"
#include "error_reporting.hpp"
#include "mangling.hpp"

#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

CodeBuilder::ReferencesGraphNodeHolder::ReferencesGraphNodeHolder( ReferencesGraphNodePtr node, FunctionContext& function_context )
	: node_(std::move(node)), function_context_(function_context)
{
	function_context_.variables_state.AddNode( node_ );
}

CodeBuilder::ReferencesGraphNodeHolder::ReferencesGraphNodeHolder( ReferencesGraphNodeHolder&& other) noexcept
	: node_(other.node_), function_context_(other.function_context_)
{
	other.node_= nullptr;
}

CodeBuilder::ReferencesGraphNodeHolder& CodeBuilder::ReferencesGraphNodeHolder::operator=( ReferencesGraphNodeHolder&& other ) noexcept
{
	this->node_= other.node_;
	other.node_= nullptr;
	return *this;
}

CodeBuilder::ReferencesGraphNodeHolder::~ReferencesGraphNodeHolder()
{
	if( node_ != nullptr )
		function_context_.variables_state.RemoveNode( node_ );
}

CodeBuilder::CodeBuilder(
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	bool build_debug_info )
	: llvm_context_( llvm_context )
	, data_layout_(data_layout)
	, build_debug_info_( build_debug_info )
	, constexpr_function_evaluator_( data_layout_ )
{
	fundamental_llvm_types_.i8 = llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.u8 = llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.i16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.u16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.i32= llvm::Type::getInt32Ty( llvm_context_ );
	fundamental_llvm_types_.u32= llvm::Type::getInt32Ty( llvm_context_ );
	fundamental_llvm_types_.i64= llvm::Type::getInt64Ty( llvm_context_ );
	fundamental_llvm_types_.u64= llvm::Type::getInt64Ty( llvm_context_ );
	fundamental_llvm_types_.i128= llvm::Type::getInt128Ty( llvm_context_ );
	fundamental_llvm_types_.u128= llvm::Type::getInt128Ty( llvm_context_ );

	fundamental_llvm_types_.f32= llvm::Type::getFloatTy( llvm_context_ );
	fundamental_llvm_types_.f64= llvm::Type::getDoubleTy( llvm_context_ );

	fundamental_llvm_types_.char8 = llvm::Type::getInt8Ty ( llvm_context_ );
	fundamental_llvm_types_.char16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.char32= llvm::Type::getInt32Ty( llvm_context_ );

	fundamental_llvm_types_.invalid_type= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.void_= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.void_for_ret= llvm::Type::getVoidTy( llvm_context_ );
	fundamental_llvm_types_.bool_= llvm::Type::getInt1Ty( llvm_context_ );

	fundamental_llvm_types_.int_ptr= data_layout_.getIntPtrType(llvm_context_);

	invalid_type_= FundamentalType( U_FundamentalType::InvalidType, fundamental_llvm_types_.invalid_type );
	void_type_= FundamentalType( U_FundamentalType::Void, fundamental_llvm_types_.void_ );
	void_type_for_ret_= FundamentalType( U_FundamentalType::Void, fundamental_llvm_types_.void_for_ret );
	bool_type_= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );
	size_type_=
		fundamental_llvm_types_.int_ptr->getIntegerBitWidth() == 32u
		? FundamentalType( U_FundamentalType::u32, fundamental_llvm_types_.u32 )
		: FundamentalType( U_FundamentalType::u64, fundamental_llvm_types_.u64 );
}

CodeBuilder::BuildResult CodeBuilder::BuildProgram( const SourceGraph& source_graph )
{
	global_errors_.clear();

	module_=
		std::make_unique<llvm::Module>(
			source_graph.nodes_storage.front().file_path,
			llvm_context_ );

	// Setup data layout
	module_->setDataLayout(data_layout_);

	// Prepare halt func.
	halt_func_=
		llvm::Function::Create(
			llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret, false ),
			llvm::Function::ExternalLinkage,
			"__U_halt",
			module_.get() );
	halt_func_->setDoesNotReturn();
	halt_func_->setDoesNotThrow();
	halt_func_->addFnAttr(llvm::Attribute::Cold );
	halt_func_->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );

	// In some places outside functions we need to execute expression evaluation.
	// Create for this function context.
	llvm::Function* const global_function=
		llvm::Function::Create(
			llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret, false ),
			llvm::Function::LinkageTypes::ExternalLinkage,
			"",
			module_.get() );

	Function global_function_type;
	global_function_type.return_type= void_type_for_ret_;

	FunctionContext global_function_context(
		std::move(global_function_type),
		void_type_for_ret_,
		llvm_context_,
		global_function );
	const StackVariablesStorage global_function_variables_storage( global_function_context );
	global_function_context_= &global_function_context;

	if( build_debug_info_ )
	{
		for( const auto& node : source_graph.nodes_storage )
			debug_info_.source_file_entries.push_back( llvm::DIFile::get( llvm_context_, node.file_path, "" ) );

		const uint32_t c_dwarf_language_id= 0x8000 /* first user-defined language code */ + 0xDC /* code of "Ü" letter */;

		debug_info_.builder= std::make_unique<llvm::DIBuilder>( *module_ );

		debug_info_.compile_unit=
			debug_info_.builder->createCompileUnit(
				c_dwarf_language_id,
				debug_info_.source_file_entries[0],
				"U+00DC-Sprache compiler " + getFullVersion(),
				false, // optimized
				"",
				0 /* runtime version */ );
	}

	// Build graph.
	BuildProgramInternal( source_graph, 0u );

	global_function->eraseFromParent(); // Kill global function.

	// Fix incomplete typeinfo.
	for( const auto& typeinfo_entry : typeinfo_cache_ )
	{
		if( !typeinfo_entry.second.type.GetLLVMType()->isSized() )
			typeinfo_entry.second.type.GetClassType()->llvm_type->setBody( llvm::ArrayRef<llvm::Type*>() );
	}

	// Finish with debug info.
	if( build_debug_info_ )
	{
		debug_info_.builder->finalize(); // We must finalize it.
	}

	// Clear internal structures.
	compiled_sources_cache_.clear();
	current_class_table_= nullptr;
	enums_table_.clear();
	template_classes_cache_.clear();
	typeinfo_cache_.clear();
	typeinfo_class_table_.clear();
	generated_template_things_storage_.clear();
	debug_info_.builder= nullptr;
	debug_info_.source_file_entries.clear();
	debug_info_.classes_di_cache.clear();
	debug_info_.enums_di_cache.clear();

	global_errors_= NormalizeErrors( global_errors_, *source_graph.macro_expansion_contexts );

	BuildResult build_result;
	build_result.errors.swap( global_errors_ );
	build_result.module.swap( module_ );
	return build_result;
}

CodeBuilder::BuildResultInternal CodeBuilder::BuildProgramInternal(
	const SourceGraph& source_graph,
	const size_t node_index )
{
	BuildResultInternal result;

	result.names_map= std::make_unique<NamesScope>( "", nullptr );
	result.names_map->SetErrors( global_errors_ );
	result.class_table= std::make_unique<ClassTable>();
	FillGlobalNamesScope( *result.names_map );

	U_ASSERT( node_index < source_graph.nodes_storage.size() );
	const SourceGraph::Node& source_graph_node= source_graph.nodes_storage[ node_index ];
	for( const size_t child_node_inex : source_graph_node.child_nodes_indeces )
	{
		// Compile each source once and put it to cache.
		const auto it= compiled_sources_cache_.find( child_node_inex );
		if( it != compiled_sources_cache_.end() )
		{
			SetCurrentClassTable( *it->second.class_table ); // Before merge each ClassProxy must points to members of "dst.names_map".
			MergeNameScopes( *result.names_map, *it->second.names_map, *result.class_table );
			continue;
		}

		BuildResultInternal child_result=
			BuildProgramInternal( source_graph, child_node_inex );

		MergeNameScopes( *result.names_map, *child_result.names_map, *result.class_table );

		compiled_sources_cache_.emplace( child_node_inex, std::move( child_result ) );
	}

	SetCurrentClassTable( *result.class_table );

	// Do work for this node.
	NamesScopeFill( source_graph_node.ast.program_elements, *result.names_map );
	NamesScopeFillOutOfLineElements( source_graph_node.ast.program_elements, *result.names_map );
	GlobalThingBuildNamespace( *result.names_map );

	// Finalize building template classes.
	// Save and update keys separately, because "generated_template_things_storage_" may change during iterations.
	{
		std::unordered_set< TemplateThingKey, TemplateThingKeyHaser > generated_template_things_keys;
		for( const auto& thing_pair : generated_template_things_storage_ )
			generated_template_things_keys.insert(thing_pair.first);
		std::unordered_set< TemplateThingKey, TemplateThingKeyHaser > new_generated_template_things_keys= generated_template_things_keys;

		while(!new_generated_template_things_keys.empty())
		{
			for( const TemplateThingKey& key : new_generated_template_things_keys )
			{
				if( const auto namespace_= generated_template_things_storage_[key].GetNamespace() )
					GlobalThingBuildNamespace( *namespace_ );
			}

			// Collect keys of new things, replace keys set with new one.
			new_generated_template_things_keys.clear();
			for( const auto& thing_pair : generated_template_things_storage_ )
			{
				if( generated_template_things_keys.count(thing_pair.first) == 0 )
				{
					generated_template_things_keys.insert(thing_pair.first);
					new_generated_template_things_keys.insert(thing_pair.first);
				}
			}
		};
	}

	return result;
}

void CodeBuilder::MergeNameScopes( NamesScope& dst, const NamesScope& src, ClassTable& dst_class_table )
{
	src.ForEachInThisScope(
		[&]( const std::string& src_name, const Value& src_member )
		{
			Value* const dst_member= dst.GetThisScopeValue(src_name );
			if( dst_member == nullptr )
			{
				// All ok - name form "src" does not exists in "dst".
				if( const NamesScopePtr names_scope= src_member.GetNamespace() )
				{
					// We copy namespaces, instead of taking same shared pointer,
					// because using same shared pointer we can change state of "src".
					const NamesScopePtr names_scope_copy= std::make_shared<NamesScope>( names_scope->GetThisNamespaceName(), &dst );
					MergeNameScopes( *names_scope_copy, *names_scope, dst_class_table );
					dst.AddName( src_name, Value( names_scope_copy, src_member.GetFilePos() ) );

					names_scope_copy->CopyAccessRightsFrom( *names_scope );
				}
				else
				{
					bool class_copied= false;
					if( const Type* const type= src_member.GetTypeName() )
					{
						if( const ClassProxyPtr class_proxy= type->GetClassTypeProxy() )
						{
							// If current namespace is parent for this class and name is primary.
							if( class_proxy->class_->members.GetParent() == &src &&
								class_proxy->class_->members.GetThisNamespaceName() == src_name )
							{
								CopyClass( src_member.GetFilePos(), class_proxy, dst_class_table, dst );
								class_copied= true;
							}
						}
					}

					if( !class_copied )
						dst.AddName( src_name, src_member );
				}
				return;
			}

			if( dst_member->GetKindIndex() != src_member.GetKindIndex() )
			{
				// Different kind of symbols - 100% error.
				REPORT_ERROR( Redefinition, dst.GetErrors(), src_member.GetFilePos(), src_name );
				return;
			}

			if( const NamesScopePtr sub_namespace= src_member.GetNamespace() )
			{
				// Merge namespaces.
				// TODO - detect here template instantiation namespaces.
				const NamesScopePtr dst_sub_namespace= dst_member->GetNamespace();
				U_ASSERT( dst_sub_namespace != nullptr );
				MergeNameScopes( *dst_sub_namespace, *sub_namespace, dst_class_table );
				return;
			}
			else if(
				OverloadedFunctionsSet* const dst_funcs_set=
				dst_member->GetFunctionsSet() )
			{
				const OverloadedFunctionsSet* const src_funcs_set= src_member.GetFunctionsSet();
				U_ASSERT( src_funcs_set != nullptr );

				for( const FunctionVariable& src_func : src_funcs_set->functions )
				{
					FunctionVariable* same_dst_func=
						GetFunctionWithSameType( *src_func.type.GetFunctionType(), *dst_funcs_set );
					if( same_dst_func != nullptr )
					{
						if( same_dst_func->prototype_file_pos != src_func.prototype_file_pos )
						{
							// Prototypes are in differrent files.
							REPORT_ERROR( FunctionPrototypeDuplication, dst.GetErrors(), src_func.prototype_file_pos, src_name );
							continue;
						}

						if( !same_dst_func->have_body &&  src_func.have_body )
							*same_dst_func= src_func; // Take this function - it have body.
						if(  same_dst_func->have_body && !src_func.have_body )
						{} // Ok, prototype imported later.
						if(  same_dst_func->have_body &&  src_func.have_body &&
							same_dst_func->body_file_pos != src_func.body_file_pos )
							REPORT_ERROR( FunctionBodyDuplication, dst.GetErrors(), src_func.body_file_pos, src_name );
					}
					else
						ApplyOverloadedFunction( *dst_funcs_set, src_func, dst.GetErrors(), src_func.prototype_file_pos );
				}

				// TODO - check duplicates and function templates with same signature.
				for( const FunctionTemplatePtr& function_template : src_funcs_set->template_functions )
				{
					if( std::find( dst_funcs_set->template_functions.begin(), dst_funcs_set->template_functions.end(), function_template ) == dst_funcs_set->template_functions.end() )
						dst_funcs_set->template_functions.push_back( function_template );
				}

				return;
			}
			else if( const Type* const type= dst_member->GetTypeName() )
			{
				if( const ClassProxyPtr dst_class_proxy= type->GetClassTypeProxy() )
				{
					const ClassProxyPtr src_class_proxy= src_member.GetTypeName()->GetClassTypeProxy();

					if( src_class_proxy == nullptr || dst_class_proxy != src_class_proxy )
					{
						// Differnet proxy means 100% different classes.
						REPORT_ERROR( Redefinition, dst.GetErrors(), src_member.GetFilePos(), src_name );
						return;
					}
					if( src_class_proxy->class_->base_template != std::nullopt || dst_class_proxy->class_->base_template != std::nullopt )
						return; // Skip class templates.

					const auto& dst_class= dst_class_table[dst_class_proxy];
					U_ASSERT( dst_class != nullptr );
					const Class& src_class= *src_class_proxy->class_;

					U_ASSERT( dst_class->forward_declaration_file_pos == src_class.forward_declaration_file_pos );

					if( !dst_class->is_complete && !src_class.is_complete )
					{} // Ok
					if( dst_class->is_complete && !src_class.is_complete )
					{} // Dst class is complete, so, use it.
					if( dst_class->is_complete && src_class.is_complete &&
						dst_class->body_file_pos != src_class.body_file_pos )
					{
						// Different bodies from different files.
						REPORT_ERROR( ClassBodyDuplication, dst.GetErrors(), src_class.body_file_pos );
					}
					if( !dst_class->is_complete && src_class.is_complete )
					{
						// Take body of more complete class and store in destintation class table.
						CopyClass( src_class.forward_declaration_file_pos, src_class_proxy, dst_class_table, dst );
					}

					return;
				}
			}

			if( dst_member->GetFilePos() == src_member.GetFilePos() )
				return; // All ok - things from one source.

			// TODO - what about merging type templates sets?

			// Can not merge other kinds of values.
			REPORT_ERROR( Redefinition, dst.GetErrors(), src_member.GetFilePos(), src_name );
		} );
}

void CodeBuilder::CopyClass(
	const FilePos& file_pos,
	const ClassProxyPtr& src_class,
	ClassTable& dst_class_table,
	NamesScope& dst_namespace )
{
	// We make deep copy of Class, imported from other file.
	// This needs for prevention of modification of source class and affection of imported file.

	const Class& src= *src_class->class_;
	auto copy= std::make_unique<Class>( src.members.GetThisNamespaceName(), &dst_namespace );

	// Make deep copy of inner namespace.
	MergeNameScopes( copy->members, src.members, dst_class_table );
	copy->members.CopyAccessRightsFrom( src.members );

	// Copy fields.
	copy->members_visibility= src.members_visibility;

	copy->syntax_element= src.syntax_element;
	copy->field_count= src.field_count;
	copy->inner_reference_type= src.inner_reference_type;
	copy->is_complete= src.is_complete;

	copy->have_explicit_noncopy_constructors= src.have_explicit_noncopy_constructors;
	copy->is_default_constructible= src.is_default_constructible;
	copy->is_copy_constructible= src.is_copy_constructible;
	copy->have_destructor= src.have_destructor;
	copy->is_copy_assignable= src.is_copy_assignable;
	copy->can_be_constexpr= src.can_be_constexpr;
	copy->have_shared_state= src.have_shared_state;

	copy->forward_declaration_file_pos= src.forward_declaration_file_pos;
	copy->body_file_pos= src.body_file_pos;

	copy->fields_order= src.fields_order;

	copy->llvm_type= src.llvm_type;
	copy->base_template= src.base_template;
	copy->typeinfo_type= src.typeinfo_type;

	copy->kind= src.kind;
	copy->base_class= src.base_class;
	copy->parents= src.parents;

	copy->virtual_table= src.virtual_table;
	copy->virtual_table_llvm_type= src.virtual_table_llvm_type;
	copy->virtual_table_llvm_variable= src.virtual_table_llvm_variable;
	copy->polymorph_type_id= src.polymorph_type_id;

	// Register copy in destination namespace and current class table.
	dst_namespace.AddName( src.members.GetThisNamespaceName(), Value( src_class, file_pos ) );
	dst_class_table[ src_class ]= std::move(copy);
}

void CodeBuilder::SetCurrentClassTable( ClassTable& table )
{
	current_class_table_= &table;

	// Make all ClassProxy pointed to Classes from current table.
	for( const ClassTable::value_type& table_entry : table )
		table_entry.first->class_= table_entry.second.get();
}

void CodeBuilder::FillGlobalNamesScope( NamesScope& global_names_scope )
{
	const FilePos fundamental_globals_file_pos( FilePos::c_max_file_index, FilePos::c_max_line, FilePos::c_max_column );

	for( size_t i= size_t(U_FundamentalType::Void); i < size_t(U_FundamentalType::LastType); ++i )
	{
		const U_FundamentalType fundamental_type= U_FundamentalType(i);
		global_names_scope.AddName(
			GetFundamentalTypeName(fundamental_type),
			Value(
				FundamentalType( fundamental_type, GetFundamentalLLVMType( fundamental_type ) ),
				fundamental_globals_file_pos ) );
	}

	global_names_scope.AddName( Keyword( Keywords::size_type_ ), Value( size_type_, fundamental_globals_file_pos ) );
}

void CodeBuilder::TryCallCopyConstructor(
	CodeBuilderErrorsContainer& errors_container,
	const FilePos& file_pos,
	llvm::Value* const this_, llvm::Value* const src,
	const ClassProxyPtr& class_proxy,
	FunctionContext& function_context )
{
	U_ASSERT( class_proxy != nullptr );
	const Type class_type= class_proxy;
	const Class& class_= *class_proxy->class_;

	if( !class_.is_copy_constructible )
	{
		// TODO - print more reliable message.
		REPORT_ERROR( OperationNotSupportedForThisType, errors_container, file_pos, class_type );
		return;
	}

	// Search for copy-constructor.
	const Value* const constructos_value= class_.members.GetThisScopeValue( Keyword( Keywords::constructor_ ) );
	U_ASSERT( constructos_value != nullptr );
	const OverloadedFunctionsSet* const constructors= constructos_value->GetFunctionsSet();
	U_ASSERT(constructors != nullptr );
	const FunctionVariable* constructor= nullptr;
	for( const FunctionVariable& candidate : constructors->functions )
	{
		const Function& constructor_type= *candidate.type.GetFunctionType();
		if( candidate.is_this_call && constructor_type.args.size() == 2u &&
			constructor_type.args.back().type == class_type && constructor_type.args.back().is_reference && !constructor_type.args.back().is_mutable )
		{
			constructor= &candidate;
			break;
		}
	}

	// Call it
	U_ASSERT(constructor != nullptr);
	function_context.llvm_ir_builder.CreateCall( constructor->llvm_function, { this_, src } );

	if( constructor->type.GetFunctionType()->unsafe && !function_context.is_in_unsafe_block )
		REPORT_ERROR( UnsafeFunctionCallOutsideUnsafeBlock, errors_container, file_pos );
}

void CodeBuilder::GenerateLoop(
	const uint64_t iteration_count,
	const std::function<void(llvm::Value* counter_value)>& loop_body,
	FunctionContext& function_context)
{
	U_ASSERT( loop_body != nullptr );
	if( iteration_count == 0u )
		return;

	const auto size_type_llvm= size_type_.GetLLVMType();
	llvm::Value* const zero_value=
		llvm::Constant::getNullValue( size_type_llvm );
	llvm::Value* const one_value=
		llvm::Constant::getIntegerValue( size_type_llvm, llvm::APInt( size_type_llvm->getIntegerBitWidth(), uint64_t(1u) ) );
	llvm::Value* const loop_count_value=
		llvm::Constant::getIntegerValue( size_type_llvm, llvm::APInt( size_type_llvm->getIntegerBitWidth(), uint64_t(iteration_count) ) );

	llvm::BasicBlock* const block_before_loop= function_context.llvm_ir_builder.GetInsertBlock();
	llvm::BasicBlock* const loop_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const block_after_loop= llvm::BasicBlock::Create( llvm_context_ );

	function_context.llvm_ir_builder.CreateBr( loop_block );
	function_context.function->getBasicBlockList().push_back( loop_block );
	function_context.llvm_ir_builder.SetInsertPoint( loop_block );

	llvm::PHINode* const counter_value= function_context.llvm_ir_builder.CreatePHI( size_type_llvm, 2u );
	loop_body( counter_value );

	llvm::Value* const counter_value_plus_one= function_context.llvm_ir_builder.CreateAdd( counter_value, one_value );
	llvm::Value* const counter_test= function_context.llvm_ir_builder.CreateICmpULT( counter_value_plus_one, loop_count_value );
	function_context.llvm_ir_builder.CreateCondBr( counter_test, loop_block, block_after_loop );

	counter_value->addIncoming( zero_value, block_before_loop );
	counter_value->addIncoming( counter_value_plus_one, function_context.llvm_ir_builder.GetInsertBlock() );

	function_context.function->getBasicBlockList().push_back( block_after_loop );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_loop );
}

void CodeBuilder::CallDestructorsImpl(
	const StackVariablesStorage& stack_variables_storage,
	FunctionContext& function_context,
	CodeBuilderErrorsContainer& errors_container,
	const FilePos& file_pos )
{
	// Call destructors in reverse order.
	for( auto it = stack_variables_storage.variables_.rbegin(); it != stack_variables_storage.variables_.rend(); ++it )
	{
		const StackVariablesStorage::NodeAndVariable& stored_variable= *it;

		if( ! function_context.variables_state.NodeMoved( stored_variable.first ) )
		{
			if( stored_variable.first->kind == ReferencesGraphNode::Kind::Variable )
			{
				if( function_context.variables_state.HaveOutgoingLinks( stored_variable.first ) )
					REPORT_ERROR( DestroyedVariableStillHaveReferences, errors_container, file_pos, stored_variable.first->name );
				const Variable& var= stored_variable.second;
				if( var.type.HaveDestructor() )
					CallDestructor( var.llvm_value, var.type, function_context, errors_container, file_pos );
			}
			function_context.variables_state.RemoveNode( stored_variable.first );
		}
	}
}

void CodeBuilder::CallDestructors(
	const StackVariablesStorage& stack_variables_storage,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const FilePos& file_pos )
{
	CallDestructorsImpl( stack_variables_storage, function_context, names_scope.GetErrors(), file_pos );
}

void CodeBuilder::CallDestructor(
	llvm::Value* const ptr,
	const Type& type,
	FunctionContext& function_context,
	CodeBuilderErrorsContainer& errors_container,
	const FilePos& file_pos )
{
	U_ASSERT( type.HaveDestructor() );

	if( const Class* const class_= type.GetClassType() )
	{
		const Value* const destructor_value= class_->members.GetThisScopeValue( Keyword( Keywords::destructor_ ) );
		U_ASSERT( destructor_value != nullptr );
		const OverloadedFunctionsSet* const destructors= destructor_value->GetFunctionsSet();
		U_ASSERT(destructors != nullptr && destructors->functions.size() == 1u );

		const FunctionVariable& destructor= destructors->functions.front();
		function_context.llvm_ir_builder.CreateCall( destructor.llvm_function, { ptr } );

		if( destructor.type.GetFunctionType()->unsafe && !function_context.is_in_unsafe_block )
			REPORT_ERROR( UnsafeFunctionCallOutsideUnsafeBlock, errors_container, file_pos );
	}
	else if( const Array* const array_type= type.GetArrayType() )
	{
		// SPRACHE_TODO - maybe call destructors of arrays in reverse order?
		GenerateLoop(
			array_type->size,
			[&]( llvm::Value* const index )
			{
				CallDestructor(
					function_context.llvm_ir_builder.CreateGEP( ptr, { GetZeroGEPIndex(), index } ),
					array_type->type,
					function_context,
					errors_container,
					file_pos );
			},
			function_context );
	}
	else if( const Tuple* const tuple_type= type.GetTupleType() )
	{
		for( const Type& element_type : tuple_type->elements )
		{
			if( element_type.HaveDestructor() )
				CallDestructor(
					function_context.llvm_ir_builder.CreateGEP( ptr, { GetZeroGEPIndex(), GetFieldGEPIndex( size_t(&element_type - tuple_type->elements.data()) ) } ),
					element_type,
					function_context,
					errors_container,
					file_pos );
		}
	}
	else U_ASSERT(false);
}

void CodeBuilder::CallDestructorsForLoopInnerVariables( NamesScope& names_scope, FunctionContext& function_context, const FilePos& file_pos )
{
	U_ASSERT( !function_context.loops_stack.empty() );

	// Destroy all local variables before "break"/"continue" in all blocks inside loop.
	size_t undestructed_stack_size= function_context.stack_variables_stack.size();
	for(
		auto it= function_context.stack_variables_stack.rbegin();
		it != function_context.stack_variables_stack.rend() &&
		undestructed_stack_size > function_context.loops_stack.back().stack_variables_stack_size;
		++it, --undestructed_stack_size )
	{
		CallDestructorsImpl( **it, function_context, names_scope.GetErrors(), file_pos );
	}
}

void CodeBuilder::CallDestructorsBeforeReturn( NamesScope& names_scope, FunctionContext& function_context, const FilePos& file_pos )
{
	// We must call ALL destructors of local variables, arguments, etc before each return.
	for( auto it= function_context.stack_variables_stack.rbegin(); it != function_context.stack_variables_stack.rend(); ++it )
		CallDestructorsImpl( **it, function_context, names_scope.GetErrors(), file_pos );
}

void CodeBuilder::CallMembersDestructors( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const FilePos& file_pos )
{
	U_ASSERT( function_context.this_ != nullptr );
	const Class* const class_= function_context.this_->type.GetClassType();
	U_ASSERT( class_ != nullptr );

	for( size_t i= 0u; i < class_->parents.size(); ++i )
	{
		U_ASSERT( class_->parents[i].class_->class_->have_destructor ); // Parents are polymorph, polymorph classes always have destructors.
		CallDestructor(
			function_context.llvm_ir_builder.CreateGEP(
				function_context.this_->llvm_value,
				{ GetZeroGEPIndex(), GetFieldGEPIndex( class_->parents[i].field_number ) } ),
			class_->parents[i].class_,
			function_context,
			errors_container,
			file_pos );
	}

	for( const std::string& field_name : class_->fields_order )
	{
		if( field_name.empty() )
			continue;

		const ClassField& field= *class_->members.GetThisScopeValue( field_name )->GetClassField();
		if( !field.type.HaveDestructor() || field.is_reference )
			continue;

		CallDestructor(
			function_context.llvm_ir_builder.CreateGEP(
				function_context.this_->llvm_value,
				{ GetZeroGEPIndex(), GetFieldGEPIndex(field.index) } ),
			field.type,
			function_context,
			errors_container,
			file_pos );
	};
}

size_t CodeBuilder::PrepareFunction(
	NamesScope& names_scope,
	const ClassProxyPtr& base_class,
	OverloadedFunctionsSet& functions_set,
	const Synt::Function& func,
	const bool is_out_of_line_function )
{
	const std::string& func_name= func.name_.back();
	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	const bool is_special_method= is_constructor || is_destructor;

	if( is_destructor || is_constructor )
		U_ASSERT( func.type_.arguments_.size() >= 1u && func.type_.arguments_.front().name_ == Keywords::this_ );

	if( !is_special_method && IsKeyword( func_name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), func.file_pos_ );

	if( is_special_method && base_class == nullptr )
	{
		REPORT_ERROR( ConstructorOrDestructorOutsideClass, names_scope.GetErrors(), func.file_pos_ );
		return ~0u;
	}
	if( !is_constructor && func.constructor_initialization_list_ != nullptr )
	{
		REPORT_ERROR( InitializationListInNonconstructor, names_scope.GetErrors(), func.constructor_initialization_list_->file_pos_ );
		return ~0u;
	}
	if( is_destructor && func.type_.arguments_.size() >= 2u )
	{
		REPORT_ERROR( ExplicitArgumentsInDestructor, names_scope.GetErrors(), func.file_pos_ );
		return ~0u;
	}

	if( std::get_if<Synt::EmptyVariant>( &func.condition_ ) == nullptr )
	{
		const Variable expression= BuildExpressionCodeEnsureVariable( func.condition_, names_scope, *global_function_context_ );
		if( expression.type == bool_type_ )
		{
			if( expression.constexpr_value != nullptr )
			{
				if( expression.constexpr_value->isZeroValue() )
					return ~0u; // Function disabled.
			}
			else
				REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), Synt::GetExpressionFilePos( func.condition_ ) );
		}
		else
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), Synt::GetExpressionFilePos( func.condition_ ), bool_type_, expression.type );
	}

	FunctionVariable func_variable;
	func_variable.type= Function();
	{ // Prepare function type
		Function& function_type= *func_variable.type.GetFunctionType();

		if( func.type_.return_type_ == nullptr )
			function_type.return_type= void_type_for_ret_;
		else
		{
			if( const auto named_return_type = std::get_if<Synt::NamedTypeName>(func.type_.return_type_.get()) )
			{
				// TODO - set flag "auto" in syntax analyzer.
				if( named_return_type->name.tail == nullptr &&
					std::get_if<std::string>( &named_return_type->name.start_value ) != nullptr &&
					std::get<std::string>( named_return_type->name.start_value ) == Keywords::auto_ )
				{
					func_variable.return_type_is_auto= true;
					if( base_class != nullptr )
						REPORT_ERROR( AutoFunctionInsideClassesNotAllowed, names_scope.GetErrors(), func.file_pos_, func_name );
					if( func.block_ == nullptr )
						REPORT_ERROR( ExpectedBodyForAutoFunction, names_scope.GetErrors(), func.file_pos_, func_name );

					if( func.type_.return_value_reference_modifier_ == ReferenceModifier::Reference )
						function_type.return_type= void_type_;
					else
						function_type.return_type= void_type_for_ret_;
				}
			}

			if( !func_variable.return_type_is_auto )
			{
				function_type.return_type= PrepareType( *func.type_.return_type_, names_scope, *global_function_context_ );
				if( function_type.return_type == invalid_type_ )
					return ~0u;
			}
		}

		function_type.return_value_is_mutable= func.type_.return_value_mutability_modifier_ == MutabilityModifier::Mutable;
		function_type.return_value_is_reference= func.type_.return_value_reference_modifier_ == ReferenceModifier::Reference;

		// HACK. We have different llvm types for "void".
		// llvm::void used only for empty return value, for other purposes we use "i8" for Ü::void.
		if( !function_type.return_value_is_reference && function_type.return_type == void_type_ )
			function_type.return_type= void_type_for_ret_;

		if( !function_type.return_value_is_reference &&
			!( function_type.return_type.GetFundamentalType() != nullptr ||
			   function_type.return_type.GetClassType() != nullptr ||
			   function_type.return_type.GetTupleType() != nullptr ||
			   function_type.return_type.GetEnumType() != nullptr ||
			   function_type.return_type.GetFunctionPointerType() != nullptr ) )
		{
			REPORT_ERROR( NotImplemented, names_scope.GetErrors(), func.file_pos_, "return value types except fundamentals, enums, classes, function pointers" );
			return ~0u;
		}

		if( is_special_method && !( function_type.return_type == void_type_ && !function_type.return_value_is_reference ) )
			REPORT_ERROR( ConstructorAndDestructorMustReturnVoid, names_scope.GetErrors(), func.file_pos_ );

		ProcessFunctionReturnValueReferenceTags( names_scope.GetErrors(), func.type_, function_type );

		// Args.
		function_type.args.reserve( func.type_.arguments_.size() );

		for( const Synt::FunctionArgument& arg : func.type_.arguments_ )
		{
			const bool is_this=
				&arg == &func.type_.arguments_.front() &&
				arg.name_ == Keywords::this_ &&
				std::get_if<Synt::EmptyVariant>(&arg.type_) != nullptr;

			if( !is_this && IsKeyword( arg.name_ ) )
				REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), arg.file_pos_ );

			function_type.args.emplace_back();
			Function::Arg& out_arg= function_type.args.back();

			if( is_this )
			{
				func_variable.is_this_call= true;
				if( base_class == nullptr )
				{
					REPORT_ERROR( ThisInNonclassFunction, names_scope.GetErrors(), arg.file_pos_, func_name );
					return ~0u;
				}
				out_arg.type= base_class;
			}
			else
				out_arg.type= PrepareType( arg.type_, names_scope, *global_function_context_ );

			out_arg.is_mutable= ( is_this && is_special_method ) || arg.mutability_modifier_ == MutabilityModifier::Mutable;
			out_arg.is_reference= is_this || arg.reference_modifier_ == ReferenceModifier::Reference;

			if( !out_arg.is_reference &&
				!( out_arg.type.GetFundamentalType() != nullptr ||
				   out_arg.type.GetClassType() != nullptr ||
				   out_arg.type.GetTupleType() != nullptr ||
				   out_arg.type.GetEnumType() != nullptr ||
				   out_arg.type.GetFunctionPointerType() != nullptr ) )
			{
				REPORT_ERROR( NotImplemented, names_scope.GetErrors(), func.file_pos_, "parameters types except fundamentals, classes, enums, functionpointers" );
				return ~0u;
			}

			ProcessFunctionArgReferencesTags( names_scope.GetErrors(), func.type_, function_type, arg, out_arg, function_type.args.size() - 1u );
		} // for arguments

		function_type.unsafe= func.type_.unsafe_;

		TryGenerateFunctionReturnReferencesMapping( names_scope.GetErrors(), func.type_, function_type );
		ProcessFunctionReferencesPollution( names_scope.GetErrors(), func, function_type, base_class );
		CheckOverloadedOperator( base_class, function_type, func.overloaded_operator_, names_scope.GetErrors(), func.file_pos_ );

	} // end prepare function type

	// Set constexpr.
	if( func.constexpr_ )
	{
		if( func.block_ == nullptr )
			REPORT_ERROR( ConstexprFunctionsMustHaveBody, names_scope.GetErrors(), func.file_pos_ );
		if( func.virtual_function_kind_ != Synt::VirtualFunctionKind::None )
			REPORT_ERROR( ConstexprFunctionCanNotBeVirtual, names_scope.GetErrors(), func.file_pos_ );

		func_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprIncomplete;
	}
	else if( func.is_template_ )
		func_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprAuto;

	// Set virtual.
	if( func.virtual_function_kind_ != Synt::VirtualFunctionKind::None )
	{
		if( base_class == nullptr )
			REPORT_ERROR( VirtualForNonclassFunction, names_scope.GetErrors(), func.file_pos_, func_name );
		if( !func_variable.is_this_call )
			REPORT_ERROR( VirtualForNonThisCallFunction, names_scope.GetErrors(), func.file_pos_, func_name );
		if( is_constructor )
			REPORT_ERROR( FunctionCanNotBeVirtual, names_scope.GetErrors(), func.file_pos_, func_name );
		if( base_class != nullptr && ( base_class->class_->kind == Class::Kind::Struct || base_class->class_->kind == Class::Kind::NonPolymorph ) )
			REPORT_ERROR( VirtualForNonpolymorphClass, names_scope.GetErrors(), func.file_pos_, func_name );
		if( is_out_of_line_function )
			REPORT_ERROR( VirtualForFunctionImplementation, names_scope.GetErrors(), func.file_pos_, func_name );

		func_variable.virtual_function_kind= func.virtual_function_kind_;
	}

	// Set no_mangle
	if( func.no_mangle_ )
	{
		// Allow only global no-mangle function. This prevents existing of multiple "nomangle" functions with same name in different namespaces.
		// If function is operator, it can not be global.
		if( names_scope.GetParent() != nullptr )
			REPORT_ERROR( NoMangleForNonglobalFunction, names_scope.GetErrors(), func.file_pos_, func_name );
		func_variable.no_mangle= true;
	}

	// Set conversion constructor.
	func_variable.is_conversion_constructor= func.is_conversion_constructor_;
	U_ASSERT( !( func.is_conversion_constructor_ && !is_constructor ) );
	if( func.is_conversion_constructor_ && func_variable.type.GetFunctionType()->args.size() != 2u )
		REPORT_ERROR( ConversionConstructorMustHaveOneArgument, names_scope.GetErrors(), func.file_pos_ );
	func_variable.is_constructor= is_constructor;

	// Check "=default" / "=delete".
	if( func.body_kind != Synt::Function::BodyKind::None )
	{
		U_ASSERT( func.block_ == nullptr );
		const Function& function_type= *func_variable.type.GetFunctionType();

		bool invalid_func= false;
		if( base_class == nullptr )
			invalid_func= true;
		else if( is_constructor )
			invalid_func= !( IsDefaultConstructor( function_type, base_class ) || IsCopyConstructor( function_type, base_class ) );
		else if( func.overloaded_operator_ == OverloadedOperator::Assign )
			invalid_func= !IsCopyAssignmentOperator( function_type, base_class );
		else
			invalid_func= true;

		if( invalid_func )
			REPORT_ERROR( InvalidMethodForBodyGeneration, names_scope.GetErrors(), func.file_pos_ );
		else
		{
			if( func.body_kind == Synt::Function::BodyKind::BodyGenerationRequired )
				func_variable.is_generated= true;
			else
				func_variable.is_deleted= true;
		}
	}

	if( FunctionVariable* const prev_function= GetFunctionWithSameType( *func_variable.type.GetFunctionType(), functions_set ) )
	{
			 if( prev_function->syntax_element->block_ == nullptr && func.block_ != nullptr )
		{ // Ok, body after prototype.
			prev_function->syntax_element= &func;
			prev_function->body_file_pos= func.file_pos_;
		}
		else if( prev_function->syntax_element->block_ != nullptr && func.block_ == nullptr )
		{ // Ok, prototype after body. Since order-independent resolving this is correct.
			prev_function->prototype_file_pos= func.file_pos_;
		}
		else if( prev_function->syntax_element->block_ == nullptr && func.block_ == nullptr )
			REPORT_ERROR( FunctionPrototypeDuplication, names_scope.GetErrors(), func.file_pos_, func_name );
		else if( prev_function->syntax_element->block_ != nullptr && func.block_ != nullptr )
			REPORT_ERROR( FunctionBodyDuplication, names_scope.GetErrors(), func.file_pos_, func_name );

		if( prev_function->is_this_call != func_variable.is_this_call )
			REPORT_ERROR( ThiscallMismatch, names_scope.GetErrors(), func.file_pos_, func_name );

		if( !is_out_of_line_function )
		{
			if( prev_function->virtual_function_kind != func.virtual_function_kind_ )
				REPORT_ERROR( VirtualMismatch, names_scope.GetErrors(), func.file_pos_, func_name );
		}
		if( prev_function->is_deleted != func_variable.is_deleted )
			REPORT_ERROR( BodyForDeletedFunction, names_scope.GetErrors(), prev_function->prototype_file_pos, func_name );
		if( prev_function->is_generated != func_variable.is_generated )
			REPORT_ERROR( BodyForGeneratedFunction, names_scope.GetErrors(), prev_function->prototype_file_pos, func_name );

		if( prev_function->no_mangle != func_variable.no_mangle )
			REPORT_ERROR( NoMangleMismatch, names_scope.GetErrors(), func.file_pos_, func_name );

		if( prev_function->is_conversion_constructor != func_variable.is_conversion_constructor )
			REPORT_ERROR( CouldNotOverloadFunction, names_scope.GetErrors(), func.file_pos_ );

		return size_t(prev_function - functions_set.functions.data());
	}
	else
	{
		if( is_out_of_line_function )
		{
			REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), func.file_pos_ );
			return ~0u;
		}
		if( functions_set.have_nomangle_function || ( !functions_set.functions.empty() && func_variable.no_mangle ) )
		{
			REPORT_ERROR( CouldNotOverloadFunction, names_scope.GetErrors(), func.file_pos_ );
			return ~0u;
		}

		const bool overloading_ok= ApplyOverloadedFunction( functions_set, func_variable, names_scope.GetErrors(), func.file_pos_ );
		if( !overloading_ok )
			return ~0u;

		if( func_variable.no_mangle )
			functions_set.have_nomangle_function= true;

		FunctionVariable& inserted_func_variable= functions_set.functions.back();
		inserted_func_variable.body_file_pos= inserted_func_variable.prototype_file_pos= func.file_pos_;
		inserted_func_variable.syntax_element= &func;

		BuildFuncCode(
			inserted_func_variable,
			base_class,
			names_scope,
			func_name,
			func.type_.arguments_,
			nullptr,
			func.constructor_initialization_list_.get() );

		return functions_set.functions.size() - 1u;
	}
}

void CodeBuilder::CheckOverloadedOperator(
	const ClassProxyPtr& base_class,
	const Function& func_type,
	const OverloadedOperator overloaded_operator,
	CodeBuilderErrorsContainer& errors_container,
	const FilePos& file_pos )
{
	if( overloaded_operator == OverloadedOperator::None )
		return; // Not operator

	if( base_class == nullptr )
	{
		REPORT_ERROR( OperatorDeclarationOutsideClass, errors_container, file_pos );
		return;
	}

	bool is_this_class= false;
	for( const Function::Arg& arg : func_type.args )
	{
		if( arg.type == base_class )
		{
			is_this_class= true;
			break;
		}
	}

	if( !is_this_class )
		REPORT_ERROR( OperatorDoesNotHaveParentClassArguments, errors_container, file_pos );

	const bool ret_is_void=
		( func_type.return_type == void_type_ || func_type.return_type == void_type_for_ret_ ) &&
		!func_type.return_value_is_reference;

	switch( overloaded_operator )
	{
	case OverloadedOperator::Add:
	case OverloadedOperator::Sub:
		if( !( func_type.args.size() == 1u || func_type.args.size() == 2u ) )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, file_pos );
		break;

	case OverloadedOperator::Equal:
	case OverloadedOperator::NotEqual:
	case OverloadedOperator::Less:
	case OverloadedOperator::LessEqual:
	case OverloadedOperator::Greater:
	case OverloadedOperator::GreaterEqual:
		if( func_type.args.size() != 2u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, file_pos );
		if( !( func_type.return_type == bool_type_ && !func_type.return_value_is_reference ) )
			REPORT_ERROR( InvalidReturnTypeForOperator, errors_container, file_pos, bool_type_ );
		break;
		
	case OverloadedOperator::Mul:
	case OverloadedOperator::Div:
	case OverloadedOperator::Rem:
	case OverloadedOperator::And:
	case OverloadedOperator::Or :
	case OverloadedOperator::Xor:
	case OverloadedOperator::ShiftLeft :
	case OverloadedOperator::ShiftRight:
		if( func_type.args.size() != 2u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, file_pos );
		break;

	case OverloadedOperator::AssignAdd:
	case OverloadedOperator::AssignSub:
	case OverloadedOperator::AssignMul:
	case OverloadedOperator::AssignDiv:
	case OverloadedOperator::AssignRem:
	case OverloadedOperator::AssignAnd:
	case OverloadedOperator::AssignOr :
	case OverloadedOperator::AssignXor:
	case OverloadedOperator::AssignShiftLeft :
	case OverloadedOperator::AssignShiftRight:
		if( func_type.args.size() != 2u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, file_pos );
		if( !ret_is_void )
			REPORT_ERROR( InvalidReturnTypeForOperator, errors_container, file_pos, void_type_ );
		break;

	case OverloadedOperator::LogicalNot:
	case OverloadedOperator::BitwiseNot:
		if( func_type.args.size() != 1u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, file_pos );
		break;

	case OverloadedOperator::Assign:
		if( func_type.args.size() != 2u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, file_pos );
		if( !ret_is_void )
			REPORT_ERROR( InvalidReturnTypeForOperator, errors_container, file_pos, void_type_ );
		break;

	case OverloadedOperator::Increment:
	case OverloadedOperator::Decrement:
		if( func_type.args.size() != 1u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, file_pos );
		if( !ret_is_void )
			REPORT_ERROR( InvalidReturnTypeForOperator, errors_container, file_pos, void_type_ );
		break;

	case OverloadedOperator::Indexing:
		if( func_type.args.size() != 2u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, file_pos );
		// Indexing operator must have first argument of parent class.
		if( !func_type.args.empty() && func_type.args[0].type != base_class )
			REPORT_ERROR( OperatorDoesNotHaveParentClassArguments, errors_container, file_pos );
		break;

	case OverloadedOperator::Call:
		if( func_type.args.empty() )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, file_pos );
		// Call operator must have first argument of parent class.
		if( !func_type.args.empty() && func_type.args[0].type != base_class )
			REPORT_ERROR( OperatorDoesNotHaveParentClassArguments, errors_container, file_pos );
		break;

	case OverloadedOperator::None:
		U_ASSERT(false);
	};
}

Type CodeBuilder::BuildFuncCode(
	FunctionVariable& func_variable,
	const ClassProxyPtr& base_class,
	NamesScope& parent_names_scope,
	const std::string& func_name,
	const Synt::FunctionArgumentsDeclaration& args,
	const Synt::Block* const block,
	const Synt::StructNamedInitializer* const constructor_initialization_list )
{
	Function& function_type= *func_variable.type.GetFunctionType();
	function_type.llvm_function_type= GetLLVMFunctionType( function_type );

	const bool first_arg_is_sret=
		function_type.llvm_function_type->getReturnType()->isVoidTy() && function_type.return_type != void_type_;

	llvm::Function* llvm_function;
	if( func_variable.llvm_function == nullptr )
	{
		llvm_function=
			llvm::Function::Create(
				function_type.llvm_function_type,
				llvm::Function::LinkageTypes::ExternalLinkage, // External - for prototype.
				func_variable.no_mangle ? func_name : ( parent_names_scope.IsInsideTemplate() ? "_tf" : MangleFunction( parent_names_scope, func_name, function_type ) ),
				module_.get() );

		// Merge functions with identical code.
		// We doesn`t need different addresses for different functions.
		llvm_function->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );

		// Mark reference-parameters as nonnull.
		// Mark mutable references as "noalias".
		for( size_t i= 0u; i < function_type.args.size(); i++ )
		{
			const unsigned int arg_attr_index= static_cast<unsigned int>(i + 1u + (first_arg_is_sret ? 1u : 0u ));
			const Function::Arg& arg= function_type.args[i];
			if( arg.is_reference || arg.type.GetClassType() != nullptr || arg.type.GetTupleType() )
				llvm_function->addAttribute( arg_attr_index, llvm::Attribute::NonNull );
			if( arg.is_reference && arg.is_mutable )
				llvm_function->addAttribute( arg_attr_index, llvm::Attribute::NoAlias );
		}

		if( first_arg_is_sret )
		{
			llvm_function->addAttribute( 1u, llvm::Attribute::StructRet );
			llvm_function->addAttribute( 1u, llvm::Attribute::NoAlias );
		}

		func_variable.llvm_function= llvm_function;

		CreateFunctionDebugInfo( func_variable, func_name );
	}
	else
		llvm_function= func_variable.llvm_function;

	if( block == nullptr )
	{
		// This is only prototype, then, function preparing work is done.
		func_variable.have_body= false;
		return function_type.return_type;
	}

	// For functions with body we can use comdat.
	if( parent_names_scope.IsInsideTemplate() )
		llvm_function->setLinkage( llvm::GlobalValue::PrivateLinkage );
	else
	{
		// Set comdat for correct linkage of same functions, emitted in several modules.
		llvm::Comdat* const comdat= module_->getOrInsertComdat( llvm_function->getName() );
		comdat->setSelectionKind( llvm::Comdat::Any );
		llvm_function->setComdat( comdat );
	}
	llvm_function->setDoesNotThrow(); // We do not support exceptions.

	if( build_debug_info_ ) // Unwind table entry for function needed for debug info.
		llvm_function->addFnAttr( llvm::Attribute::UWTable );

	func_variable.have_body= true;

	// Ensure completeness only for functions body.
	// Require full completeness even for reference arguments.
	for( const Function::Arg& arg : function_type.args )
	{
		if( arg.type != void_type_ && !EnsureTypeComplete( arg.type ) )
			REPORT_ERROR( UsingIncompleteType, parent_names_scope.GetErrors(), args.front().file_pos_, arg.type );
	}
	if( !function_type.return_value_is_reference && function_type.return_type != void_type_ &&
		!EnsureTypeComplete( function_type.return_type ) )
		REPORT_ERROR( UsingIncompleteType, parent_names_scope.GetErrors(), func_variable.body_file_pos, function_type.return_type );

	NamesScope function_names( "", &parent_names_scope );
	FunctionContext function_context(
		function_type,
		func_variable.return_type_is_auto ? std::optional<Type>(): function_type.return_type,
		llvm_context_,
		llvm_function );
	const StackVariablesStorage args_storage( function_context );

	function_context.args_nodes.resize( function_type.args.size() );

	SetCurrentDebugLocation( func_variable.body_file_pos, function_context );

	// push args
	Variable this_;
	unsigned int arg_number= 0u;

	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	for( llvm::Argument& llvm_arg : llvm_function->args() )
	{
		// Skip "sret".
		if( first_arg_is_sret && &llvm_arg == &*llvm_function->arg_begin() )
		{
			llvm_arg.setName( "_return_value" );
			function_context.s_ret_= &llvm_arg;
			continue;
		}

		const Function::Arg& arg= function_type.args[ arg_number ];

		const Synt::FunctionArgument& declaration_arg= args[arg_number ];
		const std::string& arg_name= declaration_arg.name_;

		const bool is_this= arg_number == 0u && arg_name == Keywords::this_;
		U_ASSERT( !( is_this && !arg.is_reference ) );

		Variable var;
		var.location= Variable::Location::LLVMRegister;
		var.value_type= ValueType::Reference;
		var.type= arg.type;
		var.llvm_value= &llvm_arg;

		if( declaration_arg.mutability_modifier_ != MutabilityModifier::Mutable )
			var.value_type= ValueType::ConstReference;

		if( arg.is_reference )
		{
			var.location= Variable::Location::Pointer;
			CreateReferenceVariableDebugInfo( var, arg_name, declaration_arg.file_pos_, function_context );
		}
		else
		{
			if( arg.type.GetFundamentalType() != nullptr ||
				arg.type.GetEnumType() != nullptr ||
				arg.type.GetFunctionPointerType() != nullptr )
			{
				// Move parameters to stack for assignment possibility.
				// TODO - do it, only if parameters are not constant.
				llvm::Value* address= function_context.alloca_ir_builder.CreateAlloca( var.type.GetLLVMType() );
				address->setName( arg_name );
				function_context.llvm_ir_builder.CreateStore( var.llvm_value, address );

				var.llvm_value= address;
				var.location= Variable::Location::Pointer;
			}
			else if( arg.type.GetClassType() != nullptr || arg.type.GetTupleType() != nullptr )
			{
				// Value classes and tuples parameters using llvm-pointers.
				var.location= Variable::Location::Pointer;
			}
			else U_ASSERT(false);

			CreateVariableDebugInfo( var, arg_name, declaration_arg.file_pos_, function_context );
		}

		// Create variable node, because only variable node can have inner reference node.
		// Register arg on stack, only if it is value-argument.
		const auto var_node= std::make_shared<ReferencesGraphNode>( arg_name, ReferencesGraphNode::Kind::Variable );
		function_context.args_nodes[ arg_number ].first= var_node;
		if( arg.is_reference )
		{
			function_context.variables_state.AddNode( var_node );

			const auto reference_node= std::make_shared<ReferencesGraphNode>(
				arg_name + " reference",
				arg.is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
			function_context.variables_state.AddNode( reference_node );
			function_context.variables_state.AddLink( var_node, reference_node );
			var.node= reference_node;
		}
		else
		{
			function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( var_node, var ) );
			var.node= var_node;
		}

		if (arg.type.ReferencesTagsCount() > 0u )
		{
			// Create inner node + root variable.
			const auto accesible_variable= std::make_shared<ReferencesGraphNode>( arg_name + " inner variable", ReferencesGraphNode::Kind::Variable );
			function_context.variables_state.AddNode( accesible_variable );

			const auto inner_reference= std::make_shared<ReferencesGraphNode>(
				arg_name + " inner reference",
				arg.type.GetInnerReferenceType() == InnerReferenceType::Mut ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
			function_context.variables_state.SetNodeInnerReference( var_node, inner_reference );
			function_context.variables_state.AddLink( accesible_variable, inner_reference );

			function_context.args_nodes[ arg_number ].second= accesible_variable;
		}

		if( is_this )
		{
			// Save "this" in function context for accessing inside class methods.
			this_= std::move(var);
			function_context.this_= &this_;
		}
		else
		{
			if( NameShadowsTemplateArgument( arg_name, function_names ) )
				REPORT_ERROR( DeclarationShadowsTemplateArgument, function_names.GetErrors(), declaration_arg.file_pos_, arg_name );

			const Value* const inserted_arg=
				function_names.AddName( arg_name, Value( var, declaration_arg.file_pos_ ) );
			if( inserted_arg == nullptr )
				REPORT_ERROR( Redefinition, function_names.GetErrors(), declaration_arg.file_pos_, arg_name );
		}

		llvm_arg.setName( "_arg_" + arg_name );
		++arg_number;
	}

	if( is_constructor )
	{
		U_ASSERT( base_class != nullptr );
		U_ASSERT( function_context.this_ != nullptr );

		function_context.whole_this_is_unavailable= true;

		if( constructor_initialization_list == nullptr )
		{
			// Create dummy initialization list for constructors without explicit initialization list.
			const Synt::StructNamedInitializer dumy_initialization_list( block->file_pos_ );

			BuildConstructorInitialization(
				*function_context.this_,
				*base_class->class_,
				function_names,
				function_context,
				dumy_initialization_list );
		}
		else
			BuildConstructorInitialization(
				*function_context.this_,
				*base_class->class_,
				function_names,
				function_context,
				*constructor_initialization_list );

		function_context.whole_this_is_unavailable= false;
	}

	if( ( is_constructor || is_destructor ) && ( base_class->class_->kind == Class::Kind::Abstract || base_class->class_->kind == Class::Kind::Interface ) )
		function_context.whole_this_is_unavailable= true; // Whole "this" unavailable in body of constructors and destructors of abstract classes and interfaces.

	if( is_destructor )
	{
		SetupVirtualTablePointers( this_.llvm_value, *base_class->class_, function_context );
		function_context.destructor_end_block= llvm::BasicBlock::Create( llvm_context_ );
	}

	const BlockBuildInfo block_build_info= BuildBlockElement( *block, function_names, function_context );
	U_ASSERT( function_context.stack_variables_stack.size() == 1u );

	// If we build func code only for return type deducing - we can return. Function code will be generated later.
	if( func_variable.return_type_is_auto )
	{
		func_variable.return_type_is_auto= false;
		return
			function_context.deduced_return_type
				? *function_context.deduced_return_type
				: ( function_type.return_value_is_reference ? void_type_ : void_type_for_ret_ );
	}

	if( func_variable.constexpr_kind != FunctionVariable::ConstexprKind::NonConstexpr )
	{
		// Check function type and function body.
		// Function type checked here, because in case of constexpr methods not all types are complete yet.

		// For auto-constexpr functions we do not force type completeness. If function is really-constexpr, it must already make complete using types.

		const bool auto_contexpr= func_variable.constexpr_kind == FunctionVariable::ConstexprKind::ConstexprAuto;
		bool can_be_constexpr= true;

		if( !auto_contexpr )
		{
			if( function_type.return_type != void_type_for_ret_ && !EnsureTypeComplete( function_type.return_type ) )
				REPORT_ERROR( UsingIncompleteType, function_names.GetErrors(), func_variable.body_file_pos, function_type.return_type ); // Completeness required for constexpr possibility check.
		}

		if( function_type.unsafe ||
			!function_type.return_type.CanBeConstexpr() ||
			!function_type.references_pollution.empty() ) // Side effects, such pollution, not allowed.
			can_be_constexpr= false;

		if( function_type.return_type.GetFunctionPointerType() != nullptr ) // Currently function pointers not supported.
			can_be_constexpr= false;

		for( const Function::Arg& arg : function_type.args )
		{
			if( !auto_contexpr )
			{
				if( arg.type != void_type_ && !EnsureTypeComplete( arg.type ) )
					REPORT_ERROR( UsingIncompleteType, function_names.GetErrors(), func_variable.body_file_pos, arg.type ); // Completeness required for constexpr possibility check.
			}

			if( !arg.type.CanBeConstexpr() ) // Incomplete types are not constexpr.
				can_be_constexpr= false; // Allowed only constexpr types.
			if( arg.type == void_type_ ) // Disallow "void" arguments, because we currently can not constantly convert any reference to "void" in constexpr function call.
				can_be_constexpr= false;
			if( arg.type.GetFunctionPointerType() != nullptr ) // Currently function pointers not supported.
				can_be_constexpr= false;

			// We support constexpr functions with mutable reference-arguments, but such functions can not be used as root for constexpr function evaluation.
			// We support also constexpr constructors (except constexpr copy constructors), but constexpr constructors currently can not be used for constexpr variables initialization.
		}

		if( auto_contexpr )
		{
			if( can_be_constexpr && !function_context.have_non_constexpr_operations_inside )
				func_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprComplete;
			else
				func_variable.constexpr_kind= FunctionVariable::ConstexprKind::NonConstexpr;
		}
		else
		{
			if( !can_be_constexpr )
			{
				REPORT_ERROR( InvalidTypeForConstexprFunction, function_names.GetErrors(), func_variable.body_file_pos );
				func_variable.constexpr_kind= FunctionVariable::ConstexprKind::NonConstexpr;
			}
			else if( function_context.have_non_constexpr_operations_inside )
			{
				REPORT_ERROR( ConstexprFunctionContainsUnallowedOperations, function_names.GetErrors(), func_variable.body_file_pos );
				func_variable.constexpr_kind= FunctionVariable::ConstexprKind::NonConstexpr;
			}
			else
				func_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprComplete;
		}
	}

	// We need call destructors for arguments only if function returns "void".
	// In other case, we have "return" in all branches and destructors call before each "return".
	if( !block_build_info.have_terminal_instruction_inside )
	{
		if( function_type.return_type == void_type_ && !function_type.return_value_is_reference )
		{
			// Manually generate "return" for void-return functions.
			CallDestructors( args_storage, function_names, function_context, block->end_file_pos_ );
			CheckReferencesPollutionBeforeReturn( function_context, function_names.GetErrors(), block->end_file_pos_ );

			if( function_context.destructor_end_block == nullptr )
				function_context.llvm_ir_builder.CreateRetVoid();
			else
			{
				// In explicit destructor, break to block with destructor calls for class members.
				function_context.llvm_ir_builder.CreateBr( function_context.destructor_end_block );
			}
		}
		else
		{
			REPORT_ERROR( NoReturnInFunctionReturningNonVoid, function_names.GetErrors(), block->end_file_pos_ );
			return function_type.return_type;
		}
	}


	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );

	if( is_destructor )
	{
		// Fill destructors block.
		U_ASSERT( function_context.destructor_end_block != nullptr );
		function_context.llvm_ir_builder.SetInsertPoint( function_context.destructor_end_block );
		llvm_function->getBasicBlockList().push_back( function_context.destructor_end_block );

		CallMembersDestructors( function_context, function_names.GetErrors(), block->end_file_pos_ );		
		function_context.llvm_ir_builder.CreateRetVoid();
	}

	return function_type.return_type;
}

void CodeBuilder::BuildConstructorInitialization(
	const Variable& this_,
	const Class& base_class,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::StructNamedInitializer& constructor_initialization_list )
{
	ProgramStringSet initialized_fields;

	// Check for errors, build list of initialized fields.
	bool have_fields_errors= false;
	bool base_initialized= false;
	for( const Synt::StructNamedInitializer::MemberInitializer& field_initializer : constructor_initialization_list.members_initializers )
	{
		if( field_initializer.name == Keywords::base_ )
		{
			if( base_class.base_class == nullptr )
			{
				have_fields_errors= true;
				REPORT_ERROR( BaseUnavailable, names_scope.GetErrors(), constructor_initialization_list.file_pos_ );
				continue;
			}
			if( base_initialized )
			{
				have_fields_errors= true;
				REPORT_ERROR( DuplicatedStructMemberInitializer, names_scope.GetErrors(), constructor_initialization_list.file_pos_, field_initializer.name );
				continue;
			}
			base_initialized= true;
			function_context.base_initialized= false;
			continue;
		}

		const Value* const class_member= base_class.members.GetThisScopeValue( field_initializer.name );
		if( class_member == nullptr )
		{
			have_fields_errors= true;
			REPORT_ERROR( NameNotFound, names_scope.GetErrors(), constructor_initialization_list.file_pos_, field_initializer.name );
			continue;
		}

		const ClassField* const field= class_member->GetClassField();
		if( field == nullptr )
		{
			have_fields_errors= true;
			REPORT_ERROR( InitializerForNonfieldStructMember, names_scope.GetErrors(), constructor_initialization_list.file_pos_, field_initializer.name );
			continue;
		}
		if( field->class_.lock()->class_ != &base_class )
		{
			have_fields_errors= true;
			REPORT_ERROR( InitializerForBaseClassField, names_scope.GetErrors(), constructor_initialization_list.file_pos_, field_initializer.name );
			continue;
		}

		if( initialized_fields.find( field_initializer.name ) != initialized_fields.end() )
		{
			have_fields_errors= true;
			REPORT_ERROR( DuplicatedStructMemberInitializer, names_scope.GetErrors(), constructor_initialization_list.file_pos_, field_initializer.name );
			continue;
		}

		initialized_fields.insert( field_initializer.name );
		function_context.uninitialized_this_fields.insert( field->syntax_element->name );
	} // for fields initializers

	// Initialize fields, missing in initializer list.
	for( const std::string& field_name : base_class.fields_order )
	{
		if( field_name.empty() || initialized_fields.count(field_name) != 0 )
			continue;

		const ClassField& field= *base_class.members.GetThisScopeValue( field_name )->GetClassField();

		const StackVariablesStorage temp_variables_storage( function_context );

		if( field.is_reference )
		{
			if( field.syntax_element->initializer == nullptr )
			{
				REPORT_ERROR( ExpectedInitializer, names_scope.GetErrors(), constructor_initialization_list.file_pos_, field_name );
				continue;
			}
			InitializeReferenceClassFieldWithInClassIninitalizer( this_, field, function_context );
		}
		else
		{
			Variable field_variable;
			field_variable.type= field.type;
			field_variable.location= Variable::Location::Pointer;
			field_variable.value_type= ValueType::Reference;

			field_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( this_.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( field.index ) } );

			if( field.syntax_element->initializer != nullptr )
				InitializeClassFieldWithInClassIninitalizer( field_variable, field, function_context );
			else
				ApplyEmptyInitializer( field_name, constructor_initialization_list.file_pos_, field_variable, names_scope, function_context );
		}
		CallDestructors( temp_variables_storage, names_scope, function_context, constructor_initialization_list.file_pos_ );
	}
	if( !base_initialized && base_class.base_class != nullptr )
	{
		const StackVariablesStorage temp_variables_storage( function_context );

		// Apply default initializer for base class.
		Variable base_variable;
		base_variable.type= base_class.base_class;
		base_variable.location= Variable::Location::Pointer;
		base_variable.value_type= ValueType::Reference;

		base_variable.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( this_.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( 0u /* base class is allways first field */ ) } );

		ApplyEmptyInitializer( base_class.base_class->class_->members.GetThisNamespaceName(), constructor_initialization_list.file_pos_, base_variable, names_scope, function_context );
		function_context.base_initialized= true;

		CallDestructors( temp_variables_storage, names_scope, function_context, constructor_initialization_list.file_pos_ );
	}

	if( have_fields_errors )
		return;

	for( const Synt::StructNamedInitializer::MemberInitializer& field_initializer : constructor_initialization_list.members_initializers )
	{
		const StackVariablesStorage temp_variables_storage( function_context );

		if( field_initializer.name == Keywords::base_ )
		{
			Variable base_variable;
			base_variable.type= base_class.base_class;
			base_variable.location= Variable::Location::Pointer;
			base_variable.value_type= ValueType::Reference;
			base_variable.node= this_.node;

			base_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP(
					this_.llvm_value,
					{ GetZeroGEPIndex(), GetFieldGEPIndex( 0u /* base class is allways first field */ ) } );

			ApplyInitializer( field_initializer.initializer, base_variable, names_scope, function_context );
			function_context.base_initialized= true;
			continue;
		}

		const Value* const class_member=
			base_class.members.GetThisScopeValue( field_initializer.name );
		U_ASSERT( class_member != nullptr );
		const ClassField* const field= class_member->GetClassField();
		U_ASSERT( field != nullptr );

		if( field->is_reference )
			InitializeReferenceField( this_, *field, field_initializer.initializer, names_scope, function_context );
		else
		{
			Variable field_variable;
			field_variable.type= field->type;
			field_variable.location= Variable::Location::Pointer;
			field_variable.value_type= ValueType::Reference;
			field_variable.node= this_.node;

			field_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( this_.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex(field->index) } );

			ApplyInitializer( field_initializer.initializer, field_variable, names_scope, function_context );
		}

		function_context.uninitialized_this_fields.erase( field->syntax_element->name );

		CallDestructors( temp_variables_storage, names_scope, function_context, Synt::GetInitializerFilePos( field_initializer.initializer ) );
	} // for fields initializers

	SetupVirtualTablePointers( this_.llvm_value, base_class, function_context );
}

void CodeBuilder::BuildStaticAssert( StaticAssert& static_assert_, NamesScope& names, FunctionContext& function_context )
{
	if( static_assert_.syntax_element == nullptr )
		return;

	BuildBlockElement( *static_assert_.syntax_element, names, function_context );
	static_assert_.syntax_element= nullptr;
}

Value CodeBuilder::ResolveValue(
	const FilePos& file_pos,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::ComplexName& complex_name,
	const ResolveMode resolve_mode )
{
	Value* value= nullptr;
	Value temp_value_storage;
	const Synt::ComplexName::Component* component= complex_name.tail.get();
	NamesScope* last_space= &names_scope;

	if( std::get_if<Synt::EmptyVariant>(&complex_name.start_value) != nullptr )
	{
		const auto name= std::get_if<std::string>( &complex_name.tail->name_or_template_paramenters );
		U_ASSERT( complex_name.tail != nullptr && name != nullptr )
		last_space= names_scope.GetRoot();
		value= last_space->GetThisScopeValue( *name );
		component= complex_name.tail->next.get();

		if( value == nullptr )
			REPORT_ERROR( NameNotFound, names_scope.GetErrors(), file_pos, *name );

		if( ( *name == Keywords::constructor_ || *name == Keywords::destructor_ ) && !function_context.is_in_unsafe_block )
			REPORT_ERROR( ExplicitAccessToThisMethodIsUnsafe, names_scope.GetErrors(), file_pos, *name );
	}
	else if( const auto typeof_type_name= std::get_if<Synt::TypeofTypeName>(&complex_name.start_value) )
	{
		temp_value_storage= Value( PrepareType( *typeof_type_name, names_scope, function_context ), file_pos );
		value= &temp_value_storage;
	}
	else if(const auto simple_name= std::get_if<std::string>(&complex_name.start_value) )
	{
		do
		{
			value= last_space->GetThisScopeValue( *simple_name );
			if( value != nullptr )
				break;
			last_space= last_space->GetParent();
		} while( last_space != nullptr );

		if( value == nullptr )
			REPORT_ERROR( NameNotFound, names_scope.GetErrors(), file_pos, *simple_name );
	}
	else U_ASSERT(false);

	if( value != nullptr && value->GetYetNotDeducedTemplateArg() != nullptr )
		REPORT_ERROR( TemplateArgumentIsNotDeducedYet, names_scope.GetErrors(), file_pos, complex_name );

	while( component != nullptr && value != nullptr )
	{
		if( resolve_mode == ResolveMode::ForTemplateSignatureParameter && component->next == nullptr &&
			std::get_if< std::vector<Synt::Expression> >( &component->name_or_template_paramenters ) != nullptr )
			break;

		// In case of typedef convert it to type before other checks.
		if( value->GetTypedef() != nullptr )
			GlobalThingBuildTypedef( *last_space, *value );

		if( const auto component_name= std::get_if<std::string>( &component->name_or_template_paramenters ) )
		{
			if( const NamesScopePtr inner_namespace= value->GetNamespace() )
			{
				value= inner_namespace->GetThisScopeValue( *component_name );
				last_space= inner_namespace.get();
			}
			else if( const Type* const type= value->GetTypeName() )
			{
				if( Class* const class_= type->GetClassType() )
				{
					if( class_->syntax_element != nullptr && class_->syntax_element->is_forward_declaration_ )
					{
						REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), file_pos, *type );
						return ErrorValue();
					}

					GlobalThingBuildClass( type->GetClassTypeProxy() );

					if( names_scope.GetAccessFor( type->GetClassTypeProxy() ) < class_->GetMemberVisibility( *component_name ) )
						REPORT_ERROR( AccessingNonpublicClassMember, names_scope.GetErrors(), file_pos, *component_name, class_->members.GetThisNamespaceName() );

					if( ( *component_name == Keywords::constructor_ || *component_name == Keywords::destructor_ ) && !function_context.is_in_unsafe_block )
						REPORT_ERROR( ExplicitAccessToThisMethodIsUnsafe, names_scope.GetErrors(), file_pos, *component_name );

					value= class_->members.GetThisScopeValue( *component_name );
					last_space= & class_->members;
				}
				else if( EnumPtr const enum_= type->GetEnumType() )
				{
					GlobalThingBuildEnum( enum_ );
					value= enum_->members.GetThisScopeValue( *component_name );
					last_space= &enum_->members;
				}
			}
			else if( value->GetTypeTemplatesSet() != nullptr )
			{
				REPORT_ERROR( TemplateInstantiationRequired, names_scope.GetErrors(), file_pos, *component_name );
				return ErrorValue();
			}

			if( value == nullptr )
				REPORT_ERROR( NameNotFound, names_scope.GetErrors(), file_pos, *component_name );
		}
		else if( const auto template_parameters= std::get_if< std::vector<Synt::Expression> >( &component->name_or_template_paramenters ) )
		{
			if( TypeTemplatesSet* const type_templates_set= value->GetTypeTemplatesSet() )
			{
				GlobalThingBuildTypeTemplatesSet( *last_space, *type_templates_set );
				value=
					GenTemplateType(
						file_pos,
						*type_templates_set,
						*template_parameters,
						names_scope,
						function_context );
				if( value == nullptr )
					return ErrorValue();

				const Type* const type= value->GetTypeName();
				U_ASSERT( type != nullptr );
				if( Class* const class_= type->GetClassType() )
					last_space= &class_->members;
			}
			else if( OverloadedFunctionsSet* const functions_set= value->GetFunctionsSet() )
			{
				GlobalThingBuildFunctionsSet( *last_space, *functions_set, false );
				if( functions_set->template_functions.empty() )
				{
					REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), file_pos );
					return ErrorValue();
				}

				value=
					GenTemplateFunctionsUsingTemplateParameters(
						file_pos,
						functions_set->template_functions,
						*template_parameters,
						names_scope,
						function_context );
				if( value == nullptr )
				{
					REPORT_ERROR( TemplateFunctionGenerationFailed, names_scope.GetErrors(), file_pos, complex_name );
					return ErrorValue();
				}
			}
			else
			{
				REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), file_pos );
				return ErrorValue();
			}
		}

		component= component->next.get();
	}

	// Complete some things in resolve.
	if( value != nullptr )
	{
		if( OverloadedFunctionsSet* const functions_set= value->GetFunctionsSet() )
			GlobalThingBuildFunctionsSet( *last_space, *functions_set, false );
		else if( TypeTemplatesSet* const type_templates_set= value->GetTypeTemplatesSet() )
			GlobalThingBuildTypeTemplatesSet( *last_space, *type_templates_set );
		else if( value->GetTypedef() != nullptr )
			GlobalThingBuildTypedef( *last_space, *value );
		else if( value->GetIncompleteGlobalVariable() != nullptr )
			GlobalThingBuildVariable( *last_space, *value );
		else if( const Type* const type= value->GetTypeName() )
		{
			if( const EnumPtr enum_= type->GetEnumTypePtr() )
				GlobalThingBuildEnum( enum_ );
		}
	}

	return value == nullptr ? ErrorValue() : *value;
}

llvm::Type* CodeBuilder::GetFundamentalLLVMType( const U_FundamentalType fundmantal_type )
{
	switch( fundmantal_type )
	{
	case U_FundamentalType::InvalidType:
		return fundamental_llvm_types_.invalid_type;
	case U_FundamentalType::LastType:
		break;

	case U_FundamentalType::Void:
		return fundamental_llvm_types_.void_;
	case U_FundamentalType::Bool:
		return fundamental_llvm_types_.bool_;
	case U_FundamentalType::i8 :
		return fundamental_llvm_types_.i8 ;
	case U_FundamentalType::u8 :
		return fundamental_llvm_types_.u8 ;
	case U_FundamentalType::i16:
		return fundamental_llvm_types_.i16;
	case U_FundamentalType::u16:
		return fundamental_llvm_types_.u16;
	case U_FundamentalType::i32:
		return fundamental_llvm_types_.i32;
	case U_FundamentalType::u32:
		return fundamental_llvm_types_.u32;
	case U_FundamentalType::i64:
		return fundamental_llvm_types_.i64;
	case U_FundamentalType::u64:
		return fundamental_llvm_types_.u64;
	case U_FundamentalType::i128:
		return fundamental_llvm_types_.i128;
	case U_FundamentalType::u128:
		return fundamental_llvm_types_.u128;
	case U_FundamentalType::f32:
		return fundamental_llvm_types_.f32;
	case U_FundamentalType::f64:
		return fundamental_llvm_types_.f64;
	case U_FundamentalType::char8 :
		return fundamental_llvm_types_.char8 ;
	case U_FundamentalType::char16:
		return fundamental_llvm_types_.char16;
	case U_FundamentalType::char32:
		return fundamental_llvm_types_.char32;
	};

	U_ASSERT(false);
	return nullptr;
}

llvm::Value*CodeBuilder::CreateMoveToLLVMRegisterInstruction(
	const Variable& variable, FunctionContext& function_context )
{
	// Contant values always are register-values.
	if( variable.constexpr_value != nullptr )
		return variable.constexpr_value;

	switch( variable.location )
	{
	case Variable::Location::LLVMRegister:
		return variable.llvm_value;
	case Variable::Location::Pointer:
		return function_context.llvm_ir_builder.CreateLoad( variable.llvm_value );
	};

	U_ASSERT(false);
	return nullptr;
}

llvm::Constant* CodeBuilder::GetZeroGEPIndex()
{
	return llvm::Constant::getNullValue( fundamental_llvm_types_.i32 );
}

llvm::Constant* CodeBuilder::GetFieldGEPIndex( const uint64_t field_index )
{
	return llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, field_index ) );
}

llvm::Value* CodeBuilder::CreateReferenceCast( llvm::Value* const ref, const Type& src_type, const Type& dst_type, FunctionContext& function_context )
{
	U_ASSERT( src_type.ReferenceIsConvertibleTo( dst_type ) );

	if( src_type == dst_type )
		return ref;

	if( dst_type == void_type_ )
		return function_context.llvm_ir_builder.CreatePointerCast( ref, dst_type.GetLLVMType()->getPointerTo() );
	else
	{
		const Class* const src_class_type= src_type.GetClassType();
		U_ASSERT( src_class_type != nullptr );

		for( const Class::Parent& src_parent_class : src_class_type->parents )
		{
			llvm::Value* const sub_ref=
				function_context.llvm_ir_builder.CreateGEP(
					ref,
					{ GetZeroGEPIndex(), GetFieldGEPIndex( src_parent_class.field_number ) } );

			if( src_parent_class.class_ == dst_type )
				return sub_ref;
			else if( Type(src_parent_class.class_).ReferenceIsConvertibleTo( dst_type ) )
				return CreateReferenceCast( sub_ref, src_parent_class.class_, dst_type, function_context );
		}
	}

	U_ASSERT(false);
	return nullptr;
}

llvm::GlobalVariable* CodeBuilder::CreateGlobalConstantVariable(
	const Type& type,
	const std::string& mangled_name,
	llvm::Constant* const initializer )
{
	// Try to reuse global variable.
	if( llvm::GlobalVariable* const prev_literal_name= module_->getNamedGlobal(mangled_name) )
		if( prev_literal_name->getInitializer() == initializer ) // llvm reuses constants, so, for equal constants pointers will be same.
			return prev_literal_name;
	
	llvm::GlobalVariable* const val=
		new llvm::GlobalVariable(
			*module_,
			type.GetLLVMType(),
			true, // is constant
			llvm::GlobalValue::PrivateLinkage, // We have no external variables, so, use private linkage.
			initializer,
			mangled_name );

	val->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );

	return val;
}

void CodeBuilder::SetupGeneratedFunctionAttributes( llvm::Function& function )
{
	// Merge functions with identical code.
	// We doesn`t need different addresses for different functions.
	function.setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );

	function.setLinkage( llvm::GlobalValue::PrivateLinkage );

	function.setDoesNotThrow(); // We do not support exceptions.

	if( build_debug_info_ ) // Unwind table entry for function needed for debug info.
		function.addFnAttr( llvm::Attribute::UWTable );
}

CodeBuilder::InstructionsState CodeBuilder::SaveInstructionsState( FunctionContext& function_context )
{
	InstructionsState result;
	result.variables_state= function_context.variables_state;
	result.current_block_instruction_count= function_context.llvm_ir_builder.GetInsertBlock()->size();
	result.alloca_block_instructin_count= function_context.alloca_ir_builder.GetInsertBlock()->size();
	result.block_count= function_context.function->getBasicBlockList().size();
	return result;
}

void CodeBuilder::RestoreInstructionsState(
	FunctionContext& function_context,
	const InstructionsState& state )
{
	function_context.variables_state= state.variables_state;

	// Remove instructions of some operations, that must be discarded.

	auto& bb_list= function_context.function->getBasicBlockList();
	while( bb_list.size() > state.block_count )
	{
		for( const auto use : bb_list.back().users() )
			use->dropAllReferences();
		bb_list.pop_back();
	}

	auto& inst_list= bb_list.back().getInstList();
	while( inst_list.size() > state.current_block_instruction_count )
		inst_list.pop_back();
	function_context.llvm_ir_builder.SetInsertPoint( &bb_list.back(), bb_list.back().end() );

	while( function_context.alloca_basic_block->getInstList().size() > state.alloca_block_instructin_count )
		function_context.alloca_basic_block->getInstList().pop_back();
	function_context.alloca_ir_builder.SetInsertPoint( function_context.alloca_basic_block, function_context.alloca_basic_block->end() );
}

} // namespace CodeBuilderPrivate

} // namespace U
