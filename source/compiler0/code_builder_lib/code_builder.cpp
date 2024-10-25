#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "../../code_builder_lib_common/return_value_optimization.hpp"
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


CodeBuilder::BuildResult CodeBuilder::BuildProgram(
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	const CodeBuilderOptions& options,
	const SourceGraphPtr& source_graph,
	IVfsSharedPtr vfs )
{
	CodeBuilder code_builder(
		llvm_context,
		data_layout,
		target_triple,
		options,
		std::move(vfs) );

	code_builder.BuildProgramInternal( source_graph );
	code_builder.FinalizeProgram();

	std::vector<IVfs::Path> embedded_files;
	embedded_files.reserve( code_builder.embed_files_cache_.size() );
	for( const auto& pair : code_builder.embed_files_cache_ )
		embedded_files.push_back( pair.first );

	return BuildResult{ code_builder.TakeErrors(), std::move(code_builder.module_), std::move(embedded_files) };
}

std::unique_ptr<CodeBuilder> CodeBuilder::BuildProgramAndLeaveInternalState(
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	const CodeBuilderOptions& options,
	const SourceGraphPtr& source_graph,
	IVfsSharedPtr vfs )
{
	std::unique_ptr<CodeBuilder> instance(
		new CodeBuilder(
			llvm_context,
			data_layout,
			target_triple,
			options,
			std::move(vfs) ) );

	instance->BuildProgramInternal( source_graph );
	// Do not finalize program - save some time.

	if( instance->collect_definition_points_ )
		instance->DummyInstantiateTemplates();

	return instance;
}

CodeBuilderErrorsContainer CodeBuilder::TakeErrors()
{
	CodeBuilderErrorsContainer result;
	result.swap( *global_errors_ );
	return result;
}

CodeBuilder::CodeBuilder(
	llvm::LLVMContext& llvm_context,
	const llvm::DataLayout& data_layout,
	const llvm::Triple& target_triple,
	const CodeBuilderOptions& options,
	IVfsSharedPtr vfs )
	: llvm_context_( llvm_context )
	, data_layout_(data_layout)
	, target_triple_(target_triple)
	, build_debug_info_( options.build_debug_info )
	, create_lifetimes_( options.create_lifetimes )
	, generate_lifetime_start_end_debug_calls_( options.generate_lifetime_start_end_debug_calls )
	, generate_tbaa_metadata_( options.generate_tbaa_metadata )
	, report_about_unused_names_( options.report_about_unused_names )
	, collect_definition_points_( options.collect_definition_points )
	, skip_building_generated_functions_( options.skip_building_generated_functions )
	, vfs_( std::move(vfs) )
	, constexpr_function_evaluator_( data_layout_ )
	, mangler_( CreateMangler( options.mangling_scheme, data_layout_ ) )
	, tbaa_metadata_builder_( llvm_context_, data_layout, mangler_ )
{
	fundamental_llvm_types_.i8_  = llvm::Type::getInt8Ty  ( llvm_context_ );
	fundamental_llvm_types_.u8_  = llvm::Type::getInt8Ty  ( llvm_context_ );
	fundamental_llvm_types_.i16_ = llvm::Type::getInt16Ty ( llvm_context_ );
	fundamental_llvm_types_.u16_ = llvm::Type::getInt16Ty ( llvm_context_ );
	fundamental_llvm_types_.i32_ = llvm::Type::getInt32Ty ( llvm_context_ );
	fundamental_llvm_types_.u32_ = llvm::Type::getInt32Ty ( llvm_context_ );
	fundamental_llvm_types_.i64_ = llvm::Type::getInt64Ty ( llvm_context_ );
	fundamental_llvm_types_.u64_ = llvm::Type::getInt64Ty ( llvm_context_ );
	fundamental_llvm_types_.i128_= llvm::Type::getInt128Ty( llvm_context_ );
	fundamental_llvm_types_.u128_= llvm::Type::getInt128Ty( llvm_context_ );

	fundamental_llvm_types_.f32_= llvm::Type::getFloatTy( llvm_context_ );
	fundamental_llvm_types_.f64_= llvm::Type::getDoubleTy( llvm_context_ );

	fundamental_llvm_types_.char8_ = llvm::Type::getInt8Ty ( llvm_context_ );
	fundamental_llvm_types_.char16_= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.char32_= llvm::Type::getInt32Ty( llvm_context_ );

	fundamental_llvm_types_.byte8_  = llvm::Type::getInt8Ty  ( llvm_context_ );
	fundamental_llvm_types_.byte16_ = llvm::Type::getInt16Ty ( llvm_context_ );
	fundamental_llvm_types_.byte32_ = llvm::Type::getInt32Ty ( llvm_context_ );
	fundamental_llvm_types_.byte64_ = llvm::Type::getInt64Ty ( llvm_context_ );
	fundamental_llvm_types_.byte128_= llvm::Type::getInt128Ty( llvm_context_ );

	fundamental_llvm_types_.invalid_type_= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.void_for_ret_= llvm::Type::getVoidTy( llvm_context_ );
	fundamental_llvm_types_.bool_= llvm::Type::getInt1Ty( llvm_context_ );

	// Use empty named structure for "void" type.
	fundamental_llvm_types_.void_= llvm::StructType::create( llvm_context_, {}, "__U_void" );

	fundamental_llvm_types_.ssize_type_= data_layout_.getIntPtrType(llvm_context_);
	fundamental_llvm_types_.size_type_ = data_layout_.getIntPtrType(llvm_context_);

	invalid_type_= FundamentalType( U_FundamentalType::InvalidType, fundamental_llvm_types_.invalid_type_ );
	void_type_= FundamentalType( U_FundamentalType::void_, fundamental_llvm_types_.void_ );
	bool_type_= FundamentalType( U_FundamentalType::bool_, fundamental_llvm_types_.bool_ );
	ssize_type_= FundamentalType( U_FundamentalType::ssize_type_, fundamental_llvm_types_.ssize_type_ );
	size_type_ = FundamentalType( U_FundamentalType::size_type_ , fundamental_llvm_types_.size_type_  );

	{
		// A pair of chars.
		// First - number of param from '0' up to '9'.
		// Second - '_' for reference param or letters from 'a' up to 'z' for inner reference tags.
		ArrayType a;
		a.element_type= FundamentalType( U_FundamentalType::char8_, fundamental_llvm_types_.char8_ );
		a.element_count= 2;
		a.llvm_type= llvm::ArrayType::get( a.element_type.GetLLVMType(), a.element_count );
		reference_notation_param_reference_description_type_= std::move(a);
	}
	{
		// A pair of reference param descriptions. First - destination, second - source.
		ArrayType a;
		a.element_type= reference_notation_param_reference_description_type_;
		a.element_count= 2;
		a.llvm_type= llvm::ArrayType::get( a.element_type.GetLLVMType(), a.element_count );
		reference_notation_pollution_element_type_= std::move(a);
	}

	virtual_function_pointer_type_= llvm::PointerType::get( llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret_, true ), 0u );

	// Use named struct for polymorph type id table element, because this is recursive struct.
	{
		polymorph_type_id_table_element_type_= llvm::StructType::create( llvm_context_, "__U_polymorph_type_id_table_element" );
		llvm::Type* const elements[]
		{
			fundamental_llvm_types_.size_type_, // Parent class offset.
			polymorph_type_id_table_element_type_->getPointerTo(), // Pointer to parent class type_id table.
		};
		polymorph_type_id_table_element_type_->setBody( elements );
	}
}

void CodeBuilder::BuildProgramInternal( const SourceGraphPtr& source_graph )
{
	source_graph_= source_graph;
	macro_expansion_contexts_= source_graph->macro_expansion_contexts;

	U_ASSERT( module_ == nullptr );
	module_=
		std::make_unique<llvm::Module>(
			source_graph->nodes_storage.front().file_path,
			llvm_context_ );

	// Setup data layout and target triple.
	module_->setDataLayout(data_layout_);
	module_->setTargetTriple(target_triple_.normalize());

	// Prepare halt func.
	halt_func_=
		llvm::Function::Create(
			llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret_, false ),
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
		llvm::Type* const arg_types[1]= { fundamental_llvm_types_.u8_->getPointerTo() };

		lifetime_start_debug_func_=
			llvm::Function::Create(
				llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret_, arg_types, false ),
				llvm::Function::ExternalLinkage,
				"__U_debug_lifetime_start",
				module_.get() );

		lifetime_end_debug_func_=
			llvm::Function::Create(
				llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret_, arg_types, false ),
				llvm::Function::ExternalLinkage,
				"__U_debug_lifetime_end",
				module_.get() );
	}

	// Prepare heap allocation functions.
	{
		// Use some temporary names for allocation functions.
		// Do this in order to avoid collisions with user-defined names.
		llvm::Type* const ptr_type= llvm::PointerType::get( llvm_context_, 0 );
		malloc_func_=
			llvm::Function::Create(
					llvm::FunctionType::get(
						ptr_type,
						{ fundamental_llvm_types_.size_type_ },
						false ),
					llvm::Function::ExternalLinkage,
					"__U_ust_memory_allocate_impl",
					module_.get() );

		free_func_=
			llvm::Function::Create(
					llvm::FunctionType::get(
						fundamental_llvm_types_.void_for_ret_,
						{ ptr_type },
						false ),
					llvm::Function::ExternalLinkage,
					"__U_ust_memory_free_impl",
					module_.get() );
	}

	FunctionType global_function_type;
	global_function_type.return_type= void_type_;

	// In some places outside functions we need to execute expression evaluation.
	// Create for this function context.
	llvm::Function* const global_function=
		llvm::Function::Create(
			GetLLVMFunctionType( global_function_type ),
			llvm::Function::LinkageTypes::PrivateLinkage,
			"",
			module_.get() );

	global_function_context_=
		std::make_unique<FunctionContext> (
			std::move(global_function_type),
			llvm_context_,
			global_function );
	global_function_context_->is_functionless_context= true;
	global_function_context_variables_storage_= std::make_unique<StackVariablesStorage>( *global_function_context_ );

	debug_info_builder_.emplace( llvm_context_, data_layout_, *source_graph, *module_, build_debug_info_ );

	// Build graph.
	compiled_sources_.resize( source_graph->nodes_storage.size() );
	BuildSourceGraphNode( *source_graph, 0u );

	// Perform post-checks for non_sync tags.
	// Do this at the end to avoid dependency loops.
	// Use index-for since classes table may be extended during iteration.
	for( size_t i= 0; i < classes_table_.size(); ++i )
	{
		const ClassPtr class_type= classes_table_[i].get();
		CheckClassNonSyncTagExpression( class_type );
		CheckClassNonSyncTagInheritance( class_type );
	}

	// Check for unused names in root file.
	CheckForUnusedGlobalNames( *compiled_sources_[0].names_map );

	// Leave internal structures intact.

	// Normalize result errors.
	*global_errors_= NormalizeErrors( *global_errors_, *source_graph->macro_expansion_contexts );
}

void CodeBuilder::FinalizeProgram()
{
	// Finalize "defererenceable" attributes.
	// Do this at end because we needs complete types for params/return values even for only prototypes.
	SetupDereferenceableFunctionParamsAndRetAttributes_r( *compiled_sources_.front().names_map );
	for( auto& key_value_pair : generated_template_things_storage_ )
		SetupDereferenceableFunctionParamsAndRetAttributes_r( *key_value_pair.second );

	global_function_context_->function->eraseFromParent(); // Kill global function.

	// Fix incomplete typeinfo.
	for( const auto& typeinfo_entry : typeinfo_cache_ )
	{
		if( !typeinfo_entry.second.variable->type.GetLLVMType()->isSized() )
			typeinfo_entry.second.variable->type.GetClassType()->llvm_type->setBody( llvm::ArrayRef<llvm::Type*>() );
	}

	// Replace usage of temporary allocation functions with usage of library allocation functions.
	// Do such, because we can't just declare internal functions with such names, prior to compiling file with such functions declarations ("alloc.u").
	// Without such approach functions, declared in library file, get suffix, like "ust_memory_allocate_impl.1".
	{
		if( const auto ust_memory_allocate_impl= module_->getFunction( "ust_memory_allocate_impl" ) )
			malloc_func_->replaceAllUsesWith( ust_memory_allocate_impl );
		else
			malloc_func_->setName( "ust_memory_allocate_impl" );

		if( const auto ust_memory_free_impl= module_->getFunction( "ust_memory_free_impl" ) )
			free_func_->replaceAllUsesWith( ust_memory_free_impl );
		else
			free_func_->setName( "ust_memory_free_impl" );
	}

	// Reset debug info builder in order to finish deffered debug info construction.
	debug_info_builder_= std::nullopt;
}

