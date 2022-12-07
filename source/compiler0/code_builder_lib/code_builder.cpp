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

#include "code_builder.hpp"

namespace U
{

namespace
{

std::unique_ptr<IMangler> CreateMangler(const ManglingScheme scheme, const llvm::DataLayout& data_layout)
{
	switch(scheme)
	{
	case ManglingScheme::ItaniumABI: return CreateManglerItaniumABI();
	case ManglingScheme::MSVC: return CreateManglerMSVC(data_layout.getPointerSize() == 4);
	case ManglingScheme::MSVC32: return CreateManglerMSVC(true);
	case ManglingScheme::MSVC64: return CreateManglerMSVC(false);
	};

	U_ASSERT(false);
	return CreateManglerItaniumABI();
}

} // namespace

CodeBuilder::ReferencesGraphNodeHolder::ReferencesGraphNodeHolder(
	FunctionContext& function_context,
	const ReferencesGraphNode::Kind node_kind,
	std::string node_name )
	: node_( function_context.variables_state.AddNode( node_kind, std::move(node_name) ) )
	, function_context_(function_context)
{
}

CodeBuilder::ReferencesGraphNodeHolder::ReferencesGraphNodeHolder( ReferencesGraphNodeHolder&& other) noexcept
	: node_(other.node_), function_context_(other.function_context_)
{
	other.node_= nullptr;
}

CodeBuilder::ReferencesGraphNodeHolder& CodeBuilder::ReferencesGraphNodeHolder::operator=( ReferencesGraphNodeHolder&& other ) noexcept
{
	if( this->node_ != nullptr )
		function_context_.variables_state.RemoveNode( node_ );

	this->node_= other.node_;
	other.node_= nullptr;
	return *this;
}

CodeBuilder::ReferencesGraphNodeHolder::~ReferencesGraphNodeHolder()
{
	if( node_ != nullptr )
		function_context_.variables_state.RemoveNode( node_ );
}

ReferencesGraphNodePtr CodeBuilder::ReferencesGraphNodeHolder::TakeNode()
{
	auto res= node_;
	node_= nullptr;
	return res;
}

CodeBuilder::CodeBuilder(
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	const CodeBuilderOptions& options )
	: llvm_context_( llvm_context )
	, data_layout_(data_layout)
	, target_triple_(target_triple)
	, build_debug_info_( options.build_debug_info )
	, create_lifetimes_( options.create_lifetimes )
	, generate_lifetime_start_end_debug_calls_( options.generate_lifetime_start_end_debug_calls )
	, constexpr_function_evaluator_( data_layout_ )
	, mangler_( CreateMangler( options.mangling_scheme, data_layout_ ) )
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
	fundamental_llvm_types_.void_for_ret= llvm::Type::getVoidTy( llvm_context_ );
	fundamental_llvm_types_.bool_= llvm::Type::getInt1Ty( llvm_context_ );

	// Use empty named structure for "void" type.
	fundamental_llvm_types_.void_= llvm::StructType::create( llvm_context_, {}, "__U_void" );

	fundamental_llvm_types_.int_ptr= data_layout_.getIntPtrType(llvm_context_);

	invalid_type_= FundamentalType( U_FundamentalType::InvalidType, fundamental_llvm_types_.invalid_type );
	void_type_= FundamentalType( U_FundamentalType::Void, fundamental_llvm_types_.void_ );
	bool_type_= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );
	size_type_=
		fundamental_llvm_types_.int_ptr->getIntegerBitWidth() == 32u
		? FundamentalType( U_FundamentalType::u32, fundamental_llvm_types_.u32 )
		: FundamentalType( U_FundamentalType::u64, fundamental_llvm_types_.u64 );

	// Use named struct for polymorph type id table element, because this is recursive struct.
	{
		polymorph_type_id_table_element_type_= llvm::StructType::create( llvm_context_, "__U_polymorph_type_id_table_element" );
		llvm::Type* const elements[]
		{
			fundamental_llvm_types_.int_ptr, // Parent class offset.
			polymorph_type_id_table_element_type_->getPointerTo(), // Pointer to parent class type_id table.
		};
		polymorph_type_id_table_element_type_->setBody( elements );
	}
}