void CodeBuilder::BuildSourceGraphNode( const SourceGraph& source_graph, const size_t node_index )
{
	SourceBuildResult& result= compiled_sources_[ node_index ];
	if( result.names_map != nullptr )
		return;

	const SourceGraph::Node& source_graph_node= source_graph.nodes_storage[ node_index ];

	result.names_map= std::make_unique<NamesScope>( "", nullptr );
	result.names_map->SetErrors( global_errors_ );

	if( source_graph_node.child_nodes_indices.empty() )
	{
		// Fill global names scope only for files with no imports.
		// Files with imports should inherit contents of global namespace from imported files.
		FillGlobalNamesScope( *result.names_map );
	}

	U_ASSERT( node_index < source_graph.nodes_storage.size() );

	// Build dependent nodes.
	for( const size_t child_node_inex : source_graph_node.child_nodes_indices )
		BuildSourceGraphNode( source_graph, child_node_inex );

	// Merge namespaces of imported files into one.
	for( const size_t child_node_inex : source_graph_node.child_nodes_indices )
	{
		const SourceBuildResult& child_node_build_result= compiled_sources_[ child_node_inex ];
		MergeNameScopes( *result.names_map, *child_node_build_result.names_map, child_node_build_result.classes_members_namespaces_table );
	}

	// Do work for this node.
	NamesScopeFill( *result.names_map, source_graph_node.ast.program_elements );
	NamesScopeFillOutOfLineElements( *result.names_map, source_graph_node.ast.program_elements );
	ProcessMixins( *result.names_map );
	GlobalThingBuildNamespace( *result.names_map );

	if( !skip_building_generated_functions_ )
	{
		// Finalize building template things.
		// Each new template thing added into this vector, so, by iterating through we will build all template things.
		// It's important to use an index instead of iterators during iteration because this vector may be chaged in process.
		for( size_t i= 0; i < generated_template_things_sequence_.size(); ++i )
		{
			const NamesScopePtr namespace_= generated_template_things_storage_[generated_template_things_sequence_[i]];
			GlobalThingBuildNamespace( *namespace_ );
		}
	}
	generated_template_things_sequence_.clear();

	// Fill result classes members namespaces table.
	for( const auto& class_ptr : classes_table_ )
		result.classes_members_namespaces_table.emplace( class_ptr.get(), class_ptr->members );
}