CodeBuilder::BuildResult CodeBuilder::BuildProgram( const SourceGraph& source_graph )
{
	global_errors_.clear();

	module_=
		std::make_unique<llvm::Module>(
			source_graph.nodes_storage.front().file_path,
			llvm_context_ );

	// Setup data layout and target triple.
	module_->setDataLayout(data_layout_);
	module_->setTargetTriple(target_triple_.normalize());

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

	// Prepare debug lifetime_start/lifetime_end functions.
	if( create_lifetimes_ && generate_lifetime_start_end_debug_calls_ )
	{
		llvm::Type* const arg_types[1]= { fundamental_llvm_types_.u8->getPointerTo() };

		lifetime_start_debug_func_=
			llvm::Function::Create(
				llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret, arg_types, false ),
				llvm::Function::ExternalLinkage,
				"__U_debug_lifetime_start",
				module_.get() );

		lifetime_end_debug_func_=
			llvm::Function::Create(
				llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret, arg_types, false ),
				llvm::Function::ExternalLinkage,
				"__U_debug_lifetime_end",
				module_.get() );
	}

	FunctionType global_function_type;
	global_function_type.return_type= void_type_;
	global_function_type.llvm_type= GetLLVMFunctionType( global_function_type );

	// In some places outside functions we need to execute expression evaluation.
	// Create for this function context.
	llvm::Function* const global_function=
		llvm::Function::Create(
			global_function_type.llvm_type,
			llvm::Function::LinkageTypes::ExternalLinkage,
			"",
			module_.get() );

	FunctionContext global_function_context(
		std::move(global_function_type),
		void_type_,
		llvm_context_,
		global_function );
	const StackVariablesStorage global_function_variables_storage( global_function_context );
	global_function_context_= &global_function_context;

	if( build_debug_info_ )
	{
		for( const auto& node : source_graph.nodes_storage )
			debug_info_.source_file_entries.push_back( llvm::DIFile::get( llvm_context_, node.file_path, "" ) );

		// HACK! Add a workaround for wrong assert in llvm code in Dwarf.h:235. TODO - remove this after porting to LLVM 15 or newer.
		// const uint32_t c_dwarf_language_id= llvm::dwarf::DW_LANG_lo_user + 0xDC /* code of "Ü" letter */;
		const uint32_t c_dwarf_language_id= llvm::dwarf::DW_LANG_C;

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
	compiled_sources_.resize( source_graph.nodes_storage.size() );
	BuildSourceGraphNode( source_graph, 0u );

	// Finalize "defererenceable" attributes.
	// Do this at end because we needs complete types for params/return values even for only prototypes.
	SetupDereferenceableFunctionParamsAndRetAttributes_r( *compiled_sources_.front().names_map );
	for( auto& name_value_pair : generated_template_things_storage_ )
		if( const auto names_scope= name_value_pair.second.GetNamespace() )
			SetupDereferenceableFunctionParamsAndRetAttributes_r( *names_scope );

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
	compiled_sources_.clear();
	enums_table_.clear();
	template_classes_cache_.clear();
	typeinfo_cache_.clear();
	typeinfo_class_table_.clear();
	generated_template_things_storage_.clear();
	generated_template_things_sequence_.clear();
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

void CodeBuilder::BuildSourceGraphNode( const SourceGraph& source_graph, const size_t node_index )
{
	SourceBuildResult& result= compiled_sources_[ node_index ];
	if( result.names_map != nullptr )
		return;

	result.names_map= std::make_unique<NamesScope>( "", nullptr );
	result.names_map->SetErrors( global_errors_ );
	FillGlobalNamesScope( *result.names_map );

	U_ASSERT( node_index < source_graph.nodes_storage.size() );
	const SourceGraph::Node& source_graph_node= source_graph.nodes_storage[ node_index ];

	// Build dependent nodes.
	for( const size_t child_node_inex : source_graph_node.child_nodes_indeces )
		BuildSourceGraphNode( source_graph, child_node_inex );

	// Merge namespaces of imported files into one.
	for( const size_t child_node_inex : source_graph_node.child_nodes_indeces )
	{
		const SourceBuildResult& child_node_build_result= compiled_sources_[ child_node_inex ];
		MergeNameScopes( *result.names_map, *child_node_build_result.names_map, child_node_build_result.classes_members_namespaces_table );
	}

	// Do work for this node.
	NamesScopeFill( source_graph_node.ast.program_elements, *result.names_map );
	NamesScopeFillOutOfLineElements( source_graph_node.ast.program_elements, *result.names_map );
	GlobalThingBuildNamespace( *result.names_map );

	// Finalize building template things.
	// Each new template thing added into this vector, so, by iterating through we will build all template things.
	// It's important to use an index instead of iterators during iteration because this vector may be chaged in process.
	for( size_t i= 0; i < generated_template_things_sequence_.size(); ++i )
	{
		if( const auto namespace_= generated_template_things_storage_[generated_template_things_sequence_[i]].GetNamespace() )
			GlobalThingBuildNamespace( *namespace_ );
	}
	generated_template_things_sequence_.clear();

	// Fill result classes members namespaces table.
	for( const auto& class_shared_ptr : classes_table_ )
		result.classes_members_namespaces_table.emplace( class_shared_ptr.get(), class_shared_ptr->members );
}

void CodeBuilder::MergeNameScopes(
	NamesScope& dst,
	const NamesScope& src,
	const ClassesMembersNamespacesTable& src_classes_members_namespaces_table )
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
					MergeNameScopes( *names_scope_copy, *names_scope, src_classes_members_namespaces_table );
					dst.AddName( src_name, Value( names_scope_copy, src_member.GetSrcLoc() ) );

					names_scope_copy->CopyAccessRightsFrom( *names_scope );
				}
				else
				{
					if( const Type* const type= src_member.GetTypeName() )
					{
						if( const ClassPtr class_= type->GetClassType() )
						{
							// Just take copy of internal namespace.
							// Use namespace from table in order to properly merge changes made in several source files.
							const auto it= src_classes_members_namespaces_table.find(class_);
							U_ASSERT(it != src_classes_members_namespaces_table.end());
							const auto& src_members_namespace= it->second;

							// If current namespace is parent for this class and name is primary.
							if( src_members_namespace->GetParent() == &src &&
								src_members_namespace->GetThisNamespaceName() == src_name )
							{
								const auto class_namespace_copy= std::make_shared<NamesScope>( src_members_namespace->GetThisNamespaceName(), &dst );
								MergeNameScopes( *class_namespace_copy, *src_members_namespace, src_classes_members_namespaces_table );

								class_namespace_copy->SetClass( class_ );
								class_namespace_copy->CopyAccessRightsFrom( *src_members_namespace );
								// It's fine to modify "members", since we preserve actual namespaces in the table.
								class_->members= class_namespace_copy;
							}
						}
					}
					dst.AddName( src_name, src_member );
				}
				return;
			}

			if( dst_member->GetKindIndex() != src_member.GetKindIndex() )
			{
				// Different kind of symbols - 100% error.
				REPORT_ERROR( Redefinition, dst.GetErrors(), src_member.GetSrcLoc(), src_name );
				return;
			}

			if( const NamesScopePtr sub_namespace= src_member.GetNamespace() )
			{
				// Merge namespaces.
				// TODO - detect here template instantiation namespaces.
				const NamesScopePtr dst_sub_namespace= dst_member->GetNamespace();
				U_ASSERT( dst_sub_namespace != nullptr );
				MergeNameScopes( *dst_sub_namespace, *sub_namespace, src_classes_members_namespaces_table );
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
						if( same_dst_func->prototype_src_loc != src_func.prototype_src_loc )
						{
							// Prototypes are in differrent files.
							REPORT_ERROR( FunctionPrototypeDuplication, dst.GetErrors(), src_func.prototype_src_loc, src_name );
							continue;
						}

						if( !same_dst_func->have_body &&  src_func.have_body )
							*same_dst_func= src_func; // Take this function - it have body.
						if(  same_dst_func->have_body && !src_func.have_body )
						{} // Ok, prototype imported later.
						if(  same_dst_func->have_body &&  src_func.have_body &&
							same_dst_func->body_src_loc != src_func.body_src_loc )
							REPORT_ERROR( FunctionBodyDuplication, dst.GetErrors(), src_func.body_src_loc, src_name );
					}
					else
						ApplyOverloadedFunction( *dst_funcs_set, src_func, dst.GetErrors(), src_func.prototype_src_loc );
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
				const auto dst_class= type->GetClassType();
				const auto src_class= src_member.GetTypeName()->GetClassType();
				if( dst_class != src_class )
				{
					// Different pointer value means 100% different classes.
					REPORT_ERROR( Redefinition, dst.GetErrors(), src_member.GetSrcLoc(), src_name );
					return;
				}
				if( dst_class != nullptr && src_class != nullptr )
				{
					// Merge changes made in internal namespace of the class.
					// This affects almost only functions sets.
					const auto it= src_classes_members_namespaces_table.find(src_class);
					U_ASSERT(it != src_classes_members_namespaces_table.end());
					const auto& src_members_namespace= it->second;

					// If current namespace is parent for this class and name is primary.
					if( src_members_namespace->GetParent() == &src &&
						src_members_namespace->GetThisNamespaceName() == src_name )
						MergeNameScopes( *dst_class->members, *src_members_namespace, src_classes_members_namespaces_table );
				}
			}
			else if( const auto dst_type_templates_set= dst_member->GetTypeTemplatesSet() )
			{
				if( const auto src_type_templates_set= src_member.GetTypeTemplatesSet() )
				{
					for( const TypeTemplatePtr& src_type_template : src_type_templates_set->type_templates )
					{
						bool should_add= true;
						for( const TypeTemplatePtr& dst_type_template : dst_type_templates_set->type_templates )
						{
							if( dst_type_template == src_type_template )
							{
								should_add= false;
								break;
							}
							if( src_type_template->signature_params == dst_type_template->signature_params )
							{
								REPORT_ERROR( TypeTemplateRedefinition, dst.GetErrors(), src_type_template->src_loc, src_name );
								should_add= false;
								break;
							}
						}
						if( should_add )
							dst_type_templates_set->type_templates.push_back( src_type_template );
					}

					return;
				}
			}

			if( dst_member->GetSrcLoc() == src_member.GetSrcLoc() )
				return; // All ok - things from one source.

			// Can not merge other kinds of values.
			REPORT_ERROR( Redefinition, dst.GetErrors(), src_member.GetSrcLoc(), src_name );
		} );
}

void CodeBuilder::FillGlobalNamesScope( NamesScope& global_names_scope )
{
	const SrcLoc fundamental_globals_src_loc( SrcLoc::c_max_file_index, SrcLoc::c_max_line, SrcLoc::c_max_column );

	for( size_t i= size_t(U_FundamentalType::Void); i < size_t(U_FundamentalType::LastType); ++i )
	{
		const U_FundamentalType fundamental_type= U_FundamentalType(i);
		global_names_scope.AddName(
			GetFundamentalTypeName(fundamental_type),
			Value(
				FundamentalType( fundamental_type, GetFundamentalLLVMType( fundamental_type ) ),
				fundamental_globals_src_loc ) );
	}

	global_names_scope.AddName( Keyword( Keywords::size_type_ ), Value( size_type_, fundamental_globals_src_loc ) );
}

void CodeBuilder::TryCallCopyConstructor(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	llvm::Value* const this_, llvm::Value* const src,
	const ClassPtr& class_type,
	FunctionContext& function_context )
{
	U_ASSERT( class_type != nullptr );
	const Class& class_= *class_type;

	if( !class_.is_copy_constructible )
	{
		// TODO - print more reliable message.
		REPORT_ERROR( OperationNotSupportedForThisType, errors_container, src_loc, class_type );
		return;
	}

	// Search for copy-constructor.
	const Value* const constructos_value= class_.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) );
	U_ASSERT( constructos_value != nullptr );
	const OverloadedFunctionsSet* const constructors= constructos_value->GetFunctionsSet();
	U_ASSERT(constructors != nullptr );
	const FunctionVariable* constructor= nullptr;
	for( const FunctionVariable& candidate : constructors->functions )
	{
		if( IsCopyConstructor( *candidate.type.GetFunctionType(), class_type ) )
		{
			constructor= &candidate;
			break;
		}
	}

	if( !( constructor->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete || constructor->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete ) )
		function_context.have_non_constexpr_operations_inside= true;

	// Call it
	U_ASSERT(constructor != nullptr);
	function_context.llvm_ir_builder.CreateCall( constructor->llvm_function, { this_, src } );
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
	llvm::Value* const counter_value_plus_one= function_context.llvm_ir_builder.CreateAdd( counter_value, one_value );

	loop_body( counter_value );

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
	const SrcLoc& src_loc )
{
	// Call destructors in reverse order.
	for( auto it = stack_variables_storage.variables_.rbegin(); it != stack_variables_storage.variables_.rend(); ++it )
	{
		const Variable& stored_variable= *it;

		if( stored_variable.node->kind == ReferencesGraphNode::Kind::Variable )
		{
			if( !function_context.variables_state.NodeMoved( stored_variable.node ) )
			{
				if( function_context.variables_state.HaveOutgoingLinks( stored_variable.node ) )
					REPORT_ERROR( DestroyedVariableStillHaveReferences, errors_container, src_loc, stored_variable.node->name );
				if( stored_variable.type.HaveDestructor() )
					CallDestructor( stored_variable.llvm_value, stored_variable.type, function_context, errors_container, src_loc );

				// Avoid calling "lifetime.end" for variables without address.
				if( stored_variable.location == Variable::Location::Pointer )
					CreateLifetimeEnd( function_context, stored_variable.llvm_value );
			}
		}
		function_context.variables_state.RemoveNode( stored_variable.node );
	}
}

void CodeBuilder::CallDestructors(
	const StackVariablesStorage& stack_variables_storage,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const SrcLoc& src_loc )
{
	CallDestructorsImpl( stack_variables_storage, function_context, names_scope.GetErrors(), src_loc );
}

void CodeBuilder::CallDestructor(
	llvm::Value* const ptr,
	const Type& type,
	FunctionContext& function_context,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	U_ASSERT( type.HaveDestructor() );

	if( const Class* const class_= type.GetClassType() )
	{
		const Value* const destructor_value= class_->members->GetThisScopeValue( Keyword( Keywords::destructor_ ) );
		U_ASSERT( destructor_value != nullptr );
		const OverloadedFunctionsSet* const destructors= destructor_value->GetFunctionsSet();
		U_ASSERT(destructors != nullptr && destructors->functions.size() == 1u );

		const FunctionVariable& destructor= destructors->functions.front();
		function_context.llvm_ir_builder.CreateCall( destructor.llvm_function, { ptr } );
	}
	else if( const ArrayType* const array_type= type.GetArrayType() )
	{
		// SPRACHE_TODO - maybe call destructors of arrays in reverse order?
		GenerateLoop(
			array_type->size,
			[&]( llvm::Value* const index )
			{
				CallDestructor(
					CreateArrayElementGEP( function_context, ptr, index ),
					array_type->type,
					function_context,
					errors_container,
					src_loc );
			},
			function_context );
	}
	else if( const TupleType* const tuple_type= type.GetTupleType() )
	{
		for( const Type& element_type : tuple_type->elements )
		{
			if( element_type.HaveDestructor() )
				CallDestructor(
					CreateTupleElementGEP( function_context, ptr, size_t(&element_type - tuple_type->elements.data()) ),
					element_type,
					function_context,
					errors_container,
					src_loc );
		}
	}
	else U_ASSERT(false);
}

void CodeBuilder::CallDestructorsForLoopInnerVariables( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc )
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
		CallDestructorsImpl( **it, function_context, names_scope.GetErrors(), src_loc );
	}
}

void CodeBuilder::CallDestructorsBeforeReturn( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc )
{
	// We must call ALL destructors of local variables, arguments, etc before each return.
	for( auto it= function_context.stack_variables_stack.rbegin(); it != function_context.stack_variables_stack.rend(); ++it )
		CallDestructorsImpl( **it, function_context, names_scope.GetErrors(), src_loc );
}

void CodeBuilder::CallMembersDestructors( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	U_ASSERT( function_context.this_ != nullptr );
	const Class* const class_= function_context.this_->type.GetClassType();
	U_ASSERT( class_ != nullptr );

	for( size_t i= 0u; i < class_->parents.size(); ++i )
	{
		U_ASSERT( class_->parents[i].class_->have_destructor ); // Parents are polymorph, polymorph classes always have destructors.
		CallDestructor(
			CreateClassFiledGEP( function_context, function_context.this_->llvm_value, class_->parents[i].field_number ),
			class_->parents[i].class_,
			function_context,
			errors_container,
			src_loc );
	}

	for( const std::string& field_name : class_->fields_order )
	{
		if( field_name.empty() )
			continue;

		const ClassField& field= *class_->members->GetThisScopeValue( field_name )->GetClassField();
		if( !field.type.HaveDestructor() || field.is_reference )
			continue;

		CallDestructor(
			CreateClassFiledGEP( function_context, function_context.this_->llvm_value, field.index ),
			field.type,
			function_context,
			errors_container,
			src_loc );
	};
}