void CodeBuilder::MergeNameScopes(
	NamesScope& dst,
	const NamesScope& src,
	const ClassesMembersNamespacesTable& src_classes_members_namespaces_table )
{
	src.ForEachInThisScope(
		[&]( const std::string_view src_name, const NamesScopeValue& src_member )
		{
			NamesScopeValue* const dst_member= dst.GetThisScopeValue(src_name );
			if( dst_member == nullptr )
			{
				// All ok - name form "src" does not exists in "dst".
				if( const NamesScopePtr names_scope= src_member.value.GetNamespace() )
				{
					// We copy namespaces, instead of taking same shared pointer,
					// because using same shared pointer we can change state of "src".
					const NamesScopePtr names_scope_copy= std::make_shared<NamesScope>( names_scope->GetThisNamespaceName(), &dst );
					MergeNameScopes( *names_scope_copy, *names_scope, src_classes_members_namespaces_table );
					dst.AddName( src_name, NamesScopeValue( names_scope_copy, src_member.src_loc ) );

					names_scope_copy->CopyAccessRightsFrom( *names_scope );
				}
				else if( const OverloadedFunctionsSetConstPtr functions_set= src_member.value.GetFunctionsSet() )
				{
					// Take copy of value, stored as shared_ptr to avoid modification of source value.
					dst.AddName( src_name, NamesScopeValue( std::make_shared<OverloadedFunctionsSet>(*functions_set), SrcLoc() ) );
				}
				else
				{
					if( const Type* const type= src_member.value.GetTypeName() )
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

			if( dst_member->value.GetKindIndex() != src_member.value.GetKindIndex() )
			{
				// Different kind of symbols - 100% error.
				REPORT_ERROR( Redefinition, dst.GetErrors(), src_member.src_loc, src_name );
				return;
			}

			if( const NamesScopePtr sub_namespace= src_member.value.GetNamespace() )
			{
				// Merge namespaces.
				// TODO - detect here template instantiation namespaces.
				const NamesScopePtr dst_sub_namespace= dst_member->value.GetNamespace();
				U_ASSERT( dst_sub_namespace != nullptr );
				MergeNameScopes( *dst_sub_namespace, *sub_namespace, src_classes_members_namespaces_table );
				return;
			}
			else if( const OverloadedFunctionsSetPtr dst_funcs_set= dst_member->value.GetFunctionsSet() )
			{
				const OverloadedFunctionsSetConstPtr src_funcs_set= src_member.value.GetFunctionsSet();
				U_ASSERT( src_funcs_set != nullptr );

				for( const FunctionVariable& src_func : src_funcs_set->functions )
				{
					FunctionVariable* const same_dst_func= GetFunctionWithSameType( src_func.type, *dst_funcs_set );
					if( same_dst_func != nullptr )
					{
						if( same_dst_func->prototype_src_loc != src_func.prototype_src_loc )
						{
							// Prototypes are in differrent files.
							REPORT_ERROR( FunctionPrototypeDuplication, dst.GetErrors(), src_func.prototype_src_loc, src_name );
							continue;
						}

						if( !same_dst_func->has_body &&  src_func.has_body )
							*same_dst_func= src_func; // Take this function - it has body.
						if(  same_dst_func->has_body && !src_func.has_body )
						{} // Ok, prototype imported later.
						if(  same_dst_func->has_body &&  src_func.has_body &&
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
			else if( const Type* const type= dst_member->value.GetTypeName() )
			{
				const auto dst_class= type->GetClassType();
				const auto src_class= src_member.value.GetTypeName()->GetClassType();
				if( dst_class != src_class )
				{
					// Different pointer value means 100% different classes.
					REPORT_ERROR( Redefinition, dst.GetErrors(), src_member.src_loc, src_name );
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
			else if( const auto dst_type_templates_set= dst_member->value.GetTypeTemplatesSet() )
			{
				if( const auto src_type_templates_set= src_member.value.GetTypeTemplatesSet() )
				{
					if( dst_type_templates_set->type_templates != src_type_templates_set->type_templates )
					{
						// Require defining a set of overloaded type templates in a single file.
						// This is needed in order to prevent possible mangling problems of template types with arguments - overloaded type templates.
						REPORT_ERROR( OverloadingImportedTypeTemplate, dst.GetErrors(), src_type_templates_set->type_templates.front()->syntax_element->src_loc );
					}

					return;
				}
			}
			else if( dst_member->value.GetMixins() != nullptr )
			{
				// Mixins are expanded earlier and no job is required to merging mixins somehow.
				return;
			}

			if( dst_member->src_loc == src_member.src_loc )
				return; // All ok - things from one source.

			// Can not merge other kinds of values.
			REPORT_ERROR( Redefinition, dst.GetErrors(), src_member.src_loc, src_name );
		} );
}

void CodeBuilder::FillGlobalNamesScope( NamesScope& global_names_scope )
{
	const SrcLoc fundamental_globals_src_loc( SrcLoc::c_max_file_index, SrcLoc::c_max_line, SrcLoc::c_max_column );

	for( size_t i= size_t(U_FundamentalType::void_); i < size_t(U_FundamentalType::LastType); ++i )
	{
		const U_FundamentalType fundamental_type= U_FundamentalType(i);
		global_names_scope.AddName(
			GetFundamentalTypeName(fundamental_type),
			NamesScopeValue(
				Type( FundamentalType( fundamental_type, GetFundamentalLLVMType( fundamental_type ) ) ),
				fundamental_globals_src_loc ) );
	}
}

bool CodeBuilder::IsSrcLocFromMainFile( const SrcLoc& src_loc )
{
	if( src_loc.GetFileIndex() == 0 )
		return true; // Definition in main file.

	const auto macro_expansion_index= src_loc.GetMacroExpansionIndex();
	if( macro_expansion_index == SrcLoc::c_max_macro_expanison_index )
		return false; // Not something from macro - definition can't be from main file.

	// If this is a src_loc in macro expansion - check recursively macro expansion point.
	U_ASSERT( macro_expansion_index < macro_expansion_contexts_->size() );
	return IsSrcLocFromMainFile( (*macro_expansion_contexts_)[ macro_expansion_index ].src_loc );
}

void CodeBuilder::TryCallCopyConstructor(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	llvm::Value* const this_, llvm::Value* const src,
	const ClassPtr class_type,
	FunctionContext& function_context )
{
	U_ASSERT( class_type != nullptr );
	const Class& class_= *class_type;

	if( !class_.is_copy_constructible )
	{
		REPORT_ERROR( CopyConstructValueOfNoncopyableType, errors_container, src_loc, class_type );
		return;
	}

	// Search for copy-constructor.
	const NamesScopeValue* const constructos_value= class_.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) );
	U_ASSERT( constructos_value != nullptr );
	const OverloadedFunctionsSetConstPtr constructors= constructos_value->value.GetFunctionsSet();
	U_ASSERT(constructors != nullptr );
	const FunctionVariable* constructor= nullptr;
	for( const FunctionVariable& candidate : constructors->functions )
	{
		if( IsCopyConstructor( candidate.type, class_type ) )
		{
			constructor= &candidate;
			break;
		}
	}

	U_ASSERT(constructor != nullptr);

	if( !( constructor->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete || constructor->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete ) )
		function_context.has_non_constexpr_operations_inside= true;

	// Call it
	if( !function_context.is_functionless_context )
		function_context.llvm_ir_builder.CreateCall( EnsureLLVMFunctionCreated( *constructor ), { this_, src } );
}

void CodeBuilder::GenerateLoop(
	const uint64_t iteration_count,
	const std::function<void(llvm::Value* counter_value)>& loop_body,
	FunctionContext& function_context)
{
	U_ASSERT( loop_body != nullptr );
	if( iteration_count == 0u )
		return;

	const auto size_type_llvm= fundamental_llvm_types_.size_type_;
	llvm::Value* const zero_value= llvm::Constant::getNullValue( size_type_llvm );

	if( function_context.is_functionless_context )
	{
		loop_body( zero_value );
		return;
	}

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
		const VariablePtr& stored_variable= *it;

		if( stored_variable->value_type == ValueType::Value )
		{
			if( !function_context.variables_state.NodeMoved( stored_variable ) )
			{
				if( function_context.variables_state.HasOutgoingLinks( stored_variable ) )
					REPORT_ERROR( DestroyedVariableStillHasReferences, errors_container, src_loc, stored_variable->name );

				if( !function_context.is_functionless_context )
				{
					if( stored_variable->type.HasDestructor() )
						CallDestructor( stored_variable->llvm_value, stored_variable->type, function_context, errors_container, src_loc );

					// Avoid calling "lifetime.end" for variables without address.
					if( stored_variable->location == Variable::Location::Pointer )
						CreateLifetimeEnd( function_context, stored_variable->llvm_value );
				}
			}
		}
		function_context.variables_state.RemoveNode( stored_variable );
	}

	if( !function_context.is_functionless_context )
	{
		// Free memory allocated for "alloca" declarations.
		// It's important to do this in reverse order, since allocations are made mostly by manipulation of the program stack.
		for( auto it = stack_variables_storage.allocas_.rbegin(); it != stack_variables_storage.allocas_.rend(); ++it )
		{
			const AllocaInfo& alloca_info= *it;

			llvm::BasicBlock* const stack_allocation_block= llvm::BasicBlock::Create( llvm_context_, "stack_allocation_free_block" );
			llvm::BasicBlock* const heap_allocation_block= llvm::BasicBlock::Create( llvm_context_, "heap_allocation_free_block" );
			llvm::BasicBlock* const end_block= llvm::BasicBlock::Create( llvm_context_ );

			function_context.llvm_ir_builder.CreateCondBr( alloca_info.is_stack_allocation, stack_allocation_block, heap_allocation_block );

			// Stack allocation block.
			function_context.function->getBasicBlockList().push_back( stack_allocation_block );
			function_context.llvm_ir_builder.SetInsertPoint( stack_allocation_block );
			function_context.llvm_ir_builder.CreateCall(
				llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::stackrestore ),
				{ alloca_info.ptr_for_free } );
			function_context.llvm_ir_builder.CreateBr( end_block );

			// Heap allocation block.
			function_context.function->getBasicBlockList().push_back( heap_allocation_block );
			function_context.llvm_ir_builder.SetInsertPoint( heap_allocation_block );
			function_context.llvm_ir_builder.CreateCall( free_func_, { alloca_info.ptr_for_free } );
			function_context.llvm_ir_builder.CreateBr( end_block );

			// End block.
			function_context.function->getBasicBlockList().push_back( end_block );
			function_context.llvm_ir_builder.SetInsertPoint( end_block );
		}
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
	U_ASSERT( type.HasDestructor() );

	if( const Class* const class_= type.GetClassType() )
	{
		const NamesScopeValue* const destructor_value= class_->members->GetThisScopeValue( Keyword( Keywords::destructor_ ) );
		U_ASSERT( destructor_value != nullptr );
		const OverloadedFunctionsSetPtr destructors= destructor_value->value.GetFunctionsSet();
		PrepareFunctionsSetAndBuildConstexprBodies( *class_->members, *destructors );
		U_ASSERT(destructors != nullptr && destructors->functions.size() == 1u );

		const FunctionVariable& destructor= destructors->functions.front();
		function_context.llvm_ir_builder.CreateCall( EnsureLLVMFunctionCreated( destructor ), { ptr } );
	}
	else if( const ArrayType* const array_type= type.GetArrayType() )
	{
		// SPRACHE_TODO - maybe call destructors of arrays in reverse order?
		GenerateLoop(
			array_type->element_count,
			[&]( llvm::Value* const index )
			{
				CallDestructor(
					CreateArrayElementGEP( function_context, *array_type, ptr, index ),
					array_type->element_type,
					function_context,
					errors_container,
					src_loc );
			},
			function_context );
	}
	else if( const TupleType* const tuple_type= type.GetTupleType() )
	{
		for( const Type& element_type : tuple_type->element_types )
		{
			if( element_type.HasDestructor() )
				CallDestructor(
					CreateTupleElementGEP( function_context, *tuple_type, ptr, size_t(&element_type - tuple_type->element_types.data()) ),
					element_type,
					function_context,
					errors_container,
					src_loc );
		}
	}
	else U_ASSERT(false);
}

void CodeBuilder::CallDestructorsForLoopInnerVariables( NamesScope& names_scope, FunctionContext& function_context, const size_t stack_variables_stack_size, const SrcLoc& src_loc )
{
	U_ASSERT( !function_context.loops_stack.empty() );

	// Destroy all local variables before "break"/"continue" in all blocks inside loop.
	size_t undestructed_stack_size= function_context.stack_variables_stack.size();
	for(
		auto it= function_context.stack_variables_stack.rbegin();
		it != function_context.stack_variables_stack.rend() &&
		undestructed_stack_size > stack_variables_stack_size;
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
		U_ASSERT( class_->parents[i].class_->has_destructor ); // Parents are polymorph, polymorph classes always have destructors.
		CallDestructor(
			CreateClassFieldGEP( function_context, *function_context.this_, class_->parents[i].field_number ),
			class_->parents[i].class_,
			function_context,
			errors_container,
			src_loc );
	}

	for( const ClassFieldPtr& field : class_->fields_order )
	{
		if( field == nullptr )
			continue;

		if( !field->type.HasDestructor() || field->is_reference )
			continue;

		CallDestructor(
			CreateClassFieldGEP( function_context, *function_context.this_, field->index ),
			field->type,
			function_context,
			errors_container,
			src_loc );
	};
}

void CodeBuilder::CheckForUnusedGlobalNames( const NamesScope& names_scope )
{
	if( !report_about_unused_names_ )
		return;

	CheckForUnusedGlobalNamesImpl( names_scope );
}

void CodeBuilder::CheckForUnusedGlobalNamesImpl( const NamesScope& names_scope )
{
	names_scope.ForEachInThisScope(
		[&]( const std::string_view name, const NamesScopeValue& names_scope_value )
		{
			const Value& value= names_scope_value.value;
			if( const auto functions_set= value.GetFunctionsSet() )
			{
				// Process each function individually.
				for( const FunctionVariable& function : functions_set->functions )
				{
					if( !function.referenced &&
						!function.no_mangle &&
						function.virtual_table_index == ~0u &&
						IsSrcLocFromMainFile( function.body_src_loc ) &&
						IsSrcLocFromMainFile( function.prototype_src_loc ) )
					{
						bool is_special_method= false;
						if( functions_set->base_class != nullptr )
						{
							if( name == Keyword( Keywords::destructor_ ) ||
								( name == Keyword( Keywords::constructor_ ) &&
									( IsCopyConstructor( function.type, functions_set->base_class ) || IsDefaultConstructor( function.type, functions_set->base_class ) ) ) ||
								( name == OverloadedOperatorToString( OverloadedOperator::Assign ) && IsCopyAssignmentOperator( function.type, functions_set->base_class ) ) ||
								( name == OverloadedOperatorToString( OverloadedOperator::CompareEqual ) && IsEqualityCompareOperator( function.type, functions_set->base_class ) ) )
								is_special_method= true;
						}

						// Report about unused function, only if it is defined in main file, has no prototype in one of imported files, is not "nomangle" and (obviously) not referenced.
						// Ignore special methods.
						if( !is_special_method )
							REPORT_ERROR( UnusedName, names_scope.GetErrors(), function.body_src_loc, name );
					}
				}
				// TODO - process function templates here.

				return;
			}
			if( const auto type_templates_set= value.GetTypeTemplatesSet() )
			{
				// Process each type template individually.
				for( const TypeTemplatePtr& type_template : type_templates_set->type_templates )
					if( !type_template->used && IsSrcLocFromMainFile( type_template->src_loc ) )
						REPORT_ERROR( UnusedName, names_scope.GetErrors(), type_template->src_loc, name );

				return;
			}

			// Check namespace of classes, but only for place where this class was defined.
			if( const auto type= value.GetTypeName() )
			{
				if( const auto class_type= type->GetClassType() )
				{
					if( class_type->members->GetParent() == &names_scope )
						CheckForUnusedGlobalNamesImpl( *class_type->members );
				}
			}

			if( names_scope_value.referenced )
				return; // Value is referenced.
			if( !IsSrcLocFromMainFile( names_scope_value.src_loc ) )
				return; // Ignore imported names.

			if( value.GetVariable() != nullptr )
			{
				// All global variables/references have trivial destructor.
				// So, there is a reason to report error about all unreferenced global variables/references.
				REPORT_ERROR( UnusedName, names_scope.GetErrors(), names_scope_value.src_loc, name );
			}
			else if(
				value.GetTypeName() != nullptr ||
				value.GetTypeAlias() != nullptr )
			{
				REPORT_ERROR( UnusedName, names_scope.GetErrors(), names_scope_value.src_loc, name );
			}
			else if( value.GetClassField() != nullptr )
			{
				REPORT_ERROR( UnusedName, names_scope.GetErrors(), names_scope_value.src_loc, name );
			}
			else if( value.GetThisOverloadedMethodsSet() != nullptr )
			{
				// NamesScope can't contain this.
				U_ASSERT(false);
			}
			else if( const auto namespace_= value.GetNamespace() )
				CheckForUnusedGlobalNamesImpl( *namespace_ ); // Recursively check children.
			else if(
				value.GetStaticAssert() != nullptr ||
				value.GetIncompleteGlobalVariable() != nullptr ||
				value.GetMixins() != nullptr ||
				value.GetYetNotDeducedTemplateArg() != nullptr ||
				value.GetErrorValue() != nullptr )
			{} // Ignore these kinds if values.
			else U_ASSERT(false);
		} );
}

void CodeBuilder::CheckForUnusedLocalNames( const NamesScope& names_scope )
{
	if( !report_about_unused_names_ )
		return;

	names_scope.ForEachInThisScope(
		[&]( const std::string_view name, const NamesScopeValue& names_scope_value )
		{
			if( names_scope_value.referenced )
				return; // Value is referenced.

			const Value& value= names_scope_value.value;
			if( value.GetVariable() != nullptr )
			{
				// Variable with side-effects of their existence should be marked as referenced before.
				REPORT_ERROR( UnusedName, names_scope.GetErrors(), names_scope_value.src_loc, name );
			}
			else if( value.GetTypeName() != nullptr || value.GetTypeAlias() != nullptr )
			{
				REPORT_ERROR( UnusedName, names_scope.GetErrors(), names_scope_value.src_loc, name );
			}
			else if(
				value.GetNamespace() != nullptr ||
				value.GetTypeTemplatesSet() ||
				value.GetThisOverloadedMethodsSet() != nullptr ||
				value.GetClassField() ||
				value.GetFunctionsSet() )
			{
				// Local NamesScope can't contain this.
				U_ASSERT(false);
			}
			else if(
				value.GetStaticAssert() != nullptr ||
				value.GetIncompleteGlobalVariable() != nullptr ||
				value.GetMixins() != nullptr ||
				value.GetYetNotDeducedTemplateArg() != nullptr ||
				value.GetErrorValue() != nullptr )
			{} // Ignore these kinds if values.
			else U_ASSERT(false);
		} );
}

bool CodeBuilder::VariableExistanceMayHaveSideEffects( const Type& variable_type )
{
	// Normally we should perform deep inspection in order to know, that existance of the variable has sence.
	// For example, "ust::string8" has non-trivial destructor, but it just frees memory.
	// But such check is too hard to implement, so, assume, that only variables of types with trivial (no-op) destructor may be considered unused.
	const bool destructor_is_trivial=
		// Constexpr types are fundamentals, enums, function pointers, some structs.
		variable_type.CanBeConstexpr() ||
		// Raw pointers are non-constexpr, but trivially-destructible.
		variable_type.GetRawPointerType() != nullptr;

	return !destructor_is_trivial;
}

size_t CodeBuilder::PrepareFunction(
	NamesScope& names_scope,
	const ClassPtr base_class,
	OverloadedFunctionsSet& functions_set,
	const Synt::Function& func,
	const bool is_out_of_line_function )
{
	const std::string& func_name= func.name.back().name;
	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	const bool is_special_method= is_constructor || is_destructor;

	if( is_special_method )
		U_ASSERT( func.type.params.size() >= 1u && func.type.params.front().name == Keywords::this_ );

	if( !is_special_method && IsKeyword( func_name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), func.src_loc );

	if( is_special_method && base_class == nullptr )
	{
		REPORT_ERROR( ConstructorOrDestructorOutsideClass, names_scope.GetErrors(), func.src_loc );
		return ~0u;
	}
	if( !is_constructor && func.constructor_initialization_list != nullptr )
	{
		REPORT_ERROR( InitializationListInNonConstructor, names_scope.GetErrors(), func.constructor_initialization_list->src_loc );
		return ~0u;
	}
	if( is_destructor && func.type.params.size() >= 2u )
	{
		REPORT_ERROR( ExplicitArgumentsInDestructor, names_scope.GetErrors(), func.src_loc );
		return ~0u;
	}

	if( !std::holds_alternative<Synt::EmptyVariant>( func.condition ) )
	{
		const bool res= EvaluateBoolConstantExpression( names_scope, *global_function_context_, func.condition );
		global_function_context_->args_preevaluation_cache.clear();
		if( !res )
			return ~0u;
	}


	FunctionVariable func_variable;

	func_variable.kind= func.kind;

	{ // Prepare and process function type.

		if( !func.type.params.empty() && func.type.params.front().name == Keywords::this_ )
			func_variable.is_this_call= true;

		if( func.type.IsAutoReturn() )
		{
			if( func.block == nullptr )
				REPORT_ERROR( ExpectedBodyForAutoFunction, names_scope.GetErrors(), func.src_loc, func_name );

			if( is_special_method ||
				func_name == OverloadedOperatorToString( OverloadedOperator::Assign ) ||
				func_name == OverloadedOperatorToString( OverloadedOperator::CompareEqual ) )
				REPORT_ERROR( AutoForSpecialMethod, names_scope.GetErrors(), func.src_loc );

			if( func.virtual_function_kind != Synt::VirtualFunctionKind::None )
				REPORT_ERROR( AutoForVirtualMethod, names_scope.GetErrors(), func.src_loc );

			if( func.type.return_value_reference_expression != nullptr )
				REPORT_ERROR( ReferenceNotationForAutoFunction, names_scope.GetErrors(), Synt::GetSrcLoc( *func.type.return_value_reference_expression ) );
			if( func.type.return_value_inner_references_expression != nullptr )
				REPORT_ERROR( ReferenceNotationForAutoFunction, names_scope.GetErrors(), Synt::GetSrcLoc( *func.type.return_value_inner_references_expression ) );
			if( func.type.references_pollution_expression != nullptr )
				REPORT_ERROR( ReferenceNotationForAutoFunction, names_scope.GetErrors(), Synt::GetSrcLoc( *func.type.references_pollution_expression ) );
		}

		FunctionType function_type= PrepareFunctionType( names_scope, *global_function_context_, func.type, base_class );
		global_function_context_->args_preevaluation_cache.clear();

		if( is_special_method && !( function_type.return_type == void_type_ && function_type.return_value_type == ValueType::Value ) )
		{
			REPORT_ERROR( ConstructorAndDestructorMustReturnVoid, names_scope.GetErrors(), func.src_loc );
			function_type.return_type= void_type_;
			function_type.return_value_type= ValueType::Value;
		}

		if( is_special_method && !function_type.params.empty() && function_type.params.front().value_type == ValueType::Value )
		{
			REPORT_ERROR( ByvalThisForConstructorOrDestructor, names_scope.GetErrors(), func.src_loc );
			function_type.params.front().value_type= ValueType::ReferenceMut;
		}

		if (function_type.unsafe && base_class != nullptr )
		{
			// Calls to such methods are produced by compiler itself, used in other methods generation, etc.
			// So, to avoid problems with generated unsafe calls just forbid some methods to be unsafe.
			if( is_destructor ||
				( is_constructor && ( IsDefaultConstructor( function_type, base_class ) || IsCopyConstructor( function_type, base_class ) ) ) ||
				( func.overloaded_operator == OverloadedOperator::Assign && IsCopyAssignmentOperator( function_type, base_class ) ) ||
				( func.overloaded_operator == OverloadedOperator::CompareEqual && IsEqualityCompareOperator( function_type, base_class ) ) )
			{
				REPORT_ERROR( ThisMethodCanNotBeUnsafe, names_scope.GetErrors(), func.src_loc );
				function_type.unsafe= false;
			}
		}

		// Disable non-default calling conventions for this-call methods and operators because of problems with call of generated methods/operators.
		// But it's fine to use custom calling convention for static methods.
		if( function_type.calling_convention != llvm::CallingConv::C &&
			( func_variable.is_this_call || func.overloaded_operator != OverloadedOperator::None ) )
			REPORT_ERROR( NonDefaultCallingConventionForClassMethod, names_scope.GetErrors(), func.src_loc );

		if( func_variable.IsCoroutine() )
		{
			PerformCoroutineFunctionReferenceNotationChecks( function_type, names_scope.GetErrors(), func.src_loc );

			TransformCoroutineFunctionType(
				names_scope,
				function_type,
				func_variable.kind,
				ImmediateEvaluateNonSyncTag( names_scope, *global_function_context_, func.coroutine_non_sync_tag ) );
			global_function_context_->args_preevaluation_cache.clear();

			// Disable auto-coroutines.
			if( func.type.IsAutoReturn() )
			{
				REPORT_ERROR( AutoReturnCoroutine, names_scope.GetErrors(), func.type.src_loc );
				func_variable.kind= FunctionVariable::Kind::Regular;
			}

			// Disable coroutines special methods.
			if( is_special_method )
			{
				REPORT_ERROR( CoroutineSpecialMethod, names_scope.GetErrors(), func.type.src_loc );
				func_variable.kind= FunctionVariable::Kind::Regular;
			}

			// Disable references pollution for coroutine. It is too complicated for now.
			if( func.type.references_pollution_expression != nullptr )
				REPORT_ERROR( NotImplemented, names_scope.GetErrors(), func.type.src_loc, "References pollution for coroutines." );

			if( function_type.calling_convention != llvm::CallingConv::C )
				REPORT_ERROR( NonDefaultCallingConventionForCoroutine, names_scope.GetErrors(), func.type.src_loc );

			// It is too complicated to support virtual coroutines. It is simplier to just forbid such coroutines.
			// But this is still possible to return a coroutine value from virtual function.
			if( func.virtual_function_kind != Synt::VirtualFunctionKind::None )
				REPORT_ERROR( VirtualCoroutine, names_scope.GetErrors(), func.type.src_loc );
		}

		ProcessFunctionReferencesPollution( names_scope.GetErrors(), func, function_type, base_class );
		CheckOverloadedOperator( base_class, function_type, func.overloaded_operator, names_scope.GetErrors(), func.src_loc );

		func_variable.type= std::move(function_type);
	} // end prepare function type.

	// Set constexpr.
	if( func.constexpr_ )
	{
		if( func.block == nullptr )
			REPORT_ERROR( ConstexprFunctionsMustHaveBody, names_scope.GetErrors(), func.src_loc );
		if( func.virtual_function_kind != Synt::VirtualFunctionKind::None )
			REPORT_ERROR( ConstexprFunctionCanNotBeVirtual, names_scope.GetErrors(), func.src_loc );

		func_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprIncomplete;
	}

	// Set virtual.
	if( func.virtual_function_kind != Synt::VirtualFunctionKind::None )
	{
		if( base_class == nullptr )
			REPORT_ERROR( VirtualForNonclassFunction, names_scope.GetErrors(), func.src_loc, func_name );
		if( !func_variable.is_this_call )
			REPORT_ERROR( VirtualForNonThisCallFunction, names_scope.GetErrors(), func.src_loc, func_name );
		if( is_constructor )
			REPORT_ERROR( FunctionCanNotBeVirtual, names_scope.GetErrors(), func.src_loc, func_name );
		if( base_class != nullptr && ( base_class->kind == Class::Kind::Struct || base_class->kind == Class::Kind::NonPolymorph ) )
			REPORT_ERROR( VirtualForNonpolymorphClass, names_scope.GetErrors(), func.src_loc, func_name );
		if( is_out_of_line_function )
			REPORT_ERROR( VirtualForFunctionImplementation, names_scope.GetErrors(), func.src_loc, func_name );
		if( func_variable.is_this_call && !func_variable.type.params.empty() && func_variable.type.params.front().value_type == ValueType::Value )
			REPORT_ERROR( VirtualForByvalThisFunction, names_scope.GetErrors(), func.src_loc, func_name );

		func_variable.virtual_function_kind= func.virtual_function_kind;
	}

	// Set no_mangle
	if( func.no_mangle )
	{
		// Allow only global no-mangle function. This prevents existing of multiple "nomangle" functions with same name in different namespaces.
		// If function is operator, it can not be global.
		if( names_scope.GetParent() != nullptr )
			REPORT_ERROR( NoMangleForNonglobalFunction, names_scope.GetErrors(), func.src_loc, func_name );
		func_variable.no_mangle= true;
	}

	// Set conversion constructor.
	func_variable.is_conversion_constructor= func.is_conversion_constructor;
	U_ASSERT( !( func.is_conversion_constructor && !is_constructor ) );
	func_variable.is_constructor= is_constructor;

	if( func.is_conversion_constructor )
	{
		if( func_variable.type.params.size() != 2u )
			REPORT_ERROR( ConversionConstructorMustHaveOneArgument, names_scope.GetErrors(), func.src_loc );
		else if( func_variable.type.params[1].type == func_variable.type.params[0].type )
			REPORT_ERROR( ConversionConstructorSourceTypeIsIdenticalToDestinationType, names_scope.GetErrors(), func.src_loc );
	}

	// Check "=default" / "=delete".
	if( func.body_kind != Synt::Function::BodyKind::None )
	{
		U_ASSERT( func.block == nullptr );
		const FunctionType& function_type= func_variable.type;

		bool invalid_func= false;
		if( base_class == nullptr )
			invalid_func= true;
		else if( is_constructor )
			invalid_func= !( IsDefaultConstructor( function_type, base_class ) || IsCopyConstructor( function_type, base_class ) );
		else if( func.overloaded_operator == OverloadedOperator::Assign )
			invalid_func= !IsCopyAssignmentOperator( function_type, base_class );
		else if( func.overloaded_operator == OverloadedOperator::CompareEqual )
			invalid_func= !IsEqualityCompareOperator( function_type, base_class );
		else
			invalid_func= true;

		if( invalid_func )
			REPORT_ERROR( InvalidMethodForBodyGeneration, names_scope.GetErrors(), func.src_loc );
		else
		{
			if( func.body_kind == Synt::Function::BodyKind::BodyGenerationRequired )
				func_variable.is_generated= true;
			else
				func_variable.is_deleted= true;
		}
	}

	if( FunctionVariable* const prev_function= GetFunctionWithSameType( func_variable.type, functions_set ) )
	{
			 if( prev_function->syntax_element->block == nullptr && func.block != nullptr )
		{ // Ok, body after prototype.
			prev_function->syntax_element= &func;
			prev_function->body_src_loc= func.src_loc;
			CollectFunctionDefinition( *prev_function, prev_function->prototype_src_loc );
		}
		else if( prev_function->syntax_element->block != nullptr && func.block == nullptr )
		{ // Ok, prototype after body. Since order-independent resolving this is correct.
			prev_function->prototype_src_loc= func.src_loc;
			CollectFunctionDefinition( *prev_function, prev_function->prototype_src_loc );
		}
		else if( prev_function->syntax_element->block == nullptr && func.block == nullptr )
			REPORT_ERROR( FunctionPrototypeDuplication, names_scope.GetErrors(), func.src_loc, func_name );
		else if( prev_function->syntax_element->block != nullptr && func.block != nullptr )
			REPORT_ERROR( FunctionBodyDuplication, names_scope.GetErrors(), func.src_loc, func_name );

		if( prev_function->is_this_call != func_variable.is_this_call )
			REPORT_ERROR( ThiscallMismatch, names_scope.GetErrors(), func.src_loc, func_name );

		if( !is_out_of_line_function )
		{
			if( prev_function->virtual_function_kind != func.virtual_function_kind )
				REPORT_ERROR( VirtualMismatch, names_scope.GetErrors(), func.src_loc, func_name );
		}
		if( prev_function->is_deleted != func_variable.is_deleted )
			REPORT_ERROR( BodyForDeletedFunction, names_scope.GetErrors(), prev_function->prototype_src_loc, func_name );
		if( prev_function->is_generated != func_variable.is_generated )
			REPORT_ERROR( BodyForGeneratedFunction, names_scope.GetErrors(), prev_function->prototype_src_loc, func_name );

		if( prev_function->no_mangle != func_variable.no_mangle )
			REPORT_ERROR( NoMangleMismatch, names_scope.GetErrors(), func.src_loc, func_name );

		if( prev_function->kind != func_variable.kind )
			REPORT_ERROR( CoroutineMismatch, names_scope.GetErrors(), func.src_loc, func_name );

		if( prev_function->is_conversion_constructor != func_variable.is_conversion_constructor )
			REPORT_ERROR( CouldNotOverloadFunction, names_scope.GetErrors(), func.src_loc );

		if( prev_function->is_inherited )
			REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), func.src_loc );

		return size_t(prev_function - functions_set.functions.data());
	}
	else
	{
		if( is_out_of_line_function )
		{
			REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), func.src_loc );
			return ~0u;
		}
		if( functions_set.has_nomangle_function || ( !functions_set.functions.empty() && func_variable.no_mangle ) )
		{
			REPORT_ERROR( CouldNotOverloadFunction, names_scope.GetErrors(), func.src_loc );
			return ~0u;
		}

		const bool overloading_ok= ApplyOverloadedFunction( functions_set, func_variable, names_scope.GetErrors(), func.src_loc );
		if( !overloading_ok )
			return ~0u;

		if( func_variable.no_mangle )
			functions_set.has_nomangle_function= true;

		FunctionVariable& inserted_func_variable= functions_set.functions.back();
		inserted_func_variable.body_src_loc= inserted_func_variable.prototype_src_loc= func.src_loc;
		inserted_func_variable.syntax_element= &func;

		inserted_func_variable.llvm_function=
			std::make_shared<LazyLLVMFunction>(
				inserted_func_variable.no_mangle
					? func_name
					: mangler_->MangleFunction( names_scope, func_name, inserted_func_variable.type ) );

		return functions_set.functions.size() - 1u;
	}
}

void CodeBuilder::CheckOverloadedOperator(
	const ClassPtr base_class,
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
	for( const FunctionType::Param& param : func_type.params )
	{
		if( param.type == base_class )
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
				func_type.return_type.GetFundamentalType()->fundamental_type == U_FundamentalType::i32_ &&
				func_type.return_value_type == ValueType::Value ) )
			REPORT_ERROR( InvalidReturnTypeForOperator, errors_container, src_loc, GetFundamentalTypeName( U_FundamentalType::i32_ ) );
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

	case OverloadedOperator::Assign:
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
		if( !func_type.params.empty() && func_type.params.front().value_type != ValueType::ReferenceMut )
			REPORT_ERROR( InvalidFirstParamValueTypeForAssignmentLikeOperator, errors_container, src_loc );
		break;

	case OverloadedOperator::LogicalNot:
	case OverloadedOperator::BitwiseNot:
		if( func_type.params.size() != 1u )
			REPORT_ERROR( InvalidArgumentCountForOperator, errors_container, src_loc );
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

void CodeBuilder::BuildFuncCode(
	FunctionVariable& func_variable,
	const ClassPtr base_class,
	NamesScope& parent_names_scope,
	const std::string_view func_name,
	ReturnTypeDeductionContext* const return_type_deduction_context,
	ReferenceNotationDeductionContext* const reference_notation_deduction_context,
	LambdaPreprocessingContext* const lambda_preprocessing_context )
{
	U_ASSERT( !func_variable.has_body );
	func_variable.has_body= true;

	const FunctionType& function_type= func_variable.type;

	U_ASSERT( func_variable.syntax_element != nullptr );
	const Synt::Function& syntax_element= *func_variable.syntax_element;

	const auto& params= func_variable.syntax_element->type.params;

	U_ASSERT( syntax_element.block != nullptr );
	const Synt::Block& block= *syntax_element.block;

	llvm::Function* const llvm_function= EnsureLLVMFunctionCreated( func_variable );

	// Mark this function specially in order to prevent its execution in constexpr context during its building.
	llvm_function->setMetadata( "__U_incomplete_function_marker", llvm::MDNode::get( llvm_context_, {} ) );

	// Build debug info only for functions with body.
	debug_info_builder_->CreateFunctionInfo( func_variable, func_name );

	if( parent_names_scope.IsInsideTemplate() )
	{
		// Set private visibility for functions inside templates.
		// There is no need to use external linkage, since each user of the template must import file with source template.
		llvm_function->setLinkage( llvm::GlobalValue::PrivateLinkage );
	}
	else if( !IsSrcLocFromMainFile( func_variable.body_src_loc ) )
	{
		// This function is defined inside imported file - no need to use private linkage for it.
		llvm_function->setLinkage( llvm::GlobalValue::PrivateLinkage );
	}
	else if( func_variable.no_mangle )
	{
		// The only reason to use "nomangle" functions is to interact with external code.
		// For such purposes  external linkage is essential.
		llvm_function->setLinkage( llvm::GlobalValue::ExternalLinkage );
	}
	else if( IsSrcLocFromMainFile( func_variable.prototype_src_loc ) )
	{
		// This function has no portotype in imported files.
		// There is no reason to use external linkage for it.
		llvm_function->setLinkage( llvm::GlobalValue::PrivateLinkage );
	}
	else
	{
		// This is a non-template function in main file, that has prototype in imported file. Use external linkage.
		// Do not need to use comdat here, since this function is defined only in main (compiled) file.
		llvm_function->setLinkage( llvm::GlobalValue::ExternalLinkage );
	}

	// Ensure completeness only for functions body.
	// Require full completeness even for reference arguments.
	for( const FunctionType::Param& param : function_type.params )
	{
		if( !EnsureTypeComplete( param.type ) )
			REPORT_ERROR( UsingIncompleteType, parent_names_scope.GetErrors(), params.front().src_loc, param.type );
	}
	if( !EnsureTypeComplete( function_type.return_type ) )
		REPORT_ERROR( UsingIncompleteType, parent_names_scope.GetErrors(), func_variable.body_src_loc, function_type.return_type );

	if( reference_notation_deduction_context == nullptr )
	{
		// Call this after types completion request.
		// Perform this check while function body building, since the check requires type completeness, which can't be requested during prototype preparation.
		CheckCompleteFunctionReferenceNotation( func_variable.type, parent_names_scope.GetErrors(), func_variable.body_src_loc );
	}

	if( func_variable.IsCoroutine() )
	{
		const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &function_type.return_type.GetClassType()->generated_class_data );
		U_ASSERT( coroutine_type_description != nullptr );

		if( !EnsureTypeComplete( coroutine_type_description->return_type ) )
			REPORT_ERROR( UsingIncompleteType, parent_names_scope.GetErrors(), func_variable.body_src_loc,  coroutine_type_description->return_type  );

		for( const FunctionType::Param& param : function_type.params )
		{
			// Coroutine is an object, that holds references to reference-args of coroutine function.
			// It's forbidden to create types with references inside to types with other references inside.
			// So, check if this rule is not violated for coroutines.
			// Do this now, because it's impossible to check this in coroutine declaration, because this check requires complete types of parameters.
			if( param.value_type != ValueType::Value && param.type.ReferenceTagCount() > 0u )
				REPORT_ERROR( ReferenceFieldOfTypeWithReferencesInside, parent_names_scope.GetErrors(), params.front().src_loc, "some arg" ); // TODO - use separate error code.

			// Coroutine is not declared as non-sync, but param is non-sync. This is an error.
			// Check this while building function code in order to avoid complete arguments type preparation in "non_sync" tag evaluation during function preparation.
			if( !coroutine_type_description->non_sync && GetTypeNonSync( param.type, parent_names_scope, params.front().src_loc ) )
				REPORT_ERROR( CoroutineNonSyncRequired, parent_names_scope.GetErrors(), params.front().src_loc );
		}

		if( !coroutine_type_description->non_sync && GetTypeNonSync( coroutine_type_description->return_type, parent_names_scope, block.src_loc ) )
			REPORT_ERROR( CoroutineNonSyncRequired, parent_names_scope.GetErrors(), block.src_loc );
	}

	NamesScope function_names( "", &parent_names_scope );

	FunctionContext function_context( function_type, llvm_context_, llvm_function );

	function_context.return_type_deduction_context= return_type_deduction_context;
	function_context.reference_notation_deduction_context= reference_notation_deduction_context;
	function_context.lambda_preprocessing_context= lambda_preprocessing_context;

	const StackVariablesStorage args_storage( function_context );
	function_context.args_nodes.resize( function_type.params.size() );

	debug_info_builder_->SetCurrentLocation( func_variable.body_src_loc, function_context );

	// push args
	uint32_t arg_number= 0u;

	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;

	for( llvm::Argument& llvm_arg : llvm_function->args() )
	{
		// Skip "sret".
		if( &llvm_arg == &*llvm_function->arg_begin() && FunctionTypeIsSRet( function_type ) )
		{
			llvm_arg.setName( "_return_value" );
			function_context.s_ret= &llvm_arg;
			continue;
		}

		const FunctionType::Param& param= function_type.params[ arg_number ];

		const Synt::FunctionParam& declaration_arg= params[arg_number ];
		const std::string& arg_name= declaration_arg.name;

		const VariableMutPtr variable=
			Variable::Create(
				param.type,
				ValueType::Value,
				Variable::Location::Pointer,
				arg_name + " variable itself" );

		function_context.variables_state.AddNode( variable );

		if( param.value_type == ValueType::Value )
		{
			function_context.stack_variables_stack.back()->RegisterVariable( variable );

			if( param.type.GetFundamentalType() != nullptr ||
				param.type.GetEnumType() != nullptr ||
				param.type.GetRawPointerType() != nullptr ||
				param.type.GetFunctionPointerType() != nullptr )
			{
				// Move parameters to stack for assignment possibility.
				variable->llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable->type.GetLLVMType(), nullptr, arg_name );
				CreateLifetimeStart( function_context, variable->llvm_value );

				CreateTypedStore( function_context, variable->type, &llvm_arg, variable->llvm_value );
			}
			else if( param.type.GetClassType() != nullptr || param.type.GetArrayType() != nullptr || param.type.GetTupleType() != nullptr )
			{
				if( GetSingleScalarType( variable->type.GetLLVMType() ) != nullptr )
				{
					variable->llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable->type.GetLLVMType(), nullptr, arg_name );
					CreateLifetimeStart( function_context, variable->llvm_value );

					// Reinterretete composite type address as scalar type address and store value in it.
					function_context.llvm_ir_builder.CreateStore( &llvm_arg, variable->llvm_value );
				}
				else
				{
					// Values of composite types are passed via pointer.
					if( func_variable.IsCoroutine() )
					{
						// In coroutines we must save value args, passed by hidden reference, on local stack (this may be compiled as heap allocation).
						// This is needed, because address, allocated by coroutine function initial call, does not live long enough.
						variable->llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable->type.GetLLVMType(), nullptr, arg_name );
						CreateLifetimeStart( function_context, variable->llvm_value );
						CopyBytes( variable->llvm_value, &llvm_arg, param.type, function_context );
					}
					else
						variable->llvm_value = &llvm_arg;
				}
			}
			else U_ASSERT(false);

			debug_info_builder_->CreateVariableInfo( *variable, arg_name, declaration_arg.src_loc, function_context );
		}
		else
		{
			variable->llvm_value= &llvm_arg;
			debug_info_builder_->CreateReferenceVariableInfo( *variable, arg_name, declaration_arg.src_loc, function_context );
		}

		function_context.args_nodes[ arg_number ].first= variable;

		const auto reference_tag_count= param.type.ReferenceTagCount();
		function_context.args_nodes[ arg_number ].second.resize( reference_tag_count );
		for( size_t i= 0; i < reference_tag_count; ++i )
		{
			// Create root variable.
			const VariableMutPtr accesible_variable=
				Variable::Create(
					invalid_type_,
					ValueType::Value,
					Variable::Location::Pointer,
					arg_name + " referenced variable " + std::to_string(i) );

			const SecondOrderInnerReferenceKind second_order_inner_reference_kind= param.type.GetSecondOrderInnerReferenceKind(i);
			VariableMutPtr accessible_variable_inner_reference;
			if( second_order_inner_reference_kind != SecondOrderInnerReferenceKind::None )
			{
				 accessible_variable_inner_reference=
					Variable::Create(
						invalid_type_,
						second_order_inner_reference_kind == SecondOrderInnerReferenceKind::Imut ? ValueType::ReferenceImut : ValueType::ReferenceMut,
						Variable::Location::Pointer,
						arg_name + " referenced variable " + std::to_string(i) + " inner reference" );
				accessible_variable_inner_reference->is_inner_reference_node= true;
				accessible_variable_inner_reference->is_variable_inner_reference_node= true;

				U_ASSERT( accesible_variable->inner_reference_nodes.empty() );
				accesible_variable->inner_reference_nodes.push_back( accessible_variable_inner_reference );
			}

			function_context.variables_state.AddNode( accesible_variable );

			function_context.variables_state.AddLink( accesible_variable, variable->inner_reference_nodes[i] );

			function_context.args_nodes[ arg_number ].second[i]= accesible_variable;

			if( second_order_inner_reference_kind != SecondOrderInnerReferenceKind::None )
			{
				const VariablePtr second_order_accesible_variable=
					Variable::Create(
						invalid_type_,
						ValueType::Value,
						Variable::Location::Pointer,
						arg_name + " second order referenced variable " + std::to_string(i) );
				function_context.variables_state.AddNode( second_order_accesible_variable );

				function_context.variables_state.AddLink( second_order_accesible_variable, accessible_variable_inner_reference );

				function_context.args_second_order_nodes.resize( function_type.params.size() );
				function_context.args_second_order_nodes[ arg_number ].resize( reference_tag_count );
				function_context.args_second_order_nodes[ arg_number ][i]= second_order_accesible_variable;
			}
		}

		const VariablePtr variable_reference=
			Variable::Create(
				param.type,
				( param.value_type == ValueType::ReferenceMut || declaration_arg.mutability_modifier == MutabilityModifier::Mutable ) ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				arg_name,
				variable->llvm_value );

		function_context.variables_state.AddNode( variable_reference );
		function_context.variables_state.AddLink( variable, variable_reference );
		function_context.stack_variables_stack.back()->RegisterVariable( variable_reference );

		for( size_t i= 0; i < reference_tag_count; ++i )
			function_context.variables_state.AddLink( variable->inner_reference_nodes[i], variable_reference->inner_reference_nodes[i] );

		if( arg_number == 0u && arg_name == Keywords::this_ )
		{
			// Save "this" in function context for accessing inside class methods.
			function_context.this_= variable_reference;
		}
		else
		{
			const bool force_referenced= param.value_type == ValueType::Value && VariableExistanceMayHaveSideEffects(variable_reference->type);

			const NamesScopeValue* const inserted_arg=
				function_names.AddName( arg_name, NamesScopeValue( variable_reference, declaration_arg.src_loc, force_referenced ) );
			if( inserted_arg == nullptr )
				REPORT_ERROR( Redefinition, function_names.GetErrors(), declaration_arg.src_loc, arg_name );
		}

		llvm_arg.setName( "_arg_" + arg_name );
		++arg_number;
	}

	VariablePtr lambda_this;
	if( function_context.this_ != nullptr )
	{
		if( const auto this_class= function_context.this_->type.GetClassType() )
		{
			if( const auto lambda_class_data= std::get_if<LambdaClassData>( &this_class->generated_class_data ) )
			{
				// Create names for lambda fields.
				U_ASSERT( !function_type.params.empty() );
				U_ASSERT( !params.empty() );

				if( function_type.params.front().value_type == ValueType::Value &&
					params.front().mutability_modifier == MutabilityModifier::Mutable )
				{
					// If this is a "byval mut" lambda - create names for captured variables as params, in order to have possibility to move them.
					for( const LambdaClassData::Capture& capture : lambda_class_data->captures )
					{
						const std::string& field_name= capture.captured_variable_name;
						const ClassField& class_field= *capture.field;

						llvm::Value* const field_address= CreateClassFieldGEP( function_context, *this_class, function_context.this_->llvm_value, class_field.index );
						llvm::Value* const llvm_value=
							class_field.is_reference
								? CreateTypedReferenceLoad( function_context, class_field.type, field_address )
								: field_address;
						llvm_value->setName( field_name );

						const VariableMutPtr variable=
							Variable::Create(
								class_field.type,
								ValueType::Value,
								Variable::Location::Pointer,
								field_name + " variable itself",
								llvm_value );

						const VariablePtr variable_reference=
							Variable::Create(
								class_field.type,
								class_field.is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut,
								Variable::Location::Pointer,
								field_name,
								variable->llvm_value );

						function_context.variables_state.AddNode( variable );
						function_context.variables_state.AddNode( variable_reference );
						function_context.variables_state.AddLink( variable, variable_reference );

						if( !class_field.is_reference )
							function_context.stack_variables_stack.back()->RegisterVariable( variable );
						function_context.stack_variables_stack.back()->RegisterVariable( variable_reference );

						const auto reference_tag_count= class_field.type.ReferenceTagCount();
						for( size_t i= 0; i < reference_tag_count; ++i )
						{
							// Create root variable.
							const VariableMutPtr accesible_variable=
								Variable::Create(
									invalid_type_,
									ValueType::Value,
									Variable::Location::Pointer,
									field_name + " referenced variable " + std::to_string(i) );

							const SecondOrderInnerReferenceKind second_order_inner_reference_kind= class_field.type.GetSecondOrderInnerReferenceKind(i);
							VariableMutPtr accessible_variable_inner_reference;
							if( second_order_inner_reference_kind != SecondOrderInnerReferenceKind::None )
							{
								VariableMutPtr accessible_variable_inner_reference=
									Variable::Create(
										invalid_type_,
										second_order_inner_reference_kind == SecondOrderInnerReferenceKind::Imut ? ValueType::ReferenceImut : ValueType::ReferenceMut,
										Variable::Location::Pointer,
										field_name + " referenced variable " + std::to_string(i) + " inner reference" );
								accessible_variable_inner_reference->is_inner_reference_node= true;
								accessible_variable_inner_reference->is_variable_inner_reference_node= true;

								U_ASSERT( accesible_variable->inner_reference_nodes.empty() );
								accesible_variable->inner_reference_nodes.push_back( accessible_variable_inner_reference );
							}

							function_context.variables_state.AddNode( accesible_variable );
							function_context.variables_state.AddLink( accesible_variable, variable->inner_reference_nodes[i] );
							function_context.variables_state.AddLink( variable->inner_reference_nodes[i], variable_reference->inner_reference_nodes[i] );

							if( second_order_inner_reference_kind != SecondOrderInnerReferenceKind::None )
							{
								const VariablePtr second_order_accesible_variable=
									Variable::Create(
										invalid_type_,
										ValueType::Value,
										Variable::Location::Pointer,
										field_name + " second order referenced variable " + std::to_string(i) );
								function_context.variables_state.AddNode( second_order_accesible_variable );

								function_context.variables_state.AddLink( second_order_accesible_variable, accessible_variable_inner_reference );
							}
						}

						function_names.AddName( field_name, NamesScopeValue( variable_reference, block.src_loc ) );
					}

					// Move "this" to prevent its destruction.
					// Destroy instead each member separately.
					const auto this_input_nodes= function_context.variables_state.GetNodeInputLinks( function_context.this_ );
					U_ASSERT( this_input_nodes.size() == 1 );
					function_context.variables_state.MoveNode( *this_input_nodes.begin() );
					function_context.variables_state.MoveNode( function_context.this_ );
				}
				else
				{
					// Just create names for lambda fields, if it's not possible to move these fields.
					this_class->members->ForEachInThisScope(
						[&]( const std::string_view member_name, const NamesScopeValue& value )
						{
							if( const auto class_field= value.value.GetClassField() )
							{
								const auto field_value= AccessClassField( function_names, function_context, function_context.this_, *class_field, std::string(member_name), block.src_loc );
								function_names.AddName( member_name, NamesScopeValue( field_value, block.src_loc ) );
								// TODO - create debug info?
							}
						} );
				}

				// Make "this" unavailable in lambdas.
				// Save "this" into a stack variable in order to prevent its destruction.
				lambda_this= function_context.this_;
				function_context.this_= nullptr;
			}
		}
	}

	if( func_variable.IsCoroutine() )
	{
		// Create coroutine entry block after saving args to stack.
		PrepareCoroutineBlocks( function_context );
		// Generate also initial suspend.
		CoroutineSuspend( function_names, function_context, block.src_loc );
	}

	if( is_constructor )
	{
		U_ASSERT( base_class != nullptr );
		U_ASSERT( function_context.this_ != nullptr );

		if( syntax_element.constructor_initialization_list == nullptr )
		{
			// Create dummy initialization list for constructors without explicit initialization list.
			const Synt::StructNamedInitializer dumy_initialization_list( block.src_loc );

			BuildConstructorInitialization(
				function_context.this_,
				*base_class,
				function_names,
				function_context,
				dumy_initialization_list );
		}
		else
			BuildConstructorInitialization(
				function_context.this_,
				*base_class,
				function_names,
				function_context,
				*syntax_element.constructor_initialization_list );
	}

	if( ( is_constructor || is_destructor ) && ( base_class->kind == Class::Kind::Abstract || base_class->kind == Class::Kind::Interface ) )
		function_context.whole_this_is_unavailable= true; // Whole "this" unavailable in body of constructors and destructors of abstract classes and interfaces.

	if( is_destructor )
	{
		SetupVirtualTablePointers( function_context.this_->llvm_value, *base_class, function_context );
		function_context.destructor_end_block= llvm::BasicBlock::Create( llvm_context_ );

		// Do not allow to access "this" in destructors of structs with mutable references inside.
		// Allow only accessing separate fields.
		// This is needed in order to prevent invalidation of possible derived references in destructor calls.
		if( function_context.this_ != nullptr && function_context.this_->type.ContainsMutableReferences() )
		{
			function_context.whole_this_is_unavailable= true;
			function_context.base_initialized= true;
		}
	}

	// Do not create separate namespace for function root block, reuse namespace of args.
	const BlockBuildInfo block_build_info= BuildBlockElements( function_names, function_context, block.elements );
	U_ASSERT( function_context.stack_variables_stack.size() == 1u );

	if( func_variable.constexpr_kind != FunctionVariable::ConstexprKind::NonConstexpr )
	{
		// Check function type and function body.
		// Function type checked here, because in case of constexpr methods not all types are complete yet.

		// For auto-constexpr functions we do not force type completeness. If function is really-constexpr, it must already make complete using types.

		const bool auto_constexpr= func_variable.constexpr_kind == FunctionVariable::ConstexprKind::ConstexprAuto;
		bool can_be_constexpr= true;

		if( !auto_constexpr && !EnsureTypeComplete( function_type.return_type ) )
			REPORT_ERROR( UsingIncompleteType, function_names.GetErrors(), func_variable.body_src_loc, function_type.return_type ); // Completeness required for constexpr possibility check.

		if( function_type.unsafe ||
			!function_type.return_type.CanBeConstexpr() ||
			!function_type.references_pollution.empty() ) // Side effects, such pollution, not allowed.
			can_be_constexpr= false;

		if( function_type.return_type.GetFunctionPointerType() != nullptr ) // Currently function pointers not supported.
			can_be_constexpr= false;

		// If this is preprocessing of auto-return function, check also deduced return type.
		if( function_context.return_type_deduction_context != nullptr &&
			function_context.return_type_deduction_context->return_type != std::nullopt &&
			( !function_context.return_type_deduction_context->return_type->CanBeConstexpr() ||
			   function_context.return_type_deduction_context->return_type->GetFunctionPointerType() != nullptr ) )
			can_be_constexpr= false;

		for( const FunctionType::Param& param : function_type.params )
		{
			if( !auto_constexpr && !EnsureTypeComplete( param.type ) )
				REPORT_ERROR( UsingIncompleteType, function_names.GetErrors(), func_variable.body_src_loc, param.type ); // Completeness required for constexpr possibility check.

			if( !param.type.CanBeConstexpr() || // Allowed only constexpr types. Incomplete types are not constexpr.
				param.type.GetFunctionPointerType() != nullptr )
				can_be_constexpr= false;

			// We support constexpr functions with mutable reference-arguments, but such functions can not be used as root for constexpr function evaluation.
			// We support also constexpr constructors (except constexpr copy constructors), but constexpr constructors currently can not be used for constexpr variables initialization.
		}

		if( auto_constexpr )
		{
			if( can_be_constexpr && !function_context.has_non_constexpr_operations_inside )
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
			else if( function_context.has_non_constexpr_operations_inside )
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
	if( !block_build_info.has_terminal_instruction_inside )
	{
		if( func_variable.kind == FunctionVariable::Kind::Generator )
		{
			// Add final suspention point for generators.
			CoroutineFinalSuspend( function_names, function_context, block.end_src_loc );
		}
		else if( func_variable.kind == FunctionVariable::Kind::Async )
		{
			// Automatically generate "return" for void-return async functions.

			const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &function_type.return_type.GetClassType()->generated_class_data );
			U_ASSERT( coroutine_type_description != nullptr );

			if( !( coroutine_type_description->return_type == void_type_ && coroutine_type_description->return_value_type == ValueType::Value ) )
			{
				REPORT_ERROR( NoReturnInFunctionReturningNonVoid, function_names.GetErrors(), block.end_src_loc );
				return;
			}
			CoroutineFinalSuspend( function_names, function_context, block.end_src_loc );
		}
		else if( return_type_deduction_context != nullptr )
		{
			if( reference_notation_deduction_context != nullptr )
				CollectReferencePollution( function_context );
		}
		else
		{
			// Automatically generate "return" for void-return functions.
			if( !( function_type.return_type == void_type_ && function_type.return_value_type == ValueType::Value ) )
			{
				REPORT_ERROR( NoReturnInFunctionReturningNonVoid, function_names.GetErrors(), block.end_src_loc );
				return;
			}
			BuildEmptyReturn( function_names, function_context, block.end_src_loc );
		}
	}

	if( is_destructor )
	{
		// Fill destructors block.
		U_ASSERT( function_context.destructor_end_block != nullptr );
		function_context.llvm_ir_builder.SetInsertPoint( function_context.destructor_end_block );
		llvm_function->getBasicBlockList().push_back( function_context.destructor_end_block );

		CallMembersDestructors( function_context, function_names.GetErrors(), block.end_src_loc );
		function_context.llvm_ir_builder.CreateRetVoid();
	}

	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );

	// Early return for auto-return function preprocessing.
	if( return_type_deduction_context != nullptr )
		return;

	// Early return for lambda preprocessing.
	if( lambda_preprocessing_context != nullptr )
		return;

	// Perform final steps for regular functions (if this is not some preprocessing).

	CheckForUnusedLocalNames( function_names );

	// Clear incomplete function marker. Now it is safe to execute it in constexpr context.
	llvm_function->setMetadata( "__U_incomplete_function_marker", nullptr );

	TryToPerformReturnValueAllocationOptimization( *llvm_function );
}