size_t CodeBuilder::PrepareFunction(
	NamesScope& names_scope,
	const ClassPtr& base_class,
	OverloadedFunctionsSet& functions_set,
	const Synt::Function& func,
	const bool is_out_of_line_function )
{
	const std::string& func_name= func.name_.back();
	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	const bool is_special_method= is_constructor || is_destructor;

	if( is_destructor || is_constructor )
		U_ASSERT( func.type_.params_.size() >= 1u && func.type_.params_.front().name_ == Keywords::this_ );

	if( !is_special_method && IsKeyword( func_name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), func.src_loc_ );

	if( is_special_method && base_class == nullptr )
	{
		REPORT_ERROR( ConstructorOrDestructorOutsideClass, names_scope.GetErrors(), func.src_loc_ );
		return ~0u;
	}
	if( !is_constructor && func.constructor_initialization_list_ != nullptr )
	{
		REPORT_ERROR( InitializationListInNonconstructor, names_scope.GetErrors(), func.constructor_initialization_list_->src_loc_ );
		return ~0u;
	}
	if( is_destructor && func.type_.params_.size() >= 2u )
	{
		REPORT_ERROR( ExplicitArgumentsInDestructor, names_scope.GetErrors(), func.src_loc_ );
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
				REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), Synt::GetExpressionSrcLoc( func.condition_ ) );
		}
		else
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), Synt::GetExpressionSrcLoc( func.condition_ ), bool_type_, expression.type );
	}

	FunctionVariable func_variable;
	{ // Prepare function type
		FunctionType function_type;

		if( func.type_.return_type_ == nullptr )
			function_type.return_type= void_type_;
		else
		{
			if( const auto named_return_type = std::get_if<Synt::ComplexName>(func.type_.return_type_.get()) )
			{
				// TODO - set flag "auto" in syntax analyzer.
				if( named_return_type->tail == nullptr &&
					std::get_if<std::string>( &named_return_type->start_value ) != nullptr &&
					std::get<std::string>( named_return_type->start_value ) == Keywords::auto_ )
				{
					func_variable.return_type_is_auto= true;
					if( base_class != nullptr )
						REPORT_ERROR( AutoFunctionInsideClassesNotAllowed, names_scope.GetErrors(), func.src_loc_, func_name );
					if( func.block_ == nullptr )
						REPORT_ERROR( ExpectedBodyForAutoFunction, names_scope.GetErrors(), func.src_loc_, func_name );

					function_type.return_type= void_type_;
				}
			}

			if( !func_variable.return_type_is_auto )
			{
				function_type.return_type= PrepareType( *func.type_.return_type_, names_scope, *global_function_context_ );
				if( function_type.return_type == invalid_type_ )
					return ~0u;
			}
		}

		if( func.type_.return_value_reference_modifier_ == ReferenceModifier::None )
			function_type.return_value_type= ValueType::Value;
		else
			function_type.return_value_type= func.type_.return_value_mutability_modifier_ == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;

		if( is_special_method && !( function_type.return_type == void_type_ && function_type.return_value_type == ValueType::Value ) )
			REPORT_ERROR( ConstructorAndDestructorMustReturnVoid, names_scope.GetErrors(), func.src_loc_ );

		ProcessFunctionReturnValueReferenceTags( names_scope.GetErrors(), func.type_, function_type );

		// Params.
		function_type.params.reserve( func.type_.params_.size() );

		for( const Synt::FunctionParam& in_param : func.type_.params_ )
		{
			const bool is_this=
				&in_param == &func.type_.params_.front() &&
				in_param.name_ == Keywords::this_ &&
				std::get_if<Synt::EmptyVariant>(&in_param.type_) != nullptr;

			if( !is_this && IsKeyword( in_param.name_ ) )
				REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), in_param.src_loc_ );

			function_type.params.emplace_back();
			FunctionType::Param& out_param= function_type.params.back();

			if( is_this )
			{
				func_variable.is_this_call= true;
				if( base_class == nullptr )
				{
					REPORT_ERROR( ThisInNonclassFunction, names_scope.GetErrors(), in_param.src_loc_, func_name );
					return ~0u;
				}
				out_param.type= base_class;
			}
			else
				out_param.type= PrepareType( in_param.type_, names_scope, *global_function_context_ );

			if( is_this )
				out_param.value_type= ( is_special_method || in_param.mutability_modifier_ == MutabilityModifier::Mutable ) ? ValueType::ReferenceMut : ValueType::ReferenceImut;
			else if( in_param.reference_modifier_ == ReferenceModifier::Reference )
				out_param.value_type= in_param.mutability_modifier_ == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;
			else
				out_param.value_type= ValueType::Value;

			ProcessFunctionParamReferencesTags( func.type_, function_type, in_param, out_param, function_type.params.size() - 1u );
		} // for arguments

		function_type.unsafe= func.type_.unsafe_;

		if (function_type.unsafe && base_class != nullptr )
		{
			// Calls to such methods are produced by compiler itself, used in other methods generation, etc.
			// So, to avoid problems with generated unsafe calls just forbid some methods to be unsafe.
			if( is_destructor ||
				( is_constructor && ( IsDefaultConstructor( function_type, base_class ) || IsCopyConstructor( function_type, base_class ) ) ) ||
				( func.overloaded_operator_ == OverloadedOperator::Assign && IsCopyAssignmentOperator( function_type, base_class ) ) ||
				( func.overloaded_operator_ == OverloadedOperator::CompareEqual && IsEqualityCompareOperator( function_type, base_class ) ) )
			{
				REPORT_ERROR( ThisMethodCanNotBeUnsafe, names_scope.GetErrors(), func.src_loc_ );
				function_type.unsafe= false;
			}
		}

		TryGenerateFunctionReturnReferencesMapping( names_scope.GetErrors(), func.type_, function_type );
		ProcessFunctionReferencesPollution( names_scope.GetErrors(), func, function_type, base_class );
		CheckOverloadedOperator( base_class, function_type, func.overloaded_operator_, names_scope.GetErrors(), func.src_loc_ );

		function_type.calling_convention= GetLLVMCallingConvention( func.type_.calling_convention_, func.type_.src_loc_, names_scope.GetErrors() );
		// Disable non-default calling conventions for this-call methods and operators because of problems with call of generated methods/operators.
		// But it's fine to use custom calling convention for static methods.
		if( function_type.calling_convention != llvm::CallingConv::C &&
			( func_variable.is_this_call || func.overloaded_operator_ != OverloadedOperator::None ) )
			REPORT_ERROR( NonDefaultCallingConventionForClassMethod, names_scope.GetErrors(), func.src_loc_ );

		function_type.llvm_type= GetLLVMFunctionType( function_type );

		func_variable.type= std::move(function_type);
	} // end prepare function type

	// Set constexpr.
	if( func.constexpr_ )
	{
		if( func.block_ == nullptr )
			REPORT_ERROR( ConstexprFunctionsMustHaveBody, names_scope.GetErrors(), func.src_loc_ );
		if( func.virtual_function_kind_ != Synt::VirtualFunctionKind::None )
			REPORT_ERROR( ConstexprFunctionCanNotBeVirtual, names_scope.GetErrors(), func.src_loc_ );

		func_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprIncomplete;
	}
	else if( func.is_template_ )
		func_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprAuto;

	// Set virtual.
	if( func.virtual_function_kind_ != Synt::VirtualFunctionKind::None )
	{
		if( base_class == nullptr )
			REPORT_ERROR( VirtualForNonclassFunction, names_scope.GetErrors(), func.src_loc_, func_name );
		if( !func_variable.is_this_call )
			REPORT_ERROR( VirtualForNonThisCallFunction, names_scope.GetErrors(), func.src_loc_, func_name );
		if( is_constructor )
			REPORT_ERROR( FunctionCanNotBeVirtual, names_scope.GetErrors(), func.src_loc_, func_name );
		if( base_class != nullptr && ( base_class->kind == Class::Kind::Struct || base_class->kind == Class::Kind::NonPolymorph ) )
			REPORT_ERROR( VirtualForNonpolymorphClass, names_scope.GetErrors(), func.src_loc_, func_name );
		if( is_out_of_line_function )
			REPORT_ERROR( VirtualForFunctionImplementation, names_scope.GetErrors(), func.src_loc_, func_name );

		func_variable.virtual_function_kind= func.virtual_function_kind_;
	}

	// Set no_mangle
	if( func.no_mangle_ )
	{
		// Allow only global no-mangle function. This prevents existing of multiple "nomangle" functions with same name in different namespaces.
		// If function is operator, it can not be global.
		if( names_scope.GetParent() != nullptr )
			REPORT_ERROR( NoMangleForNonglobalFunction, names_scope.GetErrors(), func.src_loc_, func_name );
		func_variable.no_mangle= true;
	}

	// Set conversion constructor.
	func_variable.is_conversion_constructor= func.is_conversion_constructor_;
	U_ASSERT( !( func.is_conversion_constructor_ && !is_constructor ) );
	if( func.is_conversion_constructor_ && func_variable.type.GetFunctionType()->params.size() != 2u )
		REPORT_ERROR( ConversionConstructorMustHaveOneArgument, names_scope.GetErrors(), func.src_loc_ );
	func_variable.is_constructor= is_constructor;

	// Check "=default" / "=delete".
	if( func.body_kind != Synt::Function::BodyKind::None )
	{
		U_ASSERT( func.block_ == nullptr );
		const FunctionType& function_type= *func_variable.type.GetFunctionType();

		bool invalid_func= false;
		if( base_class == nullptr )
			invalid_func= true;
		else if( is_constructor )
			invalid_func= !( IsDefaultConstructor( function_type, base_class ) || IsCopyConstructor( function_type, base_class ) );
		else if( func.overloaded_operator_ == OverloadedOperator::Assign )
			invalid_func= !IsCopyAssignmentOperator( function_type, base_class );
		else if( func.overloaded_operator_ == OverloadedOperator::CompareEqual )
			invalid_func= !IsEqualityCompareOperator( function_type, base_class );
		else
			invalid_func= true;

		if( invalid_func )
			REPORT_ERROR( InvalidMethodForBodyGeneration, names_scope.GetErrors(), func.src_loc_ );
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
			prev_function->body_src_loc= func.src_loc_;
		}
		else if( prev_function->syntax_element->block_ != nullptr && func.block_ == nullptr )
		{ // Ok, prototype after body. Since order-independent resolving this is correct.
			prev_function->prototype_src_loc= func.src_loc_;
		}
		else if( prev_function->syntax_element->block_ == nullptr && func.block_ == nullptr )
			REPORT_ERROR( FunctionPrototypeDuplication, names_scope.GetErrors(), func.src_loc_, func_name );
		else if( prev_function->syntax_element->block_ != nullptr && func.block_ != nullptr )
			REPORT_ERROR( FunctionBodyDuplication, names_scope.GetErrors(), func.src_loc_, func_name );

		if( prev_function->is_this_call != func_variable.is_this_call )
			REPORT_ERROR( ThiscallMismatch, names_scope.GetErrors(), func.src_loc_, func_name );

		if( !is_out_of_line_function )
		{
			if( prev_function->virtual_function_kind != func.virtual_function_kind_ )
				REPORT_ERROR( VirtualMismatch, names_scope.GetErrors(), func.src_loc_, func_name );
		}
		if( prev_function->is_deleted != func_variable.is_deleted )
			REPORT_ERROR( BodyForDeletedFunction, names_scope.GetErrors(), prev_function->prototype_src_loc, func_name );
		if( prev_function->is_generated != func_variable.is_generated )
			REPORT_ERROR( BodyForGeneratedFunction, names_scope.GetErrors(), prev_function->prototype_src_loc, func_name );

		if( prev_function->no_mangle != func_variable.no_mangle )
			REPORT_ERROR( NoMangleMismatch, names_scope.GetErrors(), func.src_loc_, func_name );

		if( prev_function->is_conversion_constructor != func_variable.is_conversion_constructor )
			REPORT_ERROR( CouldNotOverloadFunction, names_scope.GetErrors(), func.src_loc_ );

		if( prev_function->is_inherited )
			REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), func.src_loc_ );

		return size_t(prev_function - functions_set.functions.data());
	}
	else
	{
		if( is_out_of_line_function )
		{
			REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), func.src_loc_ );
			return ~0u;
		}
		if( functions_set.have_nomangle_function || ( !functions_set.functions.empty() && func_variable.no_mangle ) )
		{
			REPORT_ERROR( CouldNotOverloadFunction, names_scope.GetErrors(), func.src_loc_ );
			return ~0u;
		}

		const bool overloading_ok= ApplyOverloadedFunction( functions_set, func_variable, names_scope.GetErrors(), func.src_loc_ );
		if( !overloading_ok )
			return ~0u;

		if( func_variable.no_mangle )
			functions_set.have_nomangle_function= true;

		FunctionVariable& inserted_func_variable= functions_set.functions.back();
		inserted_func_variable.body_src_loc= inserted_func_variable.prototype_src_loc= func.src_loc_;
		inserted_func_variable.syntax_element= &func;

		const FunctionType& func_type= *inserted_func_variable.type.GetFunctionType();

		inserted_func_variable.llvm_function=
			llvm::Function::Create(
				func_type.llvm_type,
				llvm::Function::LinkageTypes::ExternalLinkage, // External - for prototype.
				inserted_func_variable.no_mangle ? func_name : mangler_->MangleFunction( names_scope, func_name, func_type ),
				module_.get() );

		SetupFunctionParamsAndRetAttributes( inserted_func_variable );

		return functions_set.functions.size() - 1u;
	}
}

void CodeBuilder::CheckOverloadedOperator(
	const ClassPtr& base_class,
	const FunctionType& func_type,
	const OverloadedOperator overloaded_operator,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	if( overloaded_operator == OverloadedOperator::None )
		return; // Not operator

	if( base_class == nullptr )
	{
		REPORT_ERROR( OperatorDeclarationOutsideClass, errors_container, src_loc );
		return;
	}

	bool is_this_class= false;
	for( const FunctionType::Param& arg : func_type.params )
	{
		if( arg.type == base_class )
		{
			is_this_class= true;
			break;
		}
	}

	if( !is_this_class )
		REPORT_ERROR( OperatorDoesNotHaveParentClassArguments, errors_container, src_loc );

	const bool ret_is_void= func_type.return_type == void_type_ && func_type.return_value_type == ValueType::Value;

	switch( overloaded_operator )
	{
	case OverloadedOperator::Add:
	case OverloadedOperator::Sub:
		if( !( func_type.params.size() == 1u || func_type.params.size() == 2u ) )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, src_loc );
		break;

	case OverloadedOperator::CompareEqual:
		if( func_type.params.size() != 2u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, src_loc );
		if( !( func_type.return_type == bool_type_ && func_type.return_value_type == ValueType::Value ) )
			REPORT_ERROR( InvalidReturnTypeForOperator, errors_container, src_loc, bool_type_ );
		break;

	case OverloadedOperator::CompareOrder:
		if( func_type.params.size() != 2u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, src_loc );
		if( !(
				func_type.return_type.GetFundamentalType() != nullptr &&
				func_type.return_type.GetFundamentalType()->fundamental_type == U_FundamentalType::i32 &&
				func_type.return_value_type == ValueType::Value ) )
			REPORT_ERROR( InvalidReturnTypeForOperator, errors_container, src_loc, GetFundamentalTypeName( U_FundamentalType::i32 ) );
		break;
		
	case OverloadedOperator::Mul:
	case OverloadedOperator::Div:
	case OverloadedOperator::Rem:
	case OverloadedOperator::And:
	case OverloadedOperator::Or :
	case OverloadedOperator::Xor:
	case OverloadedOperator::ShiftLeft :
	case OverloadedOperator::ShiftRight:
		if( func_type.params.size() != 2u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, src_loc );
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
		if( func_type.params.size() != 2u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, src_loc );
		if( !ret_is_void )
			REPORT_ERROR( InvalidReturnTypeForOperator, errors_container, src_loc, void_type_ );
		break;

	case OverloadedOperator::LogicalNot:
	case OverloadedOperator::BitwiseNot:
		if( func_type.params.size() != 1u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, src_loc );
		break;

	case OverloadedOperator::Assign:
		if( func_type.params.size() != 2u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, src_loc );
		if( !ret_is_void )
			REPORT_ERROR( InvalidReturnTypeForOperator, errors_container, src_loc, void_type_ );
		break;

	case OverloadedOperator::Increment:
	case OverloadedOperator::Decrement:
		if( func_type.params.size() != 1u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, src_loc );
		if( !ret_is_void )
			REPORT_ERROR( InvalidReturnTypeForOperator, errors_container, src_loc, void_type_ );
		break;

	case OverloadedOperator::Indexing:
		if( func_type.params.size() != 2u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, src_loc );
		// Indexing operator must have first argument of parent class.
		if( !func_type.params.empty() && func_type.params[0].type != base_class )
			REPORT_ERROR( OperatorDoesNotHaveParentClassArguments, errors_container, src_loc );
		break;

	case OverloadedOperator::Call:
		if( func_type.params.empty() )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, src_loc );
		// Call operator must have first argument of parent class.
		if( !func_type.params.empty() && func_type.params[0].type != base_class )
			REPORT_ERROR( OperatorDoesNotHaveParentClassArguments, errors_container, src_loc );
		break;

	case OverloadedOperator::None:
		U_ASSERT(false);
	};
}