void CodeBuilder::BuildStaticAssert( StaticAssert& static_assert_, NamesScope& names_scope, FunctionContext& function_context )
{
	if( static_assert_.syntax_element == nullptr )
		return;

	BuildBlockElementImpl( names_scope, function_context, *static_assert_.syntax_element );
	static_assert_.syntax_element= nullptr;
}


llvm::Type* CodeBuilder::GetFundamentalLLVMType( const U_FundamentalType fundmantal_type )
{
	switch( fundmantal_type )
	{
	case U_FundamentalType::InvalidType: return fundamental_llvm_types_.invalid_type_;
	case U_FundamentalType::void_: return fundamental_llvm_types_.void_;
	case U_FundamentalType::bool_: return fundamental_llvm_types_.bool_;
	case U_FundamentalType::i8_  : return fundamental_llvm_types_.i8_  ;
	case U_FundamentalType::u8_  : return fundamental_llvm_types_.u8_  ;
	case U_FundamentalType::i16_ : return fundamental_llvm_types_.i16_ ;
	case U_FundamentalType::u16_ : return fundamental_llvm_types_.u16_ ;
	case U_FundamentalType::i32_ : return fundamental_llvm_types_.i32_ ;
	case U_FundamentalType::u32_ : return fundamental_llvm_types_.u32_ ;
	case U_FundamentalType::i64_ : return fundamental_llvm_types_.i64_ ;
	case U_FundamentalType::u64_ : return fundamental_llvm_types_.u64_ ;
	case U_FundamentalType::i128_: return fundamental_llvm_types_.i128_;
	case U_FundamentalType::u128_: return fundamental_llvm_types_.u128_;
	case U_FundamentalType::ssize_type_: return fundamental_llvm_types_.ssize_type_;
	case U_FundamentalType::size_type_ : return fundamental_llvm_types_.size_type_ ;
	case U_FundamentalType::f32_: return fundamental_llvm_types_.f32_;
	case U_FundamentalType::f64_: return fundamental_llvm_types_.f64_;
	case U_FundamentalType::char8_ : return fundamental_llvm_types_.char8_ ;
	case U_FundamentalType::char16_: return fundamental_llvm_types_.char16_;
	case U_FundamentalType::char32_: return fundamental_llvm_types_.char32_;
	case U_FundamentalType::byte8_  : return fundamental_llvm_types_.byte8_  ;
	case U_FundamentalType::byte16_ : return fundamental_llvm_types_.byte16_ ;
	case U_FundamentalType::byte32_ : return fundamental_llvm_types_.byte32_ ;
	case U_FundamentalType::byte64_ : return fundamental_llvm_types_.byte64_ ;
	case U_FundamentalType::byte128_: return fundamental_llvm_types_.byte128_;
	case U_FundamentalType::LastType:
		break;
	};

	U_ASSERT(false);
	return nullptr;
}