Type CodeBuilder::BuildFuncCode(
	FunctionVariable& func_variable,
	const ClassPtr& base_class,
	NamesScope& parent_names_scope,
	const std::string& func_name,
	const Synt::FunctionParams& params,
	const Synt::Block& block,
	const Synt::StructNamedInitializer* const constructor_initialization_list )
{
	U_ASSERT( !func_variable.have_body );
	func_variable.have_body= true;

	const FunctionType& function_type= *func_variable.type.GetFunctionType();

	llvm::Function* const llvm_function= func_variable.llvm_function;

	// Build debug info only for functions with body.
	CreateFunctionDebugInfo( func_variable, func_name );

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

	// Ensure completeness only for functions body.
	// Require full completeness even for reference arguments.
	for( const FunctionType::Param& arg : function_type.params )
	{
		if( !EnsureTypeComplete( arg.type ) )
			REPORT_ERROR( UsingIncompleteType, parent_names_scope.GetErrors(), params.front().src_loc_, arg.type );
	}
	if( !EnsureTypeComplete( function_type.return_type ) )
		REPORT_ERROR( UsingIncompleteType, parent_names_scope.GetErrors(), func_variable.body_src_loc, function_type.return_type );

	NamesScope function_names( "", &parent_names_scope );
	FunctionContext function_context(
		function_type,
		func_variable.return_type_is_auto ? std::optional<Type>(): function_type.return_type,
		llvm_context_,
		llvm_function );
	const StackVariablesStorage args_storage( function_context );

	function_context.args_nodes.resize( function_type.params.size() );

	SetCurrentDebugLocation( func_variable.body_src_loc, function_context );

	// push args
	Variable this_;
	unsigned int arg_number= 0u;

	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;

	for( llvm::Argument& llvm_arg : llvm_function->args() )
	{
		// Skip "sret".
		if( &llvm_arg == &*llvm_function->arg_begin() && function_type.IsStructRet() )
		{
			llvm_arg.setName( "_return_value" );
			function_context.s_ret_= &llvm_arg;
			continue;
		}

		const FunctionType::Param& param= function_type.params[ arg_number ];

		const Synt::FunctionParam& declaration_arg= params[arg_number ];
		const std::string& arg_name= declaration_arg.name_;

		const bool is_this= arg_number == 0u && arg_name == Keywords::this_;
		U_ASSERT( !( is_this && param.value_type == ValueType::Value ) );

		Variable var;
		var.location= Variable::Location::Pointer;
		var.value_type= ValueType::ReferenceMut;
		var.type= param.type;

		if( declaration_arg.mutability_modifier_ != MutabilityModifier::Mutable )
			var.value_type= ValueType::ReferenceImut;

		if( param.value_type != ValueType::Value )
		{
			var.llvm_value= &llvm_arg;
			CreateReferenceVariableDebugInfo( var, arg_name, declaration_arg.src_loc_, function_context );
		}
		else
		{
			if( param.type.GetFundamentalType() != nullptr ||
				param.type.GetEnumType() != nullptr ||
				param.type.GetRawPointerType() != nullptr ||
				param.type.GetFunctionPointerType() != nullptr )
			{
				// Move parameters to stack for assignment possibility.
				var.llvm_value= function_context.alloca_ir_builder.CreateAlloca( var.type.GetLLVMType(), nullptr, arg_name );
				CreateLifetimeStart( function_context, var.llvm_value );

				if( param.type != void_type_ )
					function_context.llvm_ir_builder.CreateStore( &llvm_arg, var.llvm_value );
			}
			else if( param.type.GetClassType() != nullptr || param.type.GetArrayType() != nullptr || param.type.GetTupleType() != nullptr )
			{
				// Composite types use llvm-pointers.
				var.llvm_value = &llvm_arg;
			}
			else U_ASSERT(false);

			CreateVariableDebugInfo( var, arg_name, declaration_arg.src_loc_, function_context );
		}

		// Create variable node, because only variable node can have inner reference node.
		// Register arg on stack, only if it is value-argument.
		const auto var_node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, arg_name );
		function_context.args_nodes[ arg_number ].first= var_node;
		if( param.value_type != ValueType::Value )
		{
			const auto reference_node= function_context.variables_state.AddNode(
				param.value_type == ValueType::ReferenceMut ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut,
				arg_name + " reference" );
			function_context.variables_state.AddLink( var_node, reference_node );
			var.node= reference_node;
		}
		else
		{
			var.node= var_node;
			function_context.stack_variables_stack.back()->RegisterVariable( var );
		}

		if (param.type.ReferencesTagsCount() > 0u )
		{
			// Create inner node + root variable.
			const auto accesible_variable= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable , arg_name + " referenced variable" );

			const auto inner_reference= function_context.variables_state.CreateNodeInnerReference(
				var_node,
				param.type.GetInnerReferenceType() == InnerReferenceType::Mut ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
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
			const Value* const inserted_arg=
				function_names.AddName( arg_name, Value( var, declaration_arg.src_loc_ ) );
			if( inserted_arg == nullptr )
				REPORT_ERROR( Redefinition, function_names.GetErrors(), declaration_arg.src_loc_, arg_name );
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
			const Synt::StructNamedInitializer dumy_initialization_list( block.src_loc_ );

			BuildConstructorInitialization(
				*function_context.this_,
				*base_class,
				function_names,
				function_context,
				dumy_initialization_list );
		}
		else
			BuildConstructorInitialization(
				*function_context.this_,
				*base_class,
				function_names,
				function_context,
				*constructor_initialization_list );

		function_context.whole_this_is_unavailable= false;
	}

	if( ( is_constructor || is_destructor ) && ( base_class->kind == Class::Kind::Abstract || base_class->kind == Class::Kind::Interface ) )
		function_context.whole_this_is_unavailable= true; // Whole "this" unavailable in body of constructors and destructors of abstract classes and interfaces.

	if( is_destructor )
	{
		SetupVirtualTablePointers( this_.llvm_value, *base_class, function_context );
		function_context.destructor_end_block= llvm::BasicBlock::Create( llvm_context_ );
	}

	const BlockBuildInfo block_build_info= BuildBlock( function_names, function_context, block );
	U_ASSERT( function_context.stack_variables_stack.size() == 1u );

	// If we build func code only for return type deducing - we can return. Function code will be generated later.
	if( func_variable.return_type_is_auto )
	{
		func_variable.return_type_is_auto= false;
		return function_context.deduced_return_type ? *function_context.deduced_return_type : void_type_;
	}

	if( func_variable.constexpr_kind != FunctionVariable::ConstexprKind::NonConstexpr )
	{
		// Check function type and function body.
		// Function type checked here, because in case of constexpr methods not all types are complete yet.

		// For auto-constexpr functions we do not force type completeness. If function is really-constexpr, it must already make complete using types.

		const bool auto_contexpr= func_variable.constexpr_kind == FunctionVariable::ConstexprKind::ConstexprAuto;
		bool can_be_constexpr= true;

		if( !auto_contexpr && !EnsureTypeComplete( function_type.return_type ) )
			REPORT_ERROR( UsingIncompleteType, function_names.GetErrors(), func_variable.body_src_loc, function_type.return_type ); // Completeness required for constexpr possibility check.

		if( function_type.unsafe ||
			!function_type.return_type.CanBeConstexpr() ||
			!function_type.references_pollution.empty() ) // Side effects, such pollution, not allowed.
			can_be_constexpr= false;

		if( function_type.return_type.GetFunctionPointerType() != nullptr ) // Currently function pointers not supported.
			can_be_constexpr= false;

		for( const FunctionType::Param& param : function_type.params )
		{
			if( !auto_contexpr && !EnsureTypeComplete( param.type ) )
				REPORT_ERROR( UsingIncompleteType, function_names.GetErrors(), func_variable.body_src_loc, param.type ); // Completeness required for constexpr possibility check.

			if( !param.type.CanBeConstexpr() || // Allowed only constexpr types. Incomplete types are not constexpr.
				param.type.GetFunctionPointerType() != nullptr )
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
				REPORT_ERROR( InvalidTypeForConstexprFunction, function_names.GetErrors(), func_variable.body_src_loc );
				func_variable.constexpr_kind= FunctionVariable::ConstexprKind::NonConstexpr;
			}
			else if( function_context.have_non_constexpr_operations_inside )
			{
				REPORT_ERROR( ConstexprFunctionContainsUnallowedOperations, function_names.GetErrors(), func_variable.body_src_loc );
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
		if( function_type.return_type == void_type_ && function_type.return_value_type == ValueType::Value )
		{
			// Manually generate "return" for void-return functions.
			CallDestructors( args_storage, function_names, function_context, block.end_src_loc_ );
			CheckReferencesPollutionBeforeReturn( function_context, function_names.GetErrors(), block.end_src_loc_ );

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
			REPORT_ERROR( NoReturnInFunctionReturningNonVoid, function_names.GetErrors(), block.end_src_loc_ );
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

		CallMembersDestructors( function_context, function_names.GetErrors(), block.end_src_loc_ );
		function_context.llvm_ir_builder.CreateRetVoid();
	}

	// Replace return value allocation at end of function build process.
	// We can do this only now, because now there is no "llvm_value" for this allocation stored in some intermediate structs.
	if( function_context.return_value_replaced_allocation != nullptr )
	{
		U_ASSERT( function_context.s_ret_ != nullptr );
		function_context.return_value_replaced_allocation->replaceAllUsesWith( function_context.s_ret_ );
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
				REPORT_ERROR( BaseUnavailable, names_scope.GetErrors(), constructor_initialization_list.src_loc_ );
				continue;
			}
			if( base_initialized )
			{
				have_fields_errors= true;
				REPORT_ERROR( DuplicatedStructMemberInitializer, names_scope.GetErrors(), constructor_initialization_list.src_loc_, field_initializer.name );
				continue;
			}
			base_initialized= true;
			function_context.base_initialized= false;
			continue;
		}

		const Value* const class_member= base_class.members->GetThisScopeValue( field_initializer.name );
		if( class_member == nullptr )
		{
			have_fields_errors= true;
			REPORT_ERROR( NameNotFound, names_scope.GetErrors(), constructor_initialization_list.src_loc_, field_initializer.name );
			continue;
		}

		const ClassField* const field= class_member->GetClassField();
		if( field == nullptr )
		{
			have_fields_errors= true;
			REPORT_ERROR( InitializerForNonfieldStructMember, names_scope.GetErrors(), constructor_initialization_list.src_loc_, field_initializer.name );
			continue;
		}
		if( field->class_ != &base_class )
		{
			have_fields_errors= true;
			REPORT_ERROR( InitializerForBaseClassField, names_scope.GetErrors(), constructor_initialization_list.src_loc_, field_initializer.name );
			continue;
		}

		if( initialized_fields.find( field_initializer.name ) != initialized_fields.end() )
		{
			have_fields_errors= true;
			REPORT_ERROR( DuplicatedStructMemberInitializer, names_scope.GetErrors(), constructor_initialization_list.src_loc_, field_initializer.name );
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

		const ClassField& field= *base_class.members->GetThisScopeValue( field_name )->GetClassField();

		const StackVariablesStorage temp_variables_storage( function_context );

		if( field.is_reference )
		{
			if( field.syntax_element->initializer == nullptr )
			{
				REPORT_ERROR( ExpectedInitializer, names_scope.GetErrors(), constructor_initialization_list.src_loc_, field_name );
				continue;
			}
			InitializeReferenceClassFieldWithInClassIninitalizer( this_, field, function_context );
		}
		else
		{
			Variable field_variable;
			field_variable.type= field.type;
			field_variable.location= Variable::Location::Pointer;
			field_variable.value_type= ValueType::ReferenceMut;

			field_variable.llvm_value= CreateClassFiledGEP( function_context, this_.llvm_value, field.index );

			if( field.syntax_element->initializer != nullptr )
				InitializeClassFieldWithInClassIninitalizer( field_variable, field, function_context );
			else
				ApplyEmptyInitializer( field_name, constructor_initialization_list.src_loc_, field_variable, names_scope, function_context );
		}
		CallDestructors( temp_variables_storage, names_scope, function_context, constructor_initialization_list.src_loc_ );
	}
	if( !base_initialized && base_class.base_class != nullptr )
	{
		const StackVariablesStorage temp_variables_storage( function_context );

		// Apply default initializer for base class.
		Variable base_variable;
		base_variable.type= base_class.base_class;
		base_variable.location= Variable::Location::Pointer;
		base_variable.value_type= ValueType::ReferenceMut;

		base_variable.llvm_value= CreateBaseClassGEP( function_context, this_.llvm_value );

		ApplyEmptyInitializer( base_class.base_class->members->GetThisNamespaceName(), constructor_initialization_list.src_loc_, base_variable, names_scope, function_context );
		function_context.base_initialized= true;

		CallDestructors( temp_variables_storage, names_scope, function_context, constructor_initialization_list.src_loc_ );
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
			base_variable.value_type= ValueType::ReferenceMut;
			base_variable.node= this_.node;

			base_variable.llvm_value= CreateBaseClassGEP( function_context, this_.llvm_value );

			ApplyInitializer( base_variable, names_scope, function_context, field_initializer.initializer );
			function_context.base_initialized= true;
			continue;
		}

		const Value* const class_member=
			base_class.members->GetThisScopeValue( field_initializer.name );
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
			field_variable.value_type= ValueType::ReferenceMut;
			field_variable.node= this_.node;

			field_variable.llvm_value= CreateClassFiledGEP( function_context, this_.llvm_value, field->index );

			ApplyInitializer( field_variable, names_scope, function_context, field_initializer.initializer );
		}

		function_context.uninitialized_this_fields.erase( field->syntax_element->name );

		CallDestructors( temp_variables_storage, names_scope, function_context, Synt::GetInitializerSrcLoc( field_initializer.initializer ) );
	} // for fields initializers

	SetupVirtualTablePointers( this_.llvm_value, base_class, function_context );
}

void CodeBuilder::BuildStaticAssert( StaticAssert& static_assert_, NamesScope& names, FunctionContext& function_context )
{
	if( static_assert_.syntax_element == nullptr )
		return;

	BuildBlockElementImpl( names, function_context, *static_assert_.syntax_element );
	static_assert_.syntax_element= nullptr;
}

Value CodeBuilder::ResolveValue(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::ComplexName& complex_name,
	const ResolveMode resolve_mode )
{
	const SrcLoc& src_loc= complex_name.src_loc_;

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
			REPORT_ERROR( NameNotFound, names_scope.GetErrors(), src_loc, *name );

		if( ( *name == Keywords::constructor_ || *name == Keywords::destructor_ ) && !function_context.is_in_unsafe_block )
			REPORT_ERROR( ExplicitAccessToThisMethodIsUnsafe, names_scope.GetErrors(), src_loc, *name );
	}
	else if( const auto typeof_type_name= std::get_if<Synt::TypeofTypeName>(&complex_name.start_value) )
	{
		temp_value_storage= Value( PrepareTypeImpl( names_scope, function_context, *typeof_type_name ), src_loc );
		value= &temp_value_storage;
	}
	else if(const auto simple_name= std::get_if<std::string>(&complex_name.start_value) )
	{
		do
		{
			if( const auto class_type= last_space->GetClass() )
			{
				const auto class_value= ResolveClassValue( class_type, *simple_name );
				value= class_value.first;
				if( names_scope.GetAccessFor( class_type ) < class_value.second )
					REPORT_ERROR( AccessingNonpublicClassMember, names_scope.GetErrors(), src_loc, *simple_name, last_space->GetThisNamespaceName() );
			}
			else
				value= last_space->GetThisScopeValue( *simple_name );
			if( value != nullptr )
				break;
			last_space= last_space->GetParent();
		} while( last_space != nullptr );

		if( value == nullptr )
			REPORT_ERROR( NameNotFound, names_scope.GetErrors(), src_loc, *simple_name );
	}
	else U_ASSERT(false);

	if( value != nullptr && value->GetYetNotDeducedTemplateArg() != nullptr )
		REPORT_ERROR( TemplateArgumentIsNotDeducedYet, names_scope.GetErrors(), src_loc, complex_name );

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
				if( ClassPtr class_= type->GetClassType() )
				{
					const auto class_value= ResolveClassValue( class_, *component_name );
					if( names_scope.GetAccessFor( class_ ) < class_value.second )
						REPORT_ERROR( AccessingNonpublicClassMember, names_scope.GetErrors(), src_loc, *component_name, class_->members->GetThisNamespaceName() );

					if( ( *component_name == Keywords::constructor_ || *component_name == Keywords::destructor_ ) && !function_context.is_in_unsafe_block )
						REPORT_ERROR( ExplicitAccessToThisMethodIsUnsafe, names_scope.GetErrors(), src_loc, *component_name );

					value= class_value.first;
					// This is wrong for name fetched from parent space.
					// But it is fine until we perform real build of global things, using this space.
					// "ResolveClassValue" function should build resolved global things, using correct namespace.
					last_space= class_->members.get();
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
				REPORT_ERROR( TemplateInstantiationRequired, names_scope.GetErrors(), src_loc, *component_name );
				return ErrorValue();
			}

			if( value == nullptr )
				REPORT_ERROR( NameNotFound, names_scope.GetErrors(), src_loc, *component_name );
		}
		else if( const auto template_parameters= std::get_if< std::vector<Synt::Expression> >( &component->name_or_template_paramenters ) )
		{
			if( TypeTemplatesSet* const type_templates_set= value->GetTypeTemplatesSet() )
			{
				GlobalThingBuildTypeTemplatesSet( *last_space, *type_templates_set );
				value=
					GenTemplateType(
						src_loc,
						*type_templates_set,
						*template_parameters,
						names_scope,
						function_context );
				if( value == nullptr )
					return ErrorValue();

				const Type* const type= value->GetTypeName();
				U_ASSERT( type != nullptr );
				if( Class* const class_= type->GetClassType() )
					last_space= class_->members.get();
			}
			else if( OverloadedFunctionsSet* const functions_set= value->GetFunctionsSet() )
			{
				GlobalThingBuildFunctionsSet( *last_space, *functions_set, false );
				if( functions_set->template_functions.empty() )
				{
					REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), src_loc );
					return ErrorValue();
				}

				value=
					ParametrizeFunctionTemplate(
						src_loc,
						*functions_set,
						*template_parameters,
						names_scope,
						function_context );
				if( value == nullptr )
				{
					REPORT_ERROR( TemplateFunctionGenerationFailed, names_scope.GetErrors(), src_loc, complex_name );
					return ErrorValue();
				}
			}
			else
			{
				REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), src_loc );
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
			if( const EnumPtr enum_= type->GetEnumType() )
				GlobalThingBuildEnum( enum_ );
		}
	}

	return value == nullptr ? ErrorValue() : *value;
}