uint64_t CodeBuilder::GetFundamentalTypeSize( const U_FundamentalType fundamental_type )
{
	// Handle here all cases except size_type and ssize_type, which have variable size.
	switch(fundamental_type)
	{
	case U_FundamentalType::InvalidType: return 0u;
	case U_FundamentalType::void_: return 0u;
	case U_FundamentalType::bool_: return 1u;
	case U_FundamentalType::i8_  : return  1u;
	case U_FundamentalType::u8_  : return  1u;
	case U_FundamentalType::i16_ : return  2u;
	case U_FundamentalType::u16_ : return  2u;
	case U_FundamentalType::i32_ : return  4u;
	case U_FundamentalType::u32_ : return  4u;
	case U_FundamentalType::i64_ : return  8u;
	case U_FundamentalType::u64_ : return  8u;
	case U_FundamentalType::i128_: return 16u;
	case U_FundamentalType::u128_: return 16u;
	case U_FundamentalType::ssize_type_: return fundamental_llvm_types_.ssize_type_->getBitWidth() >> 3;
	case U_FundamentalType::size_type_ : return fundamental_llvm_types_.size_type_ ->getBitWidth() >> 3;
	case U_FundamentalType::f32_: return 4u;
	case U_FundamentalType::f64_: return 8u;
	case U_FundamentalType::char8_ : return 1u;
	case U_FundamentalType::char16_: return 2u;
	case U_FundamentalType::char32_: return 4u;
	case U_FundamentalType::byte8_  : return  1u;
	case U_FundamentalType::byte16_ : return  2u;
	case U_FundamentalType::byte32_ : return  4u;
	case U_FundamentalType::byte64_ : return  8u;
	case U_FundamentalType::byte128_: return 16u;
	case U_FundamentalType::LastType: break;
	};

	U_ASSERT( false );
	return 0u;
}

llvm::Value* CodeBuilder::CreateTypedLoad( FunctionContext& function_context, const Type& type, llvm::Value* const address )
{
	if( address == nullptr || function_context.is_functionless_context )
		return nullptr;

	if( type == void_type_ )
		return llvm::UndefValue::get( fundamental_llvm_types_.void_ );

	llvm::LoadInst* const result= function_context.llvm_ir_builder.CreateLoad( type.GetLLVMType(), address );

	if( generate_tbaa_metadata_ )
		result->setMetadata( llvm::LLVMContext::MD_tbaa, tbaa_metadata_builder_.CreateAccessTag( type ) );

	return result;
}

llvm::LoadInst* CodeBuilder::CreateTypedReferenceLoad( FunctionContext& function_context, const Type& type, llvm::Value* const address )
{
	if( address == nullptr || function_context.is_functionless_context )
		return nullptr;

	llvm::LoadInst* const result= function_context.llvm_ir_builder.CreateLoad( type.GetLLVMType()->getPointerTo(), address );

	if( generate_tbaa_metadata_ )
		result->setMetadata( llvm::LLVMContext::MD_tbaa, tbaa_metadata_builder_.CreateReferenceAccessTag( type ) );

	// References are never null, so, mark result of reference load with "nonnull" metadata.
	result->setMetadata( llvm::LLVMContext::MD_nonnull, llvm::MDNode::get( llvm_context_, llvm::None ) );

	return result;
}