std::pair<Value*, ClassMemberVisibility> CodeBuilder::ResolveClassValue( const ClassPtr class_type, const std::string& name )
{
	return ResolveClassValueImpl( class_type, name );
}

std::pair<Value*, ClassMemberVisibility> CodeBuilder::ResolveClassValueImpl( ClassPtr class_type, const std::string& name, const bool recursive_call )
{
	const bool is_special_method=
		name == Keyword( Keywords::constructor_ ) ||
		name == Keyword( Keywords::destructor_ ) ||
		name == OverloadedOperatorToString( OverloadedOperator::Assign ) ||
		name == OverloadedOperatorToString( OverloadedOperator::CompareEqual ) ||
		name == OverloadedOperatorToString( OverloadedOperator::CompareOrder );

	if( is_special_method )
	{
		// Special methods may be generated during class build. So, require complete type to access these methods.
		GlobalThingBuildClass( class_type ); // Functions set changed in this call.
	}

	if( const auto value= class_type->members->GetThisScopeValue( name ) )
	{
		const auto visibility= class_type->GetMemberVisibility( name );

		// We need to build some things right now (typedefs, global variables, etc.) in order to do this in correct namespace - namespace of class itself but not namespace of one of its child.

		if( value->GetClassField() != nullptr )
		{
			if( !class_type->is_complete )
			{
				// We can't just return class field value if class is incomplete. So, request class build to fill class field properly.
				GlobalThingBuildClass( class_type ); // Class field value changed in this call.
			}
		}
		else if( const auto functions_set= value->GetFunctionsSet() )
		{
			GlobalThingPrepareClassParentsList( class_type );
			if( !class_type->is_complete && !class_type->parents.empty() )
			{
				// Request class build in order to merge functions from parent classes into this functions set.
				GlobalThingBuildClass( class_type ); // Functions set changed in this call.
			}
			GlobalThingBuildFunctionsSet( *class_type->members, *functions_set, false );
		}
		else if( const auto type_templates_set= value->GetTypeTemplatesSet() )
		{
			GlobalThingPrepareClassParentsList( class_type );
			if( !class_type->is_complete && !class_type->parents.empty() )
			{
				// Request class build in order to merge type templaes from parent classes into this type templates set.
				GlobalThingBuildClass( class_type ); // Type templates set changed in this call.
			}
			GlobalThingBuildTypeTemplatesSet( *class_type->members, *type_templates_set );
		}
		else if( value->GetTypedef() != nullptr )
		{
			GlobalThingBuildTypedef( *class_type->members, *value );
		}
		else if( value->GetIncompleteGlobalVariable() != nullptr )
		{
			GlobalThingBuildVariable( *class_type->members, *value );
		}
		else if( const auto type= value->GetTypeName() )
		{
			if( const auto enum_= type->GetEnumType() )
				GlobalThingBuildEnum( enum_ );
		}
		else if(
			value->GetVariable() != nullptr ||
			value->GetStaticAssert() != nullptr ||
			value->GetYetNotDeducedTemplateArg() != nullptr )
		{}
		else U_ASSERT(false);

		return std::make_pair( value, visibility );
	}

	// Do not try to fetch special methods from parent.
	if( is_special_method )
		return std::make_pair( nullptr, ClassMemberVisibility::Public );

	// Value not found in this class. Try to fetch value from parents.
	GlobalThingPrepareClassParentsList( class_type );

	// TODO - maybe produce some kind of error if name found more than in one parents?
	std::pair<Value*, ClassMemberVisibility> parent_class_value;
	bool has_mergable_thing= false;
	for( const Class::Parent& parent : class_type->parents )
	{
		const auto current_parent_class_value= ResolveClassValue( parent.class_, name );
		if( current_parent_class_value.first != nullptr && current_parent_class_value.second != ClassMemberVisibility::Private )
		{
			const bool current_thing_is_mergable=
				current_parent_class_value.first->GetFunctionsSet() != nullptr ||
				current_parent_class_value.first->GetTypeTemplatesSet() != nullptr;

			if( current_thing_is_mergable && has_mergable_thing && !class_type->is_complete && !recursive_call )
			{
				// If we found more than one functions sets or template sets - trigger class build and perform resolve again.
				// Mergable thisngs will be merged during class build and added into class namespace.
				GlobalThingBuildClass( class_type );
				return ResolveClassValueImpl( class_type, name, true );
			}

			parent_class_value= current_parent_class_value;
			has_mergable_thing|= current_thing_is_mergable;
		}
	}

	// Return public class member as public, protected as protected. Do not return parent class private members.
	return parent_class_value;
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

llvm::Value* CodeBuilder::CreateMoveToLLVMRegisterInstruction( const Variable& variable, FunctionContext& function_context )
{
	// Contant values always are register-values.
	if( variable.constexpr_value != nullptr )
		return variable.constexpr_value;

	switch( variable.location )
	{
	case Variable::Location::LLVMRegister:
		return variable.llvm_value;
	case Variable::Location::Pointer:
		if( variable.type == void_type_ )
			return llvm::UndefValue::get(fundamental_llvm_types_.void_);
		else
			return function_context.llvm_ir_builder.CreateLoad( variable.type.GetLLVMType(), variable.llvm_value );
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

llvm::Value*CodeBuilder:: CreateBaseClassGEP( FunctionContext& function_context, llvm::Value* const class_ptr )
{
	return CreateClassFiledGEP( function_context, class_ptr, 0 /* base class is allways first field */ );
}

llvm::Value*CodeBuilder:: CreateVirtualTablePointerGEP( FunctionContext& function_context, llvm::Value* const class_ptr )
{
	return CreateClassFiledGEP( function_context, class_ptr, 0 /* virtual table pointer is allways first field */ );
}

llvm::Value* CodeBuilder::CreateClassFiledGEP( FunctionContext& function_context, const Variable& class_variable, const ClassField& class_field )
{
	ClassPtr actual_field_class= class_variable.type.GetClassType();
	llvm::Value* actual_field_class_ptr= class_variable.llvm_value;
	while( actual_field_class != class_field.class_ )
	{
		if( actual_field_class->base_class == nullptr )
			return nullptr;
		actual_field_class_ptr= CreateBaseClassGEP( function_context, actual_field_class_ptr );
		actual_field_class= actual_field_class->base_class;
	}

	return CreateClassFiledGEP( function_context, actual_field_class_ptr, class_field.index );
}

llvm::Value* CodeBuilder::CreateClassFiledGEP( FunctionContext& function_context, llvm::Value* const class_ptr, const uint64_t field_index )
{
	return function_context.llvm_ir_builder.CreateGEP(
		class_ptr->getType()->getPointerElementType(),
		class_ptr,
		{ GetZeroGEPIndex(), GetFieldGEPIndex( field_index ) } );
}

llvm::Value* CodeBuilder::CreateTupleElementGEP( FunctionContext& function_context, llvm::Value* tuple_ptr, const uint64_t element_index )
{
	return CreateClassFiledGEP( function_context, tuple_ptr, element_index );
}

llvm::Value* CodeBuilder::CreateArrayElementGEP( FunctionContext& function_context, llvm::Value* const array_ptr, const uint64_t element_index )
{
	return CreateArrayElementGEP( function_context, array_ptr, llvm::ConstantInt::get( fundamental_llvm_types_.u64, element_index ) );
}

llvm::Value* CodeBuilder::CreateArrayElementGEP( FunctionContext& function_context, llvm::Value* const array_ptr, llvm::Value* const index )
{
	return function_context.llvm_ir_builder.CreateGEP(
		array_ptr->getType()->getPointerElementType(),
		array_ptr,
		{ GetZeroGEPIndex(), index } );
}

llvm::Value* CodeBuilder::CreateReferenceCast( llvm::Value* const ref, const Type& src_type, const Type& dst_type, FunctionContext& function_context )
{
	U_ASSERT( src_type.ReferenceIsConvertibleTo( dst_type ) );

	if( src_type == dst_type )
		return ref;

	const Class* const src_class_type= src_type.GetClassType();
	U_ASSERT( src_class_type != nullptr );

	for( const Class::Parent& src_parent_class : src_class_type->parents )
	{
		llvm::Value* const sub_ref= CreateClassFiledGEP( function_context, ref, src_parent_class.field_number );

		if( src_parent_class.class_ == dst_type )
			return sub_ref;
		else if( Type(src_parent_class.class_).ReferenceIsConvertibleTo( dst_type ) )
			return CreateReferenceCast( sub_ref, src_parent_class.class_, dst_type, function_context );
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

llvm::GlobalVariable* CodeBuilder::CreateGlobalMutableVariable( const Type& type, const std::string& mangled_name )
{
	// Use external linkage and comdat for global mutable variables to guarantee address uniqueness.
	const auto var=
		new llvm::GlobalVariable(
			*module_,
			type.GetLLVMType(),
			false, // is constant
			llvm::GlobalValue::ExternalLinkage,
			nullptr,
			mangled_name );

	llvm::Comdat* const comdat= module_->getOrInsertComdat( var->getName() );
	comdat->setSelectionKind( llvm::Comdat::Any );
	var->setComdat( comdat );

	var->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );

	return var;
}

void CodeBuilder::SetupFunctionParamsAndRetAttributes( FunctionVariable& function_variable )
{
	const auto llvm_function= function_variable.llvm_function;
	const FunctionType& function_type= *function_variable.type.GetFunctionType();

	const bool first_arg_is_sret= function_type.IsStructRet();

	for( size_t i= 0u; i < function_type.params.size(); i++ )
	{
		const auto arg_attr_index=
			static_cast<unsigned int>(llvm::AttributeList::FirstArgIndex + i + (first_arg_is_sret ? 1u : 0u ));
		const FunctionType::Param& param= function_type.params[i];

		const bool param_is_composite= param.type.GetClassType() != nullptr || param.type.GetArrayType() != nullptr || param.type.GetTupleType() != nullptr;
		// Mark reference params as nonnull.
		if( param.value_type != ValueType::Value || param_is_composite )
			llvm_function->addAttribute( arg_attr_index, llvm::Attribute::NonNull );
		// Mutable reference params or composite value-args must not alias.
		// Also we can mark as "noalias" non-mutable references. See https://releases.llvm.org/9.0.0/docs/AliasAnalysis.html#must-may-or-no.
		if( param.value_type != ValueType::Value || param_is_composite )
			llvm_function->addAttribute( arg_attr_index, llvm::Attribute::NoAlias );
		// Mark as "readonly" immutable reference params.
		if( param.value_type == ValueType::ReferenceImut )
			llvm_function->addAttribute( arg_attr_index, llvm::Attribute::ReadOnly );
		// Mark as "nocapture" value args of composite types, which is actually passed by hidden reference.
		// It is not possible to capture this reference.
		if( param.value_type == ValueType::Value && param_is_composite )
			llvm_function->addAttribute( arg_attr_index, llvm::Attribute::NoCapture );
	}

	if( first_arg_is_sret )
	{
		llvm_function->addAttribute( llvm::AttributeList::FirstArgIndex, llvm::Attribute::StructRet );
		llvm_function->addAttribute( llvm::AttributeList::FirstArgIndex, llvm::Attribute::NoAlias );
	}
	if( function_type.return_value_type != ValueType::Value )
		llvm_function->addAttribute( llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull );

	// Merge functions with identical code.
	// We doesn`t need different addresses for different functions.
	llvm_function->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );

	llvm_function->setDoesNotThrow(); // We do not support exceptions.

	llvm_function->setCallingConv( function_type.calling_convention );

	if( build_debug_info_ ) // Unwind table entry for function needed for debug info.
		llvm_function->addFnAttr( llvm::Attribute::UWTable );

	// Use "private" linkage for generated functions since such functions are emitted in every compilation unit.
	if( function_variable.is_generated )
		llvm_function->setLinkage( llvm::GlobalValue::PrivateLinkage );
}

void CodeBuilder::SetupDereferenceableFunctionParamsAndRetAttributes( FunctionVariable& function_variable )
{
	const auto llvm_function= function_variable.llvm_function;
	const FunctionType& function_type= *function_variable.type.GetFunctionType();

	const bool first_arg_is_sret= function_type.IsStructRet();

	for( size_t i= 0u; i < function_type.params.size(); i++ )
	{
		const auto arg_attr_index=
			static_cast<unsigned int>(llvm::AttributeList::FirstArgIndex + i + (first_arg_is_sret ? 1u : 0u ));
		const FunctionType::Param& param= function_type.params[i];

		const bool param_is_composite= param.type.GetClassType() != nullptr || param.type.GetArrayType() != nullptr || param.type.GetTupleType() != nullptr;
		// Mark reference params and passed by hidden reference params with "dereferenceable" attribute.
		if( param.value_type != ValueType::Value || param_is_composite )
		{
			const auto llvm_type= param.type.GetLLVMType();
			if( !llvm_type->isSized() )
				continue; // May be in case of error.

			llvm_function->addDereferenceableAttr( arg_attr_index, data_layout_.getTypeAllocSize( llvm_type ) );
		}
	}

	const auto llvm_ret_type= function_type.return_type.GetLLVMType();
	if( !llvm_ret_type->isSized() )
		return; // May be in case of error.

	if( first_arg_is_sret )
		llvm_function->addDereferenceableAttr( llvm::AttributeList::FirstArgIndex, data_layout_.getTypeAllocSize( llvm_ret_type ) );
	else if( function_type.return_value_type != ValueType::Value )
		llvm_function->addDereferenceableAttr( llvm::AttributeList::ReturnIndex, data_layout_.getTypeAllocSize( llvm_ret_type ) );
}

void CodeBuilder::SetupDereferenceableFunctionParamsAndRetAttributes_r( NamesScope& names_scope )
{
	names_scope.ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const NamesScopePtr inner_namespace= value.GetNamespace() )
				SetupDereferenceableFunctionParamsAndRetAttributes_r( *inner_namespace );
			else if( OverloadedFunctionsSet* const functions_set= value.GetFunctionsSet() )
			{
				for( FunctionVariable& function_variable : functions_set->functions )
					SetupDereferenceableFunctionParamsAndRetAttributes( function_variable );
			}

			else if( const Type* const type= value.GetTypeName() )
			{
				if( const ClassPtr class_type= type->GetClassType() )
				{
					// Build classes only from parent namespace.
					// Otherwise we can get loop, using typedef.
					if( class_type->members->GetParent() == &names_scope )
						SetupDereferenceableFunctionParamsAndRetAttributes_r( *class_type->members );
				}
			}
		});
}