void CodeBuilder::CreateTypedStore( FunctionContext& function_context, const Type& type, llvm::Value* const value_to_store, llvm::Value* const address )
{
	if( address == nullptr || function_context.is_functionless_context )
		return;

	if( type == void_type_ )
		return;

	llvm::StoreInst* const result= function_context.llvm_ir_builder.CreateStore( value_to_store, address );

	if( generate_tbaa_metadata_ )
		result->setMetadata( llvm::LLVMContext::MD_tbaa, tbaa_metadata_builder_.CreateAccessTag( type ) );
}

void CodeBuilder::CreateTypedReferenceStore( FunctionContext& function_context, const Type& type,  llvm::Value* const value_to_store, llvm::Value* const address )
{
	if( address == nullptr || function_context.is_functionless_context )
		return;

	llvm::StoreInst* const result= function_context.llvm_ir_builder.CreateStore( value_to_store, address );

	if( generate_tbaa_metadata_ )
		result->setMetadata( llvm::LLVMContext::MD_tbaa, tbaa_metadata_builder_.CreateReferenceAccessTag( type ) );
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
		return CreateTypedLoad( function_context, variable.type, variable.llvm_value );
	};

	U_ASSERT(false);
	return nullptr;
}

llvm::Constant* CodeBuilder::GetZeroGEPIndex()
{
	return llvm::Constant::getNullValue( fundamental_llvm_types_.i32_ );
}

llvm::Constant* CodeBuilder::GetFieldGEPIndex( const uint64_t field_index )
{
	return llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32_, llvm::APInt( 32u, field_index ) );
}

llvm::Value*CodeBuilder::CreateBaseClassGEP( FunctionContext& function_context, const Class& class_type, llvm::Value* const class_ptr )
{
	return CreateClassFieldGEP( function_context, class_type, class_ptr, 0 /* base class is allways first field */ );
}

llvm::Value* CodeBuilder::CreateClassFieldGEP( FunctionContext& function_context, const Variable& class_variable, const uint64_t field_index )
{
	const auto class_type= class_variable.type.GetClassType();
	U_ASSERT(class_type != nullptr);
	return CreateClassFieldGEP( function_context, *class_type, class_variable.llvm_value, field_index );
}

llvm::Value* CodeBuilder::CreateClassFieldGEP( FunctionContext& function_context, const Class& class_type, llvm::Value* const class_ptr, const uint64_t field_index )
{
	return CreateCompositeElementGEP( function_context, class_type.llvm_type, class_ptr, GetFieldGEPIndex( field_index ) );
}

llvm::Value* CodeBuilder::CreateTupleElementGEP( FunctionContext& function_context, const Variable& tuple_variable, const uint64_t element_index )
{
	const auto tuple_type= tuple_variable.type.GetTupleType();
	U_ASSERT(tuple_type != nullptr);
	return CreateTupleElementGEP( function_context, *tuple_type, tuple_variable.llvm_value, element_index );
}

llvm::Value* CodeBuilder::CreateTupleElementGEP( FunctionContext& function_context, const TupleType& tuple_type, llvm::Value* const tuple_ptr, const uint64_t element_index )
{
	return CreateCompositeElementGEP( function_context, tuple_type.llvm_type, tuple_ptr, GetFieldGEPIndex( element_index ) );
}

llvm::Value* CodeBuilder::CreateArrayElementGEP( FunctionContext& function_context, const Variable& array_variable, const uint64_t element_index )
{
	return CreateArrayElementGEP( function_context, array_variable, llvm::ConstantInt::get( fundamental_llvm_types_.u64_, element_index ) );
}

llvm::Value* CodeBuilder::CreateArrayElementGEP( FunctionContext& function_context, const Variable& array_variable, llvm::Value* const index )
{
	const auto array_type= array_variable.type.GetArrayType();
	U_ASSERT(array_type != nullptr);
	return CreateArrayElementGEP( function_context, *array_type, array_variable.llvm_value, index );
}

llvm::Value* CodeBuilder::CreateArrayElementGEP( FunctionContext& function_context, const ArrayType& array_type, llvm::Value* const array_ptr, const uint64_t element_index )
{
	return CreateArrayElementGEP( function_context, array_type, array_ptr, llvm::ConstantInt::get( fundamental_llvm_types_.u64_, element_index ) );
}

llvm::Value* CodeBuilder::CreateArrayElementGEP( FunctionContext& function_context, const ArrayType& array_type, llvm::Value* const array_ptr, llvm::Value* const index )
{
	return CreateCompositeElementGEP( function_context, array_type.llvm_type, array_ptr, index );
}

llvm::Value* CodeBuilder::CreateCompositeElementGEP( FunctionContext& function_context, llvm::Type* const type, llvm::Value* const value, llvm::Value* const index )
{
	if( value == nullptr || index == nullptr )
		return nullptr;

	// Allow only constant GEP in functionless context.
	if( function_context.is_functionless_context && !(llvm::isa<llvm::Constant>(value) && llvm::isa<llvm::Constant>(index) ) )
		return nullptr;

	return function_context.llvm_ir_builder.CreateInBoundsGEP( type, value, { GetZeroGEPIndex(), index } );
}

llvm::Value* CodeBuilder::ForceCreateConstantIndexGEP( FunctionContext& function_context, llvm::Type* const type, llvm::Value* const value, const uint32_t index )
{
	if( value == nullptr )
		return nullptr;

	const auto index_value= GetFieldGEPIndex( index );

	if( llvm::isa<llvm::Constant>(value) )
	{
		// Constant will be folded properly and no instruction will be actiually inserted.
		return function_context.llvm_ir_builder.CreateInBoundsGEP( type, value, { GetZeroGEPIndex(), index_value } );
	}

	const auto gep= llvm::GetElementPtrInst::CreateInBounds( type, value, { GetZeroGEPIndex(), index_value } );

	// Try to insert "GEP" instruction with constant index directly after of value calculation.
	// This is needed in order to have possibility to reuse this instruction in diffirent basic blocks.
	if( const auto instruction= llvm::dyn_cast<llvm::Instruction>( value ) )
		gep->insertAfter( instruction );
	else if( llvm::isa<llvm::Argument>( value ) )
		function_context.alloca_ir_builder.Insert( gep );
	else
		function_context.llvm_ir_builder.Insert( gep ); // TODO - maybe add assert here?

	return gep;
}

llvm::Value* CodeBuilder::CreateReferenceCast( llvm::Value* const ref, const Type& src_type, const Type& dst_type, FunctionContext& function_context )
{
	U_ASSERT( src_type.ReferenceIsConvertibleTo( dst_type ) );

	if( ref == nullptr )
		return nullptr;

	if( src_type == dst_type )
		return ref;

	const Class* const src_class_type= src_type.GetClassType();
	U_ASSERT( src_class_type != nullptr );

	for( const Class::Parent& src_parent_class : src_class_type->parents )
	{
		llvm::Value* const sub_ref= CreateClassFieldGEP( function_context, *src_class_type, ref, src_parent_class.field_number );

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
	const std::string_view mangled_name,
	llvm::Constant* const initializer )
{
	// Try to reuse global variable.
	if( llvm::GlobalVariable* const prev_literal_name= module_->getNamedGlobal( StringViewToStringRef(mangled_name) ) )
		if( prev_literal_name->getInitializer() == initializer ) // llvm reuses constants, so, for equal constants pointers will be same.
			return prev_literal_name;
	
	llvm::GlobalVariable* const val=
		new llvm::GlobalVariable(
			*module_,
			type.GetLLVMType(),
			true, // is constant
			llvm::GlobalValue::PrivateLinkage, // We have no external variables, so, use private linkage.
			initializer,
			StringViewToStringRef(mangled_name) );

	val->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );

	return val;
}

llvm::GlobalVariable* CodeBuilder::CreateGlobalMutableVariable( const Type& type, const std::string_view mangled_name, const bool externally_available )
{
	const auto var=
		new llvm::GlobalVariable(
			*module_,
			type.GetLLVMType(),
			false, // is constant
			llvm::GlobalValue::ExternalLinkage,
			nullptr,
			StringViewToStringRef(mangled_name) );

	if( externally_available )
	{
		// Use external linkage and comdat for global mutable variables to guarantee address uniqueness.
		llvm::Comdat* const comdat= module_->getOrInsertComdat( var->getName() );
		comdat->setSelectionKind( llvm::Comdat::Any );
		var->setComdat( comdat );

		var->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );
	}
	else
	{
		// This global mutable variable is local for this module, do not need to use external linkage.
		var->setLinkage( llvm::GlobalValue::PrivateLinkage );
	}

	return var;
}

bool CodeBuilder::IsGlobalVariable( const VariablePtr& variable )
{
	return
		variable->llvm_value != nullptr &&
		llvm::isa<llvm::Constant>( variable->llvm_value ) &&
		variable->location == Variable::Location::Pointer;
}

llvm::Function* CodeBuilder::EnsureLLVMFunctionCreated( const FunctionVariable& function_variable )
{
	llvm::Function*& llvm_function= function_variable.llvm_function->function;

	if( llvm_function != nullptr )
		return llvm_function;

	const FunctionType& function_type= function_variable.type;

	llvm_function=
		llvm::Function::Create(
			GetLLVMFunctionType( function_type ),
			// Use private linkage for generated function.
			function_variable.is_generated ? llvm::GlobalValue::PrivateLinkage : llvm::Function::LinkageTypes::ExternalLinkage,
			function_variable.llvm_function->name_mangled,
			module_.get() );

	llvm_function->setCallingConv( function_type.calling_convention );

	// Merge functions with identical code.
	// We doesn`t need different addresses for different functions.
	llvm_function->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );

	llvm_function->setDoesNotThrow(); // We do not support exceptions.

	if( build_debug_info_ ) // Unwind table entry for function needed for debug info.
	{
		llvm::AttrBuilder builder(llvm_context_);
		builder.addUWTableAttr(llvm::UWTableKind::Async);
		llvm_function->addFnAttrs( builder );
	}

	if( function_variable.IsCoroutine() )
		llvm_function->addFnAttr( llvm::Attribute::PresplitCoroutine );

	// Prepare params attributes.

	const bool first_param_is_sret= FunctionTypeIsSRet( function_type );

	for( size_t i= 0u; i < function_type.params.size(); i++ )
	{
		const auto param_attr_index= uint32_t(i + (first_param_is_sret ? 1u : 0u ));
		const FunctionType::Param& param= function_type.params[i];

		const bool pass_value_param_by_hidden_ref=
			param.value_type == ValueType::Value &&
			(param.type.GetClassType() != nullptr || param.type.GetArrayType() != nullptr || param.type.GetTupleType() != nullptr ) &&
			GetSingleScalarType( param.type.GetLLVMType() ) == nullptr;

		// Mark reference params as nonnull.
		if( param.value_type != ValueType::Value || pass_value_param_by_hidden_ref )
			llvm_function->addParamAttr( param_attr_index, llvm::Attribute::NonNull );
		// Mutable reference params or composite value-args must not alias.
		// Also we can mark as "noalias" non-mutable references. See https://releases.llvm.org/9.0.0/docs/AliasAnalysis.html#must-may-or-no.
		if( param.value_type != ValueType::Value || pass_value_param_by_hidden_ref )
			llvm_function->addParamAttr( param_attr_index, llvm::Attribute::NoAlias );
		// Mark as "readonly" immutable reference params.
		if( param.value_type == ValueType::ReferenceImut )
			llvm_function->addParamAttr( param_attr_index, llvm::Attribute::ReadOnly );
		// Mark as "nocapture" value args of composite types, which is actually passed by hidden reference.
		// It is not possible to capture this reference.
		if( pass_value_param_by_hidden_ref )
			llvm_function->addParamAttr( param_attr_index, llvm::Attribute::NoCapture );
	}

	// Prepare ret attributes.
	if( first_param_is_sret )
	{
		llvm_function->addParamAttr( 0, llvm::Attribute::NoAlias );

		llvm::AttrBuilder builder(llvm_context_);
		builder.addStructRetAttr(function_type.return_type.GetLLVMType());
		llvm_function->addParamAttrs( 0, builder );
	}
	if( function_type.return_value_type != ValueType::Value )
		llvm_function->addRetAttr( llvm::Attribute::NonNull );

	// We can't specify dereferenceable attrubutes here, since types of reference args and return values may be still incomplete.
	// So, setup dereferenceable attributes later, using separate pass.

	return llvm_function;
}

void CodeBuilder::SetupDereferenceableFunctionParamsAndRetAttributes( FunctionVariable& function_variable )
{
	llvm::Function* const llvm_function= function_variable.llvm_function->function;
	if( llvm_function == nullptr )
	{
		// Do not force to create llvm function, if it was not created previously.
		// This means, that this is only unused declaration.
		return;
	}

	const FunctionType& function_type= function_variable.type;

	const bool first_param_is_sret= FunctionTypeIsSRet( function_type );

	for( size_t i= 0u; i < function_type.params.size(); i++ )
	{
		const auto param_attr_index= uint32_t(i + (first_param_is_sret ? 1u : 0u ));
		const FunctionType::Param& param= function_type.params[i];

		const bool pass_value_param_by_hidden_ref=
			param.value_type == ValueType::Value &&
			( param.type.GetClassType() != nullptr || param.type.GetArrayType() != nullptr || param.type.GetTupleType() != nullptr ) &&
			GetSingleScalarType( param.type.GetLLVMType() ) == nullptr;
		// Mark reference params and passed by hidden reference params with "dereferenceable" attribute.
		if( param.value_type != ValueType::Value || pass_value_param_by_hidden_ref )
		{
			const auto llvm_type= param.type.GetLLVMType();
			if( !llvm_type->isSized() )
				continue; // May be in case of error.

			llvm_function->addDereferenceableParamAttr( param_attr_index, data_layout_.getTypeAllocSize( llvm_type ) );
		}
	}

	const auto llvm_ret_type= function_type.return_type.GetLLVMType();
	if( !llvm_ret_type->isSized() )
		return; // May be in case of error.

	if( first_param_is_sret )
		llvm_function->addDereferenceableParamAttr( 0, data_layout_.getTypeAllocSize( llvm_ret_type ) );
	else if( function_type.return_value_type != ValueType::Value )
	{
		llvm::AttrBuilder builder(llvm_context_);
		builder.addDereferenceableAttr( data_layout_.getTypeAllocSize( llvm_ret_type ) );
		llvm_function->addRetAttrs(builder);
	}
}

void CodeBuilder::SetupDereferenceableFunctionParamsAndRetAttributes_r( NamesScope& names_scope )
{
	names_scope.ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const NamesScopePtr inner_namespace= value.GetNamespace() )
				SetupDereferenceableFunctionParamsAndRetAttributes_r( *inner_namespace );
			else if( const OverloadedFunctionsSetPtr functions_set= value.GetFunctionsSet() )
			{
				for( FunctionVariable& function_variable : functions_set->functions )
					SetupDereferenceableFunctionParamsAndRetAttributes( function_variable );
			}
			else if( const Type* const type= value.GetTypeName() )
			{
				if( const ClassPtr class_type= type->GetClassType() )
				{
					// Process classes only from parent namespace.
					// Otherwise we can get loop, using type alias.
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

	const auto alloca_inst= llvm::dyn_cast<llvm::AllocaInst>(address);
	if( alloca_inst == nullptr )
		return;

	llvm::Type* const type= alloca_inst->getAllocatedType();
	if( !type->isSized() )
		return; // May be in case of error.

	function_context.llvm_ir_builder.CreateLifetimeStart(
		address,
		llvm::ConstantInt::get(
			fundamental_llvm_types_.u64_,
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

	const auto alloca_inst= llvm::dyn_cast<llvm::AllocaInst>(address);
	if( alloca_inst == nullptr )
		return;

	llvm::Type* const type= alloca_inst->getAllocatedType();
	if( !type->isSized() )
		return; // May be in case of error.

	function_context.llvm_ir_builder.CreateLifetimeEnd(
		address,
		llvm::ConstantInt::get(
			fundamental_llvm_types_.u64_,
			data_layout_.getTypeAllocSize(type) ) );

	if( generate_lifetime_start_end_debug_calls_ )
		function_context.llvm_ir_builder.CreateCall(
			lifetime_end_debug_func_,
			{
				function_context.llvm_ir_builder.CreatePointerCast( address, lifetime_end_debug_func_->arg_begin()->getType() )
			} );
}

CodeBuilder::FunctionContextState CodeBuilder::SaveFunctionContextState( FunctionContext& function_context )
{
	FunctionContextState result;
	result.variables_state= function_context.variables_state;
	result.block_count= function_context.function->getBasicBlockList().size();
	return result;
}

void CodeBuilder::RestoreFunctionContextState( FunctionContext& function_context, const FunctionContextState& state )
{
	function_context.variables_state= state.variables_state;

	// Make sure no new basic blocks were added.
	U_ASSERT( function_context.function->getBasicBlockList().size() == state.block_count );
	// New instructions may still be added - in case of GEP for structs or tuples. But it is fine since such instructions have no side-effects.
}

} // namespace U