void CodeBuilder::CreateLifetimeStart( FunctionContext& function_context, llvm::Value* const address )
{
	if( !create_lifetimes_ )
		return;

	if( llvm::dyn_cast<llvm::AllocaInst>(address) == nullptr )
		return;

	llvm::Type* const type= address->getType()->getPointerElementType();
	if( !type->isSized() )
		return; // May be in case of error.

	function_context.llvm_ir_builder.CreateLifetimeStart(
		address,
		llvm::ConstantInt::get(
			fundamental_llvm_types_.u64,
			data_layout_.getTypeAllocSize(type) ) );

	if( generate_lifetime_start_end_debug_calls_ )
		function_context.llvm_ir_builder.CreateCall(
			lifetime_start_debug_func_,
			{
				function_context.llvm_ir_builder.CreatePointerCast( address, lifetime_start_debug_func_->arg_begin()->getType() )
			} );
}

void CodeBuilder::CreateLifetimeEnd( FunctionContext& function_context, llvm::Value* const address )
{
	if( !create_lifetimes_ )
		return;

	if( llvm::dyn_cast<llvm::AllocaInst>(address) == nullptr )
		return;

	llvm::Type* const type= address->getType()->getPointerElementType();
	if( !type->isSized() )
		return; // May be in case of error.

	function_context.llvm_ir_builder.CreateLifetimeEnd(
		address,
		llvm::ConstantInt::get(
			fundamental_llvm_types_.u64,
			data_layout_.getTypeAllocSize(type) ) );

	if( generate_lifetime_start_end_debug_calls_ )
		function_context.llvm_ir_builder.CreateCall(
			lifetime_end_debug_func_,
			{
				function_context.llvm_ir_builder.CreatePointerCast( address, lifetime_end_debug_func_->arg_begin()->getType() )
			} );
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

} // namespace U
