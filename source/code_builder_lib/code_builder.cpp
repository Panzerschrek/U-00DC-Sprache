#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include "pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "../sprache_version/sprache_version.hpp"
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
	std::string target_triple_str,
	const llvm::DataLayout& data_layout,
	bool build_debug_info )
	: llvm_context_( llvm_context )
	, target_triple_str_(std::move(target_triple_str))
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

ICodeBuilder::BuildResult CodeBuilder::BuildProgram( const SourceGraph& source_graph )
{
	global_errors_.clear();

	module_=
		std::make_unique<llvm::Module>(
			source_graph.nodes_storage[ source_graph.root_node_index ].file_path,
			llvm_context_ );

	// Setup data layout
	module_->setDataLayout(data_layout_);
	module_->setTargetTriple(target_triple_str_);

	// Prepare halt func.
	halt_func_=
		llvm::Function::Create(
			llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret, false ),
			llvm::Function::ExternalLinkage,
			"__U_halt",
			module_.get() );
	halt_func_->setDoesNotReturn();
	halt_func_->setDoesNotThrow();
	halt_func_->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );

	// In some places outside functions we need to execute expression evaluation.
	// Create for this function context.
	llvm::Function* const global_function=
		llvm::Function::Create(
			llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret, false ),
			llvm::Function::LinkageTypes::ExternalLinkage,
			"",
			module_.get() );

	FunctionContext global_function_context(
		void_type_for_ret_,
		false, false,
		llvm_context_,
		global_function );
	const StackVariablesStorage global_function_variables_storage( global_function_context );
	global_function_context_= &global_function_context;

	// Prepare debug info builder.
	if( build_debug_info_ )
	{
		debug_info_.builder= std::make_unique<llvm::DIBuilder>( *module_ );

		module_->addModuleFlag( llvm::Module::Warning, "Debug Info Version", 3 );

		const uint32_t c_dwarf_language_id= 0x8000 /* first user-defined language code */ + 0xDC /* code of "Ü" letter */;

		debug_info_.source_file_entries.reserve( source_graph.nodes_storage.size() );
		for( const SourceGraph::Node& source_graph_node : source_graph.nodes_storage )
		{
			const auto file= debug_info_.builder->createFile( source_graph_node.file_path, "");

			const auto compile_unit=
				debug_info_.builder->createCompileUnit(
					c_dwarf_language_id,
					file,
					"Ü-Sprache compiler " + getFullVersion(),
					false, // optimized
					"",
					0 /* runtime version */ );

			debug_info_.source_file_entries.push_back( DebugSourceFileEntry{ file, compile_unit } );
		}
	}

	// Build graph.
	BuildProgramInternal( source_graph, source_graph.root_node_index );

	global_function->eraseFromParent(); // Kill global function.

	// Fix incomplete typeinfo.
	for( const auto& typeinfo_entry : typeinfo_cache_ )
	{
		if( !typeinfo_entry.second.type.GetLLVMType()->isSized() )
			typeinfo_entry.second.type.GetClassType()->llvm_type->setBody( llvm::ArrayRef<llvm::Type*>() );
	}

	if( build_debug_info_ )
		debug_info_.builder->finalize(); // We must finalize it.

	// Clear internal structures.
	compiled_sources_cache_.clear();
	current_class_table_= nullptr;
	enums_table_.clear();
	template_classes_cache_.clear();
	typeinfo_cache_.clear();
	typeinfo_class_table_.clear();
	debug_info_.builder= nullptr;
	debug_info_.source_file_entries.clear();
	debug_info_.classes_di_cache.clear();
	debug_info_.enums_di_cache.clear();

	NormalizeErrors( global_errors_ );

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
		ProgramStringSet generated_template_things_keys;
		for( const auto& thing_pair : generated_template_things_storage_ )
			generated_template_things_keys.insert(thing_pair.first);
		ProgramStringSet new_generated_template_things_keys= generated_template_things_keys;

		while(!new_generated_template_things_keys.empty())
		{
			for( const std::string& key : new_generated_template_things_keys )
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

	// Take generated template things.
	result.generated_template_things_storage = std::make_unique< ProgramStringMap<Value> >();
	result.generated_template_things_storage->swap( generated_template_things_storage_ );

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

					if( dst_class->completeness == TypeCompleteness::Incomplete && src_class.completeness == TypeCompleteness::Incomplete )
					{} // Ok
					if( dst_class->completeness != TypeCompleteness::Incomplete && src_class.completeness == TypeCompleteness::Incomplete )
					{} // Dst class is complete, so, use it.
					if( dst_class->completeness != TypeCompleteness::Incomplete && src_class.completeness != TypeCompleteness::Incomplete &&
						dst_class->body_file_pos != src_class.body_file_pos )
					{
						// Different bodies from different files.
						REPORT_ERROR( ClassBodyDuplication, dst.GetErrors(), src_class.body_file_pos );
					}
					if(  dst_class->completeness == TypeCompleteness::Incomplete && src_class.completeness != TypeCompleteness::Incomplete )
					{
						// Take body of more complete class and store in destintation class table.
						CopyClass( src_class.forward_declaration_file_pos, src_class_proxy, dst_class_table, dst );
					}

					return;
				}
			}

			if( dst_member->GetFilePos() == src_member.GetFilePos() )
				return; // All ok - things from one source.

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

	// Copy fields.
	copy->members_visibility= src.members_visibility;

	copy->syntax_element= src.syntax_element;
	copy->field_count= src.field_count;
	copy->references_tags_count= src.references_tags_count;
	copy->completeness= src.completeness;

	copy->have_explicit_noncopy_constructors= src.have_explicit_noncopy_constructors;
	copy->is_default_constructible= src.is_default_constructible;
	copy->is_copy_constructible= src.is_copy_constructible;
	copy->have_destructor= src.have_destructor;
	copy->is_copy_assignable= src.is_copy_assignable;
	copy->can_be_constexpr= src.can_be_constexpr;
	copy->have_shared_state= src.have_shared_state;

	copy->forward_declaration_file_pos= src.forward_declaration_file_pos;
	copy->body_file_pos= src.body_file_pos;

	copy->llvm_type= src.llvm_type;
	copy->base_template= src.base_template;
	copy->typeinfo_type= src.typeinfo_type;

	copy->kind= src.kind;
	copy->base_class= src.base_class;
	copy->parents= src.parents;

	copy->virtual_table= src.virtual_table;
	copy->virtual_table_llvm_type= src.virtual_table_llvm_type;
	copy->this_class_virtual_table= src.this_class_virtual_table;
	copy->ancestors_virtual_tables= src.ancestors_virtual_tables;
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
	FilePos fundamental_globals_file_pos;
	fundamental_globals_file_pos.file_index=
	fundamental_globals_file_pos.line=
	fundamental_globals_file_pos.column= std::numeric_limits<unsigned short>::max();

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

Type CodeBuilder::PrepareType(
	const Synt::TypeName& type_name,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	return
		std::visit(
		[&](const auto& t)
		{
			return PrepareType( t, names_scope, function_context );
		},
		type_name);
}

Type CodeBuilder::PrepareType( const Synt::EmptyVariant&, NamesScope&, FunctionContext& )
{
	U_ASSERT(false);
	return invalid_type_;
}

Type CodeBuilder::PrepareType( const Synt::ArrayTypeName& array_type_name, NamesScope& names_scope, FunctionContext& function_context )
{
	Array array_type;
	array_type.type= PrepareType( *array_type_name.element_type, names_scope, function_context );

	const Synt::Expression& num= *array_type_name.size;
	const FilePos num_file_pos= Synt::GetExpressionFilePos( num );

	const Variable size_variable= BuildExpressionCodeEnsureVariable( num, names_scope, function_context );
	if( size_variable.constexpr_value != nullptr )
	{
		if( const FundamentalType* const size_fundamental_type= size_variable.type.GetFundamentalType() )
		{
			if( IsInteger( size_fundamental_type->fundamental_type ) )
			{
				if( llvm::dyn_cast<llvm::UndefValue>(size_variable.constexpr_value) != nullptr )
					array_type.size= 1u;
				else
				{
					const llvm::APInt& size_value= size_variable.constexpr_value->getUniqueInteger();
					if( IsSignedInteger( size_fundamental_type->fundamental_type ) && size_value.isNegative() )
						REPORT_ERROR( ArraySizeIsNegative, names_scope.GetErrors(), num_file_pos );
					else
						array_type.size= size_value.getLimitedValue();
				}
			}
			else
				REPORT_ERROR( ArraySizeIsNotInteger, names_scope.GetErrors(), num_file_pos );
		}
		else
			U_ASSERT( false && "Nonfundamental constexpr? WTF?" );
	}
	else
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), num_file_pos );

	// TODO - generate error, if total size of type (incuding arrays) is more, than half of address space of target architecture.
	array_type.llvm_type= llvm::ArrayType::get( array_type.type.GetLLVMType(), array_type.size );
	return std::move(array_type);
}

Type CodeBuilder::PrepareType( const Synt::TypeofTypeName& typeof_type_name, NamesScope& names_scope, FunctionContext& function_context )
{
	Type result;
	const auto prev_state= SaveInstructionsState( function_context );
	{
		const StackVariablesStorage dummy_stack_variables_storage( function_context );
		const Variable variable= BuildExpressionCodeEnsureVariable( *typeof_type_name.expression, names_scope, function_context );
		result= std::move(variable.type);
	}
	RestoreInstructionsState( function_context, prev_state );
	return result;
}

Type CodeBuilder::PrepareType( const Synt::FunctionTypePtr& function_type_name_ptr, NamesScope& names_scope, FunctionContext& function_context )
{
	const Synt::FunctionType& function_type_name= *function_type_name_ptr;
	FunctionPointer function_pointer_type;
	Function& function_type= function_pointer_type.function;

	if( function_type_name.return_type_ == nullptr )
		function_type.return_type= void_type_for_ret_;
	else
		function_type.return_type= PrepareType( *function_type_name.return_type_, names_scope, function_context );
	function_type.return_value_is_mutable= function_type_name.return_value_mutability_modifier_ == MutabilityModifier::Mutable;
	function_type.return_value_is_reference= function_type_name.return_value_reference_modifier_ == ReferenceModifier::Reference;

	if( !function_type.return_value_is_reference &&
		!( function_type.return_type.GetFundamentalType() != nullptr ||
		   function_type.return_type.GetClassType() != nullptr ||
		   function_type.return_type.GetTupleType() != nullptr ||
		   function_type.return_type.GetEnumType() != nullptr ||
		   function_type.return_type.GetFunctionPointerType() != nullptr ) )
		REPORT_ERROR( NotImplemented, names_scope.GetErrors(), function_type_name.file_pos_, "return value types except fundamentals, enums, classes, function pointers" );

	for( const Synt::FunctionArgument& arg : function_type_name.arguments_ )
	{
		if( IsKeyword( arg.name_ ) )
			REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), arg.file_pos_ );

		function_type.args.emplace_back();
		Function::Arg& out_arg= function_type.args.back();
		out_arg.type= PrepareType( arg.type_, names_scope, function_context );

		out_arg.is_mutable= arg.mutability_modifier_ == MutabilityModifier::Mutable;
		out_arg.is_reference= arg.reference_modifier_ == ReferenceModifier::Reference;

		if( !out_arg.is_reference &&
			!( out_arg.type.GetFundamentalType() != nullptr ||
			   out_arg.type.GetClassType() != nullptr ||
			   out_arg.type.GetTupleType() != nullptr ||
			   out_arg.type.GetEnumType() != nullptr ||
			   out_arg.type.GetFunctionPointerType() != nullptr ) )
			REPORT_ERROR( NotImplemented, names_scope.GetErrors(), arg.file_pos_, "parameters types except fundamentals, classes, enums, functionpointers" );

		ProcessFunctionArgReferencesTags( names_scope.GetErrors(), function_type_name, function_type, arg, out_arg, function_type.args.size() - 1u );
	}

	function_type.unsafe= function_type_name.unsafe_;

	TryGenerateFunctionReturnReferencesMapping( names_scope.GetErrors(), function_type_name, function_type );
	ProcessFunctionTypeReferencesPollution( names_scope.GetErrors(), function_type_name, function_type );

	function_type.llvm_function_type= GetLLVMFunctionType( function_type );
	function_pointer_type.llvm_function_pointer_type= function_type.llvm_function_type->getPointerTo();
	return std::move(function_pointer_type);
}

Type CodeBuilder::PrepareType( const Synt::TupleType& tuple_type_name, NamesScope& names_scope, FunctionContext& function_context )
{
	Tuple tuple;
	tuple.elements.reserve( tuple_type_name.element_types_.size() );

	std::vector<llvm::Type*> elements_llvm_types;
	elements_llvm_types.reserve( tuple_type_name.element_types_.size() );

	for( const Synt::TypeName& element_type_name : tuple_type_name.element_types_ )
	{
		Type element_type= PrepareType( element_type_name, names_scope, function_context );
		if( element_type == invalid_type_ )
			return invalid_type_;

		elements_llvm_types.push_back( element_type.GetLLVMType() );
		tuple.elements.push_back( std::move(element_type) );
	}

	tuple.llvm_type= llvm::StructType::get( llvm_context_, elements_llvm_types );

	return std::move(tuple);
}

Type CodeBuilder::PrepareType( const Synt::NamedTypeName& named_type_name, NamesScope& names_scope, FunctionContext& )
{
	if( const Value* value= ResolveValue( named_type_name.file_pos_, names_scope, named_type_name.name ) )
	{
		if( const Type* const type= value->GetTypeName() )
			return *type;
		else
			REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), named_type_name.file_pos_, named_type_name.name.components.back().name );
	}
	else
		REPORT_ERROR( NameNotFound, names_scope.GetErrors(), named_type_name.file_pos_, named_type_name.name );
	return invalid_type_;
}

llvm::FunctionType* CodeBuilder::GetLLVMFunctionType( const Function& function_type )
{
	ArgsVector<llvm::Type*> args_llvm_types;

	bool first_arg_is_sret= false;
	if( !function_type.return_value_is_reference )
	{
		if( function_type.return_type.GetFundamentalType() != nullptr ||
			function_type.return_type.GetEnumType() != nullptr ||
			function_type.return_type.GetFunctionPointerType() != nullptr )
		{}
		else if( function_type.return_type.GetClassType() != nullptr || function_type.return_type.GetTupleType() != nullptr )
		{
			// Add return-value ponter as "sret" argument for class and tuple types.
			args_llvm_types.push_back( function_type.return_type.GetLLVMType()->getPointerTo() );
			first_arg_is_sret= true;
		}
		else U_ASSERT( false );
	}

	for( const Function::Arg& arg : function_type.args )
	{
		llvm::Type* type= arg.type.GetLLVMType();
		if( arg.is_reference )
			type= type->getPointerTo();
		else
		{
			if( arg.type.GetFundamentalType() != nullptr || arg.type.GetEnumType() != nullptr || arg.type.GetFunctionPointerType() )
			{}
			else if( arg.type.GetClassType() != nullptr || arg.type.GetTupleType() )
			{
				// Mark value-parameters of class and tuple types as pointer.
				type= type->getPointerTo();
			}
			else U_ASSERT( false );
		}
		args_llvm_types.push_back( type );
	}

	llvm::Type* llvm_function_return_type= nullptr;
	if( first_arg_is_sret )
		llvm_function_return_type= fundamental_llvm_types_.void_for_ret;
	else
	{
		llvm_function_return_type= function_type.return_type.GetLLVMType();
		if( function_type.return_value_is_reference )
			llvm_function_return_type= llvm_function_return_type->getPointerTo();
	}

	return llvm::FunctionType::get( llvm_function_return_type, args_llvm_types, false );
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
		llvm::Constant::getIntegerValue( size_type_llvm, llvm::APInt( size_type_llvm->getIntegerBitWidth(), uint64_t(0) ) );
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
	ReferencesGraph& variables_state_copy,
	CodeBuilderErrorsContainer& errors_container,
	const FilePos& file_pos )
{
	// Call destructors in reverse order.
	for( auto it = stack_variables_storage.variables_.rbegin(); it != stack_variables_storage.variables_.rend(); ++it )
	{
		const StackVariablesStorage::NodeAndVariable& stored_variable= *it;

		if( ! variables_state_copy.NodeMoved( stored_variable.first ) )
		{
			if( stored_variable.first->kind == ReferencesGraphNode::Kind::Variable )
			{
				if( variables_state_copy.HaveOutgoingLinks( stored_variable.first ) )
					REPORT_ERROR( DestroyedVariableStillHaveReferences, errors_container, file_pos, stored_variable.first->name );
				const Variable& var= stored_variable.second;
				if( var.type.HaveDestructor() )
					CallDestructor( var.llvm_value, var.type, function_context, errors_container, file_pos );
			}
			variables_state_copy.RemoveNode( stored_variable.first );
		}
	}
}

void CodeBuilder::CallDestructors(
	const StackVariablesStorage& stack_variables_storage,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const FilePos& file_pos )
{
	ReferencesGraph variables_state_copy= function_context.variables_state;
	CallDestructorsImpl( stack_variables_storage, function_context, variables_state_copy, names_scope.GetErrors(), file_pos );
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
	ReferencesGraph variables_state_copy= function_context.variables_state;

	U_ASSERT( !function_context.loops_stack.empty() );

	// Destroy all local variables before "break"/"continue" in all blocks inside loop.
	size_t undestructed_stack_size= function_context.stack_variables_stack.size();
	for(
		auto it= function_context.stack_variables_stack.rbegin();
		it != function_context.stack_variables_stack.rend() &&
		undestructed_stack_size > function_context.loops_stack.back().stack_variables_stack_size;
		++it, --undestructed_stack_size )
	{
		CallDestructorsImpl( **it, function_context, variables_state_copy, names_scope.GetErrors(), file_pos );
	}
}

void CodeBuilder::CallDestructorsBeforeReturn( NamesScope& names_scope, FunctionContext& function_context, const FilePos& file_pos )
{
	ReferencesGraph variables_state_copy= function_context.variables_state;
	// We must call ALL destructors of local variables, arguments, etc before each return.
	for( auto it= function_context.stack_variables_stack.rbegin(); it != function_context.stack_variables_stack.rend(); ++it )
		CallDestructorsImpl( **it, function_context, variables_state_copy, names_scope.GetErrors(), file_pos );
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

	class_->members.ForEachValueInThisScope(
		[&]( const Value& member )
		{
			const ClassField* const field= member.GetClassField();
			if( field == nullptr || field->is_reference || !field->type.HaveDestructor() ||
				field->class_.lock()->class_ != class_ )
				return;

			CallDestructor(
				function_context.llvm_ir_builder.CreateGEP(
					function_context.this_->llvm_value,
					{ GetZeroGEPIndex(), GetFieldGEPIndex(field->index ) } ),
				field->type,
				function_context,
				errors_container,
				file_pos );
		} );
}

size_t CodeBuilder::PrepareFunction(
	NamesScope& names_scope,
	const ClassProxyPtr& base_class,
	OverloadedFunctionsSet& functions_set,
	const Synt::Function& func,
	const bool is_out_of_line_function )
{
	const std::string& func_name= func.name_.components.back().name;
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
				if( named_return_type->name.components.size() == 1u &&
					!named_return_type->name.components.front().have_template_parameters &&
					named_return_type->name.components.front().name == Keywords::auto_ )
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

		if( is_special_method && function_type.return_type != void_type_ )
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

		if( !prev_function->no_mangle && func_variable.no_mangle )
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
			REPORT_ERROR( InvalidReturnTypeForOperator, errors_container, file_pos, void_type_ );
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
				func_variable.no_mangle ? func_name : MangleFunction( parent_names_scope, func_name, function_type ),
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
			llvm_function->addAttribute( 1u, llvm::Attribute::StructRet );

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
	{
		// Set comdat for correct linkage of same functions, emitted in several modules.
		llvm::Comdat* const comdat= module_->getOrInsertComdat( llvm_function->getName() );
		comdat->setSelectionKind( llvm::Comdat::Any );
		llvm_function->setComdat( comdat );
	}

	func_variable.have_body= true;

	// Ensure completeness only for functions body.
	for( const Function::Arg& arg : function_type.args )
	{
		if( !arg.is_reference && arg.type != void_type_ &&
			!EnsureTypeCompleteness( arg.type, TypeCompleteness::Complete ) )
			REPORT_ERROR( UsingIncompleteType, parent_names_scope.GetErrors(), args.front().file_pos_, arg.type );
	}
	if( !function_type.return_value_is_reference && function_type.return_type != void_type_ &&
		!EnsureTypeCompleteness( function_type.return_type, TypeCompleteness::Complete ) )
		REPORT_ERROR( UsingIncompleteType, parent_names_scope.GetErrors(), func_variable.body_file_pos, function_type.return_type );

	NamesScope function_names( "", &parent_names_scope );
	FunctionContext function_context(
		func_variable.return_type_is_auto ? std::optional<Type>(): function_type.return_type,
		function_type.return_value_is_mutable,
		function_type.return_value_is_reference,
		llvm_context_,
		llvm_function );
	const StackVariablesStorage args_storage( function_context );

	SetCurrentDebugLocation( func_variable.body_file_pos, function_context );

	// arg node + optional inner reference variable node.
	ArgsVector< std::pair< ReferencesGraphNodePtr, ReferencesGraphNodePtr > > args_nodes( function_type.args.size() );

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
			var.location= Variable::Location::Pointer;
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
		if( arg.is_reference )
			function_context.variables_state.AddNode( var_node );
		else
			function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( var_node, var ) );
		args_nodes[ arg_number ].first= var_node;
		var.node= var_node;

		if (arg.type.ReferencesTagsCount() > 0u )
		{
			// Create inner node + root variable.
			const auto accesible_variable= std::make_shared<ReferencesGraphNode>( arg_name + " inner variable", ReferencesGraphNode::Kind::Variable );
			function_context.variables_state.AddNode( accesible_variable );

			const auto inner_reference= std::make_shared<ReferencesGraphNode>( arg_name + " inner reference", ReferencesGraphNode::Kind::ReferenceMut );
			function_context.variables_state.SetNodeInnerReference( var_node, inner_reference );
			function_context.variables_state.AddLink( accesible_variable, inner_reference );

			args_nodes[ arg_number ].second= accesible_variable;
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

	// Fill list of allowed for returning references.
	if( function_type.return_value_is_reference || function_type.return_type.ReferencesTagsCount() > 0u )
	{
		for( const Function::ArgReference& arg_and_tag : function_type.return_references )
		{
			const size_t arg_n= arg_and_tag.first;
			U_ASSERT( arg_n < args_nodes.size() );
			if( arg_and_tag.second == Function::c_arg_reference_tag_number )
				function_context.allowed_for_returning_references.emplace( args_nodes[arg_n].first );
			else if( arg_and_tag.second == 0u && args_nodes[arg_n].second != nullptr )
				function_context.allowed_for_returning_references.emplace( args_nodes[arg_n].second );
		}
	}

	if( is_constructor )
	{
		U_ASSERT( base_class != nullptr );
		U_ASSERT( function_context.this_ != nullptr );

		function_context.whole_this_is_unavailable= true;

		if( constructor_initialization_list == nullptr )
		{
			// Create dummy initialization list for constructors without explicit initialization list.
			const Synt::StructNamedInitializer dumy_initialization_list{ FilePos() };

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
			if( function_type.return_type != void_type_for_ret_ && !EnsureTypeCompleteness( function_type.return_type, TypeCompleteness::Complete ) )
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
				if( arg.type != void_type_ && !EnsureTypeCompleteness( arg.type, TypeCompleteness::Complete ) )
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

	// Now, we can check references pollution. After this point only code is destructors calls, which can not link references.
	for( size_t i= 0u; i < function_type.args.size(); ++i )
	{
		const auto& node_pair= args_nodes[i];
		if( node_pair.second != nullptr && function_context.variables_state.GetNodeInnerReference( node_pair.second ) != nullptr )
			REPORT_ERROR( ReferencePollutionForArgReference, function_names.GetErrors(), block->end_file_pos_ );

		const ReferencesGraphNodePtr inner_reference= function_context.variables_state.GetNodeInnerReference( node_pair.first );
		if( inner_reference == nullptr )
			continue;

		for( const ReferencesGraphNodePtr& accesible_variable : function_context.variables_state.GetAllAccessibleVariableNodes_r( inner_reference ) )
		{
			if( accesible_variable == node_pair.second )
				continue;

			std::optional<Function::ArgReference> reference;
			for( size_t j= 0u; j < function_type.args.size(); ++j )
			{
				if( accesible_variable == args_nodes[j].first )
					reference= Function::ArgReference( j, Function::c_arg_reference_tag_number );
				if( accesible_variable == args_nodes[j].second )
					reference= Function::ArgReference( j, 0u );
			}

			if( reference != std::nullopt )
			{
				Function::ReferencePollution pollution;
				pollution.src= *reference;
				pollution.dst.first= i;
				pollution.dst.second= 0u;
				// Currently check both mutable and immutable. TODO - maybe akt more smarter?
				pollution.src_is_mutable= true;
				if( function_type.references_pollution.count( pollution ) != 0u )
					continue;
				pollution.src_is_mutable= false;
				if( function_type.references_pollution.count( pollution ) != 0u )
					continue;
			}
			REPORT_ERROR( UnallowedReferencePollution, function_names.GetErrors(), block->end_file_pos_);
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
		function_context.uninitialized_this_fields.insert( field );
	} // for fields initializers

	ProgramStringSet uninitialized_fields;

	base_class.members.ForEachValueInThisScope(
		[&]( const Value& member )
		{
			const ClassField* const field= member.GetClassField();
			if( field == nullptr )
				return;
			if( field->class_.lock()->class_ != &base_class ) // Parent class field.
				return;

			if( initialized_fields.find( field->syntax_element->name ) == initialized_fields.end() )
				uninitialized_fields.insert( field->syntax_element->name );
		} );

	// Initialize fields, missing in initializer list.
	for( const std::string& field_name : uninitialized_fields )
	{
		const StackVariablesStorage temp_variables_storage( function_context );

		const Value* const class_member=
			base_class.members.GetThisScopeValue( field_name );
		U_ASSERT( class_member != nullptr );
		const ClassField* const field= class_member->GetClassField();
		U_ASSERT( field != nullptr );

		if( field->is_reference )
		{
			if( field->syntax_element->initializer == nullptr )
			{
				REPORT_ERROR( ExpectedInitializer, names_scope.GetErrors(), class_member->GetFilePos(), field_name );
				continue;
			}
			InitializeReferenceClassFieldWithInClassIninitalizer( this_, *field, function_context );
		}
		else
		{
			Variable field_variable;
			field_variable.type= field->type;
			field_variable.location= Variable::Location::Pointer;
			field_variable.value_type= ValueType::Reference;

			field_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( this_.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( field->index ) } );

			if( field->syntax_element->initializer != nullptr )
				InitializeClassFieldWithInClassIninitalizer( field_variable, *field, function_context );
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

		function_context.uninitialized_this_fields.erase( field );

		CallDestructors( temp_variables_storage, names_scope, function_context, Synt::GetInitializerFilePos( field_initializer.initializer ) );
	} // for fields initializers

	SetupVirtualTablePointers( this_.llvm_value, base_class, function_context );
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::Block& block,
	NamesScope& names,
	FunctionContext& function_context )
{
	DebugInfoStartBlock( block.file_pos_, function_context );

	NamesScope block_names( "", &names );
	const StackVariablesStorage block_variables_storage( function_context );

	// Save unsafe flag.
	const bool prev_unsafe= function_context.is_in_unsafe_block;
	if( block.safety_ == Synt::Block::Safety::Unsafe )
	{
		function_context.have_non_constexpr_operations_inside= true; // Unsafe operations can not be used in constexpr functions.
		function_context.is_in_unsafe_block= true;
	}
	else if( block.safety_ == Synt::Block::Safety::Safe )
		function_context.is_in_unsafe_block= false;
	else if( block.safety_ == Synt::Block::Safety::None ) {}
	else U_ASSERT(false);

	BlockBuildInfo block_build_info;
	size_t block_element_index= 0u;
	for( const Synt::BlockElement& block_element : block.elements_ )
	{
		++block_element_index;
		
		const BlockBuildInfo info=
			std::visit(
				[&]( const auto& t )
				{
					SetCurrentDebugLocation( t.file_pos_, function_context );
					return BuildBlockElement( t, block_names, function_context );
				},
				block_element );
		
		if( info.have_terminal_instruction_inside )
		{
			block_build_info.have_terminal_instruction_inside= true;
			break;
		}
	}

	if( block_element_index < block.elements_.size() )
		REPORT_ERROR( UnreachableCode, names.GetErrors(), Synt::GetBlockElementFilePos( block.elements_[ block_element_index ] ) );

	SetCurrentDebugLocation( block.end_file_pos_, function_context );

	// If there are undconditional "break", "continue", "return" operators,
	// we didn`t need call destructors, it must be called in this operators.
	if( !block_build_info.have_terminal_instruction_inside )
		CallDestructors( block_variables_storage, block_names, function_context, block.end_file_pos_ );

	// Restire unsafe flag.
	function_context.is_in_unsafe_block= prev_unsafe;
	
	DebugInfoEndBlock( function_context );

	return block_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::VariablesDeclaration& variables_declaration,
	NamesScope& names,
	FunctionContext& function_context )
{
	const Type type= PrepareType( variables_declaration.type, names, function_context );

	for( const Synt::VariablesDeclaration::VariableEntry& variable_declaration : variables_declaration.variables )
	{
		if( variable_declaration.reference_modifier != ReferenceModifier::Reference ||
			variable_declaration.mutability_modifier == Synt::MutabilityModifier::Constexpr )
		{
			// Full completeness required for value-variables and any constexpr variable.
			if( !EnsureTypeCompleteness( type, TypeCompleteness::Complete ) )
			{
				REPORT_ERROR( UsingIncompleteType, names.GetErrors(), variables_declaration.file_pos_, type );
				continue;
			}
		}
		if( variable_declaration.reference_modifier != ReferenceModifier::Reference && type.IsAbstract() )
			REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), variables_declaration.file_pos_, type );

		if( variable_declaration.reference_modifier != ReferenceModifier::Reference && !type.CanBeConstexpr() )
			function_context.have_non_constexpr_operations_inside= true; // Declaring variable with non-constexpr type in constexpr function not allowed.

		if( IsKeyword( variable_declaration.name ) )
		{
			REPORT_ERROR( UsingKeywordAsName, names.GetErrors(), variables_declaration.file_pos_ );
			continue;
		}

		if( variable_declaration.mutability_modifier == MutabilityModifier::Constexpr && !type.CanBeConstexpr() )
		{
			REPORT_ERROR( InvalidTypeForConstantExpressionVariable, names.GetErrors(), variables_declaration.file_pos_ );
			continue;
		}

		// Destruction frame for temporary variables of initializer expression.
		StackVariablesStorage& prev_variables_storage= *function_context.stack_variables_stack.back();
		const StackVariablesStorage temp_variables_storage( function_context );

		Variable variable;
		variable.type= type;
		variable.location= Variable::Location::Pointer;
		variable.value_type= ValueType::Reference;

		ReferencesGraphNode::Kind node_kind;
		if( variable_declaration.reference_modifier != ReferenceModifier::Reference )
			node_kind= ReferencesGraphNode::Kind::Variable;
		else if( variable_declaration.mutability_modifier == MutabilityModifier::Mutable )
			node_kind= ReferencesGraphNode::Kind::ReferenceMut;
		else
			node_kind= ReferencesGraphNode::Kind::ReferenceImut;
		const auto var_node= std::make_shared<ReferencesGraphNode>( variable_declaration.name, node_kind );

		if( variable_declaration.reference_modifier == ReferenceModifier::None )
		{
			variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType() );
			variable.llvm_value->setName( variable_declaration.name );

			CreateVariableDebugInfo( variable, variable_declaration.name, variable_declaration.file_pos, function_context );

			prev_variables_storage.RegisterVariable( std::make_pair( var_node, variable ) );
			variable.node= var_node;

			if( variable_declaration.initializer != nullptr )
				variable.constexpr_value=
					ApplyInitializer( *variable_declaration.initializer, variable, names, function_context );
			else
				ApplyEmptyInitializer( variable_declaration.name, variable_declaration.file_pos, variable, names, function_context );

			// Make immutable, if needed, only after initialization, because in initialization we need call constructors, which is mutable methods.
			if( variable_declaration.mutability_modifier != MutabilityModifier::Mutable )
				variable.value_type= ValueType::ConstReference;
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			// Mark references immutable before initialization.
			if( variable_declaration.mutability_modifier != MutabilityModifier::Mutable )
				variable.value_type= ValueType::ConstReference;

			if( variable_declaration.initializer == nullptr )
			{
				REPORT_ERROR( ExpectedInitializer, names.GetErrors(), variables_declaration.file_pos_, variable_declaration.name );
				continue;
			}

			const Synt::Expression* initializer_expression= nullptr;
			if( const auto expression_initializer= std::get_if<Synt::ExpressionInitializer>( variable_declaration.initializer.get() ) )
				initializer_expression= &expression_initializer->expression;
			else if( const auto constructor_initializer= std::get_if<Synt::ConstructorInitializer>( variable_declaration.initializer.get() ) )
			{
				if( constructor_initializer->call_operator.arguments_.size() != 1u )
				{
					REPORT_ERROR( ReferencesHaveConstructorsWithExactlyOneParameter, names.GetErrors(), constructor_initializer->file_pos_ );
					continue;
				}
				initializer_expression= &constructor_initializer->call_operator.arguments_.front();
			}
			else
			{
				REPORT_ERROR( UnsupportedInitializerForReference, names.GetErrors(), variable_declaration.file_pos );
				continue;
			}

			const Variable expression_result= BuildExpressionCodeEnsureVariable( *initializer_expression, names, function_context );
			if( !ReferenceIsConvertible( expression_result.type, variable.type, names.GetErrors(), variables_declaration.file_pos_ ) )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), variables_declaration.file_pos_, variable.type, expression_result.type );
				continue;
			}

			if( expression_result.value_type == ValueType::Value )
			{
				REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), variables_declaration.file_pos_ );
				continue;
			}
			if( expression_result.value_type == ValueType::ConstReference && variable.value_type == ValueType::Reference )
			{
				REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), variable_declaration.file_pos );
				continue;
			}

			// TODO - maybe make copy of varaible address in new llvm register?
			llvm::Value* result_ref= expression_result.llvm_value;
			if( variable.type != expression_result.type )
				result_ref= CreateReferenceCast( result_ref, expression_result.type, variable.type, function_context );
			variable.llvm_value= result_ref;
			variable.constexpr_value= expression_result.constexpr_value;

			prev_variables_storage.RegisterVariable( std::make_pair( var_node, variable ) );
			variable.node= var_node;

			const bool is_mutable= variable.value_type == ValueType::Reference;
			if( expression_result.node != nullptr )
			{
				if( is_mutable )
				{
					if( function_context.variables_state.HaveOutgoingLinks( expression_result.node ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), variable_declaration.file_pos, expression_result.node->name );
				}
				else if( function_context.variables_state.HaveOutgoingMutableNodes( expression_result.node ) )
					REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), variable_declaration.file_pos, expression_result.node->name );
				function_context.variables_state.AddLink( expression_result.node, var_node );
			}
		}
		else U_ASSERT(false);

		if( variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
			variable.constexpr_value == nullptr )
		{
			REPORT_ERROR( VariableInitializerIsNotConstantExpression, names.GetErrors(), variable_declaration.file_pos );
			continue;
		}

		// Reset constexpr initial value for mutable variables.
		if( variable.value_type != ValueType::ConstReference )
			variable.constexpr_value= nullptr;

		if( NameShadowsTemplateArgument( variable_declaration.name, names ) )
		{
			REPORT_ERROR( DeclarationShadowsTemplateArgument, names.GetErrors(), variables_declaration.file_pos_, variable_declaration.name );
			continue;
		}

		const Value* const inserted_value=
			names.AddName( variable_declaration.name, Value( variable, variable_declaration.file_pos ) );
		if( inserted_value == nullptr )
		{
			REPORT_ERROR( Redefinition, names.GetErrors(), variables_declaration.file_pos_, variable_declaration.name );
			continue;
		}

		// After lock of references we can call destructors.
		CallDestructors( temp_variables_storage, names, function_context, variable_declaration.file_pos );
	} // for variables
	
	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::AutoVariableDeclaration& auto_variable_declaration,
	NamesScope& names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of initializer expression.
	StackVariablesStorage& prev_variables_storage= *function_context.stack_variables_stack.back();
	const StackVariablesStorage temp_variables_storage( function_context );

	const Variable initializer_experrsion= BuildExpressionCodeEnsureVariable( auto_variable_declaration.initializer_expression, names, function_context );

	{ // Check expression type. Expression can have exotic types, such "Overloading functions set", "class name", etc.
		const bool type_is_ok=
			initializer_experrsion.type.GetFundamentalType() != nullptr ||
			initializer_experrsion.type.GetArrayType() != nullptr ||
			initializer_experrsion.type.GetTupleType() != nullptr ||
			initializer_experrsion.type.GetClassType() != nullptr ||
			initializer_experrsion.type.GetEnumType() != nullptr ||
			initializer_experrsion.type.GetFunctionPointerType() != nullptr;
		if( !type_is_ok || initializer_experrsion.type == invalid_type_ )
		{
			REPORT_ERROR( InvalidTypeForAutoVariable, names.GetErrors(), auto_variable_declaration.file_pos_, initializer_experrsion.type );
			return BlockBuildInfo();
		}
	}

	Variable variable;
	variable.type= initializer_experrsion.type;
	variable.value_type= auto_variable_declaration.mutability_modifier == MutabilityModifier::Mutable ? ValueType::Reference : ValueType::ConstReference;
	variable.location= Variable::Location::Pointer;

	ReferencesGraphNode::Kind node_kind;
	if( auto_variable_declaration.reference_modifier != ReferenceModifier::Reference )
		node_kind= ReferencesGraphNode::Kind::Variable;
	else if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Mutable )
		node_kind= ReferencesGraphNode::Kind::ReferenceMut;
	else
		node_kind= ReferencesGraphNode::Kind::ReferenceImut;
	const auto var_node= std::make_shared<ReferencesGraphNode>( auto_variable_declaration.name, node_kind );

	if( auto_variable_declaration.reference_modifier != ReferenceModifier::Reference ||
		auto_variable_declaration.mutability_modifier == Synt::MutabilityModifier::Constexpr )
	{
		// Full completeness required for value-variables and any constexpr variable.
		if( !EnsureTypeCompleteness( variable.type, TypeCompleteness::Complete ) )
		{
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), auto_variable_declaration.file_pos_, variable.type );
			return BlockBuildInfo();
		}
	}
	if( auto_variable_declaration.reference_modifier != ReferenceModifier::Reference && variable.type.IsAbstract() )
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), auto_variable_declaration.file_pos_, variable.type );

	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr && !variable.type.CanBeConstexpr() )
	{
		REPORT_ERROR( InvalidTypeForConstantExpressionVariable, names.GetErrors(), auto_variable_declaration.file_pos_ );
		return BlockBuildInfo();
	}

	if( auto_variable_declaration.reference_modifier == ReferenceModifier::Reference )
	{
		if( initializer_experrsion.value_type == ValueType::ConstReference && variable.value_type != ValueType::ConstReference )
		{
			REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), auto_variable_declaration.file_pos_ );
			return BlockBuildInfo();
		}

		variable.llvm_value= initializer_experrsion.llvm_value;
		variable.constexpr_value= initializer_experrsion.constexpr_value;

		if( initializer_experrsion.value_type == ValueType::Value )
		{
			if( auto_variable_declaration.lock_temps )
			{
				// In case of "lock_temps" we can bind "Value" to "Reference" or "ConstReference".
				if( initializer_experrsion.location == Variable::Location::LLVMRegister )
				{
					llvm::Value* const storage= function_context.alloca_ir_builder.CreateAlloca( initializer_experrsion.type.GetLLVMType() );
					function_context.llvm_ir_builder.CreateStore( initializer_experrsion.llvm_value, storage );
					variable.llvm_value= storage;
				}
			}
			else
			{
				REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), auto_variable_declaration.file_pos_ );
				return BlockBuildInfo();
			}
		}

		prev_variables_storage.RegisterVariable( std::make_pair( var_node, variable ) );
		variable.node= var_node;

		const bool is_mutable= variable.value_type == ValueType::Reference;
		if( initializer_experrsion.node != nullptr )
		{
			if( is_mutable )
			{
				if( function_context.variables_state.HaveOutgoingLinks( initializer_experrsion.node ) )
					REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), auto_variable_declaration.file_pos_, initializer_experrsion.node->name );
			}
			else if( function_context.variables_state.HaveOutgoingMutableNodes( initializer_experrsion.node ) )
				REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), auto_variable_declaration.file_pos_, initializer_experrsion.node->name );
			function_context.variables_state.AddLink( initializer_experrsion.node, var_node );
		}
	}
	else if( auto_variable_declaration.reference_modifier == ReferenceModifier::None )
	{	
		if( !variable.type.CanBeConstexpr() )
			function_context.have_non_constexpr_operations_inside= true; // Declaring variable with non-constexpr type in constexpr function not allowed.

		variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType(), nullptr, auto_variable_declaration.name );

		CreateVariableDebugInfo( variable, auto_variable_declaration.name, auto_variable_declaration.file_pos_, function_context );

		prev_variables_storage.RegisterVariable( std::make_pair( var_node, variable ) );
		variable.node= var_node;

		if( initializer_experrsion.value_type == ValueType::Value )
		{
			const ReferencesGraphNodePtr& variable_for_move= initializer_experrsion.node;
			if( variable_for_move != nullptr )
			{
				U_ASSERT(variable_for_move->kind == ReferencesGraphNode::Kind::Variable );

				const ReferencesGraphNodePtr initializer_expression_inner_node= function_context.variables_state.GetNodeInnerReference( variable_for_move );
				if( initializer_expression_inner_node != nullptr )
				{
					const ReferencesGraphNodePtr inner_reference= std::make_shared<ReferencesGraphNode>(
						"var" + auto_variable_declaration.name + " inner node",
						initializer_expression_inner_node->kind);
					function_context.variables_state.SetNodeInnerReference( var_node, inner_reference );
					function_context.variables_state.AddLink( initializer_expression_inner_node, inner_reference );
				}
				function_context.variables_state.MoveNode( variable_for_move );
			}

			CopyBytes( initializer_experrsion.llvm_value, variable.llvm_value, variable.type, function_context );
		}
		else
		{
			if( !variable.type.IsCopyConstructible() )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), auto_variable_declaration.file_pos_, variable.type );
				return BlockBuildInfo();
			}
			BuildCopyConstructorPart(
				variable.llvm_value, initializer_experrsion.llvm_value,
				variable.type,
				function_context );

			const ReferencesGraphNodePtr& src_node= initializer_experrsion.node;
			if( src_node != nullptr )
			{
				const auto src_node_inner_references= function_context.variables_state.GetAllAccessibleInnerNodes_r( src_node );
				if( !src_node_inner_references.empty() )
				{
					bool node_is_mutable= false;
					for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
						node_is_mutable= node_is_mutable || src_node_inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut;

					const auto dst_node_inner_reference= std::make_shared<ReferencesGraphNode>( var_node->name + " inner variable", node_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
					function_context.variables_state.SetNodeInnerReference( var_node, dst_node_inner_reference );
					for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
						function_context.variables_state.AddLink( src_node_inner_reference, dst_node_inner_reference );
				}
			}
		}
		// constexpr preserved for move/copy.
		variable.constexpr_value= initializer_experrsion.constexpr_value;
	}
	else U_ASSERT(false);

	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr && variable.constexpr_value == nullptr )
	{
		REPORT_ERROR( VariableInitializerIsNotConstantExpression, names.GetErrors(), auto_variable_declaration.file_pos_ );
		return BlockBuildInfo();
	}

	// Reset constexpr initial value for mutable variables.
	if( variable.value_type != ValueType::ConstReference )
		variable.constexpr_value= nullptr;

	if( NameShadowsTemplateArgument( auto_variable_declaration.name, names ) )
	{
		REPORT_ERROR( DeclarationShadowsTemplateArgument, names.GetErrors(), auto_variable_declaration.file_pos_, auto_variable_declaration.name );
		return BlockBuildInfo();
	}

	const Value* const inserted_value=
		names.AddName( auto_variable_declaration.name, Value( variable, auto_variable_declaration.file_pos_ ) );
	if( inserted_value == nullptr )
		REPORT_ERROR( Redefinition, names.GetErrors(), auto_variable_declaration.file_pos_, auto_variable_declaration.name );

	if( auto_variable_declaration.lock_temps )
	{
		const auto accesible_variable_nodes= function_context.variables_state.GetAllAccessibleVariableNodes_r( var_node );
		std::unordered_set<ReferencesGraphNodePtr> indirect_accesible_variable_nodes;

		// Get accesible by inner references variables. Currently, we have onyl 1 level of indirection.
		for( const ReferencesGraphNodePtr& accesible_variable_node : accesible_variable_nodes )
		{
			if( const ReferencesGraphNodePtr& inner_reference = function_context.variables_state.GetNodeInnerReference( accesible_variable_node ) )
			{
				const auto accesible_variable_nodes2= function_context.variables_state.GetAllAccessibleVariableNodes_r( inner_reference );
				indirect_accesible_variable_nodes.insert( accesible_variable_nodes2.begin(), accesible_variable_nodes2.end() );
			}
		}

		std::vector<StackVariablesStorage::NodeAndVariable>& src_storage= function_context.stack_variables_stack.back()->variables_;
		std::vector<StackVariablesStorage::NodeAndVariable>& dst_storage= function_context.stack_variables_stack[ function_context.stack_variables_stack.size() - 2u ]->variables_;
		for( size_t i = 0u; i < src_storage.size(); )
		{
			if( src_storage[i].first->kind == ReferencesGraphNode::Kind::Variable &&
				( accesible_variable_nodes .count( src_storage[i].first ) != 0 ||
				indirect_accesible_variable_nodes.count( src_storage[i].first ) != 0 ) )
			{
				dst_storage.insert( dst_storage.begin() + std::ptrdiff_t( dst_storage.size() - 1u ), src_storage[i] ); // insert before declared variable storage.
				src_storage.erase( src_storage.begin() + std::ptrdiff_t(i) );
			}
			else
				++i;
		}
	}

	// After lock of references we can call destructors.
	CallDestructors( temp_variables_storage, names, function_context, auto_variable_declaration.file_pos_ );
	
	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::ReturnOperator& return_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	BlockBuildInfo block_info;
	block_info.have_terminal_instruction_inside= true;
	
	if( std::get_if<Synt::EmptyVariant>(&return_operator.expression_) != nullptr )
	{
		if( function_context.return_type == std::nullopt )
		{
			if( function_context.return_value_is_reference )
			{
				REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), return_operator.file_pos_ );
				return block_info;
			}

			if( function_context.deduced_return_type == std::nullopt )
				function_context.deduced_return_type = void_type_for_ret_;
			else if( *function_context.deduced_return_type != void_type_for_ret_ )
				REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.file_pos_, *function_context.deduced_return_type, void_type_for_ret_ );
			return block_info;
		}

		if( !( function_context.return_type == void_type_ && !function_context.return_value_is_reference ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.file_pos_, void_type_, *function_context.return_type );
			return block_info;
		}

		CallDestructorsBeforeReturn( names, function_context, return_operator.file_pos_ );

		if( function_context.destructor_end_block == nullptr )
			function_context.llvm_ir_builder.CreateRetVoid();
		else
		{
			// In explicit destructor, break to block with destructor calls for class members.
			function_context.llvm_ir_builder.CreateBr( function_context.destructor_end_block );
		}

		return block_info;
	}

	// Destruction frame for temporary variables of result expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	const Variable expression_result= BuildExpressionCodeEnsureVariable( return_operator.expression_, names, function_context );
	if( expression_result.type == invalid_type_ )
	{
		// Add "ret void", because we do not need to break llvm basic blocks structure.
		function_context.llvm_ir_builder.CreateRetVoid();
		return block_info;
	}

	// For functions with "auto" on return type use type of first return expression.
	if( function_context.return_type == std::nullopt )
	{
		if( function_context.deduced_return_type == std::nullopt )
			function_context.deduced_return_type = expression_result.type;
		else if( *function_context.deduced_return_type != expression_result.type )
			REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.file_pos_, *function_context.deduced_return_type, expression_result.type );
		return block_info;
	}

	if( function_context.return_value_is_reference )
	{
		if( !ReferenceIsConvertible( expression_result.type, *function_context.return_type, names.GetErrors(), return_operator.file_pos_ ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.file_pos_, *function_context.return_type, expression_result.type );
			return block_info;
		}

		if( expression_result.value_type == ValueType::Value )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), return_operator.file_pos_ );
			return block_info;
		}
		if( expression_result.value_type == ValueType::ConstReference && function_context.return_value_is_mutable )
		{
			REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), return_operator.file_pos_ );
			return block_info;
		}

		{ // Lock references to return value variables.
			ReferencesGraphNodeHolder return_value_lock(
				std::make_shared<ReferencesGraphNode>(
					"ret result",
					function_context.return_value_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut ),
				function_context );
			if( expression_result.node != nullptr )
				function_context.variables_state.AddLink( expression_result.node, return_value_lock.Node() );

			CallDestructorsBeforeReturn( names, function_context, return_operator.file_pos_ );
		} // Reset locks AFTER destructors call. We must get error in case of returning of reference to stack variable or value-argument.

		// Check correctness of returning reference.
		if( expression_result.node != nullptr )
		{
			for( const ReferencesGraphNodePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes_r( expression_result.node ) )
			{
				if( function_context.allowed_for_returning_references.count( var_node ) == 0 )
					REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), return_operator.file_pos_ );
			}
		}

		llvm::Value* ret_value= expression_result.llvm_value;
		if( expression_result.type != function_context.return_type )
			ret_value= CreateReferenceCast( ret_value, expression_result.type, *function_context.return_type, function_context );
		function_context.llvm_ir_builder.CreateRet( ret_value );
	}
	else
	{
		if( expression_result.type != function_context.return_type )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.file_pos_, *function_context.return_type, expression_result.type );
			return block_info;
		}

		if( expression_result.type.ReferencesTagsCount() > 0u )
		{
			// Check correctness of returning references.
			if( expression_result.node != nullptr )
			{
				if( const ReferencesGraphNodePtr& inner_reference = function_context.variables_state.GetNodeInnerReference( expression_result.node ) )
				{
					for( const ReferencesGraphNodePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes_r( inner_reference ) )
					{
						if( function_context.allowed_for_returning_references.count( var_node ) == 0 )
							REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), return_operator.file_pos_ );
					}
				}
			}
		}

		if( function_context.s_ret_ != nullptr )
		{
			if( expression_result.value_type == ValueType::Value )
			{
				if( expression_result.node != nullptr )
					function_context.variables_state.MoveNode( expression_result.node );
				CopyBytes( expression_result.llvm_value, function_context.s_ret_, *function_context.return_type, function_context );
			}
			else
				BuildCopyConstructorPart(
					function_context.s_ret_,
					CreateReferenceCast( expression_result.llvm_value, expression_result.type, *function_context.return_type, function_context ),
					*function_context.return_type,
					function_context );

			CallDestructorsBeforeReturn( names, function_context, return_operator.file_pos_ );
			function_context.llvm_ir_builder.CreateRetVoid();
		}
		else
		{
			// Now we can return by value only fundamentals end enums.
			U_ASSERT( expression_result.type.GetFundamentalType() != nullptr || expression_result.type.GetEnumType() != nullptr || expression_result.type.GetFunctionPointerType() != nullptr );

			if( expression_result.type == void_type_ || expression_result.type == void_type_for_ret_ )
			{
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
				// We must read return value before call of destructors.
				llvm::Value* const value_for_return= CreateMoveToLLVMRegisterInstruction( expression_result, function_context );

				CallDestructorsBeforeReturn( names, function_context, return_operator.file_pos_ );
				function_context.llvm_ir_builder.CreateRet( value_for_return );
			}
		}
	}
	
	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::ForOperator& for_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	const StackVariablesStorage temp_variables_storage( function_context );
	const Variable sequence_expression= BuildExpressionCodeEnsureVariable( for_operator.sequence_, names, function_context );

	std::optional<ReferencesGraphNodeHolder> sequence_lock;
	if( sequence_expression.node != nullptr )
		sequence_lock.emplace(
			std::make_shared<ReferencesGraphNode>(
				sequence_expression.node->name + " seequence lock",
				sequence_expression.value_type == ValueType::Reference ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut ),
			function_context );

	if( const Tuple* const tuple_type= sequence_expression.type.GetTupleType() )
	{
		llvm::BasicBlock* const finish_basic_block= tuple_type->elements.empty() ? nullptr : llvm::BasicBlock::Create( llvm_context_ );

		U_ASSERT( sequence_expression.location == Variable::Location::Pointer );
		for( const Type& element_type : tuple_type->elements )
		{
			const size_t element_index= size_t( &element_type - tuple_type->elements.data() );
			NamesScope loop_names( "", &names );
			const StackVariablesStorage element_pass_variables_storage( function_context );

			Variable variable;
			variable.type= element_type;
			variable.value_type= for_operator.mutability_modifier_ == MutabilityModifier::Mutable ? ValueType::Reference : ValueType::ConstReference;

			ReferencesGraphNode::Kind node_kind;
			if( for_operator.reference_modifier_ != ReferenceModifier::Reference )
				node_kind= ReferencesGraphNode::Kind::Variable;
			else if( for_operator.mutability_modifier_ == MutabilityModifier::Mutable )
				node_kind= ReferencesGraphNode::Kind::ReferenceMut;
			else
				node_kind= ReferencesGraphNode::Kind::ReferenceImut;
			const auto var_node= std::make_shared<ReferencesGraphNode>( for_operator.loop_variable_name_, node_kind );

			if( for_operator.reference_modifier_ == ReferenceModifier::Reference )
			{
				if( for_operator.mutability_modifier_ == MutabilityModifier::Mutable && sequence_expression.value_type != ValueType::Reference )
				{
					REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), for_operator.file_pos_ );
					continue;
				}

				variable.llvm_value= function_context.llvm_ir_builder.CreateGEP( sequence_expression.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( element_index ) } );
				function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( var_node, variable ) );
				variable.node= var_node;

				const bool is_mutable= variable.value_type == ValueType::Reference;
				if( sequence_lock != std::nullopt )
				{
					if( is_mutable )
					{
						if( function_context.variables_state.HaveOutgoingLinks( sequence_expression.node ) )
							REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), for_operator.file_pos_, sequence_expression.node->name );
					}
					else if( function_context.variables_state.HaveOutgoingMutableNodes( sequence_expression.node ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), for_operator.file_pos_, sequence_expression.node->name );
					function_context.variables_state.AddLink( sequence_lock->Node(), var_node );
				}
			}
			else
			{
				if( !EnsureTypeCompleteness( element_type, TypeCompleteness::Complete ) )
				{
					REPORT_ERROR( UsingIncompleteType, names.GetErrors(), for_operator.file_pos_, element_type );
					continue;
				}
				if( !element_type.IsCopyConstructible() )
				{
					REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), for_operator.file_pos_, element_type );
					continue;
				}

				variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( element_type.GetLLVMType(), nullptr, for_operator.loop_variable_name_ + std::to_string(element_index) );
				function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( var_node, variable ) );
				variable.node= var_node;

				const ReferencesGraphNodePtr& src_node= sequence_expression.node;
				const ReferencesGraphNodePtr& dst_node= var_node;
				if( src_node != nullptr && dst_node != nullptr && variable.type.ReferencesTagsCount() > 0u )
				{
					const auto src_node_inner_references= function_context.variables_state.GetAllAccessibleInnerNodes_r( src_node );
					if( !src_node_inner_references.empty() )
					{
						bool node_is_mutable= false;
						for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
							node_is_mutable= node_is_mutable || src_node_inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut;

						const auto dst_node_inner_reference= std::make_shared<ReferencesGraphNode>( dst_node->name + " inner variable", node_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
						function_context.variables_state.SetNodeInnerReference( dst_node, dst_node_inner_reference );
						for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
							function_context.variables_state.AddLink( src_node_inner_reference, dst_node_inner_reference );
					}
				}

				BuildCopyConstructorPart(
					variable.llvm_value,
					function_context.llvm_ir_builder.CreateGEP( sequence_expression.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( element_index ) } ),
					element_type,
					function_context );
			}

			loop_names.AddName( for_operator.loop_variable_name_, Value( std::move(variable), for_operator.file_pos_ ) );


			llvm::BasicBlock* const next_basic_block=
				( element_index + 1u == tuple_type->elements.size() )
					? finish_basic_block
					: llvm::BasicBlock::Create( llvm_context_ );
			function_context.loops_stack.emplace_back();
			function_context.loops_stack.back().block_for_continue= next_basic_block;
			function_context.loops_stack.back().block_for_break= finish_basic_block;
			function_context.loops_stack.back().stack_variables_stack_size= function_context.stack_variables_stack.size();

			// TODO - create template errors context.
			const BlockBuildInfo block_build_info= BuildBlockElement( for_operator.block_, loop_names, function_context );
			CallDestructors( element_pass_variables_storage, names, function_context, for_operator.file_pos_ );

			// Overloading resolution uses addresses of syntax elements as keys. Reset it, because we use same syntax elements multiple times.
			function_context.overloading_resolution_cache.clear();

			function_context.loops_stack.pop_back();

			if( !block_build_info.have_terminal_instruction_inside )
				function_context.llvm_ir_builder.CreateBr( next_basic_block );

			function_context.function->getBasicBlockList().push_back( next_basic_block );
			function_context.llvm_ir_builder.SetInsertPoint( next_basic_block );
		}
	}
	else
	{
		// TODO - support array types.
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), for_operator.file_pos_, sequence_expression.type );
		return BlockBuildInfo();
	}

	CallDestructors( temp_variables_storage, names, function_context, for_operator.file_pos_ );
	
	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::WhileOperator& while_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	llvm::BasicBlock* const test_block= llvm::BasicBlock::Create( llvm_context_ );

	// Break to test block. We must push terminal instruction at and of current block.
	function_context.llvm_ir_builder.CreateBr( test_block );

	// Test block code.
	function_context.function->getBasicBlockList().push_back( test_block );
	function_context.llvm_ir_builder.SetInsertPoint( test_block );

	const StackVariablesStorage temp_variables_storage( function_context );
	const Variable condition_expression= BuildExpressionCodeEnsureVariable( while_operator.condition_, names, function_context );

	ReferencesGraph variables_state_before_while= function_context.variables_state;

	const FilePos condition_file_pos= Synt::GetExpressionFilePos( while_operator.condition_ );
	if( condition_expression.type != bool_type_ )
	{
		REPORT_ERROR( TypesMismatch,
				names.GetErrors(),
				condition_file_pos,
				bool_type_,
				condition_expression.type );
		return BlockBuildInfo();
	}

	llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
	CallDestructors( temp_variables_storage, names, function_context, condition_file_pos );

	llvm::BasicBlock* const while_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const block_after_while= llvm::BasicBlock::Create( llvm_context_ );
	function_context.llvm_ir_builder.CreateCondBr( condition_in_register, while_block, block_after_while );

	// While block code.

	function_context.loops_stack.emplace_back();
	function_context.loops_stack.back().block_for_break= block_after_while;
	function_context.loops_stack.back().block_for_continue= test_block;
	function_context.loops_stack.back().stack_variables_stack_size= function_context.stack_variables_stack.size();

	function_context.function->getBasicBlockList().push_back( while_block );
	function_context.llvm_ir_builder.SetInsertPoint( while_block );

	BuildBlockElement( while_operator.block_, names, function_context );
	function_context.llvm_ir_builder.CreateBr( test_block );

	function_context.loops_stack.pop_back();

	// Block after while code.
	function_context.function->getBasicBlockList().push_back( block_after_while );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_while );

	const auto errors= ReferencesGraph::CheckWhileBlokVariablesState( variables_state_before_while, function_context.variables_state, while_operator.block_.end_file_pos_ );
	names.GetErrors().insert( names.GetErrors().end(), errors.begin(), errors.end() );
	
	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::BreakOperator& break_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	BlockBuildInfo block_info;
	block_info.have_terminal_instruction_inside= true;
	
	if( function_context.loops_stack.empty() )
	{
		REPORT_ERROR( BreakOutsideLoop, names.GetErrors(), break_operator.file_pos_ );
		return block_info;
	}
	U_ASSERT( function_context.loops_stack.back().block_for_break != nullptr );

	CallDestructorsForLoopInnerVariables( names, function_context, break_operator.file_pos_ );
	function_context.llvm_ir_builder.CreateBr( function_context.loops_stack.back().block_for_break );
	
	
	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::ContinueOperator& continue_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	BlockBuildInfo block_info;
	block_info.have_terminal_instruction_inside= true;

	if( function_context.loops_stack.empty() )
	{
		REPORT_ERROR( ContinueOutsideLoop, names.GetErrors(), continue_operator.file_pos_ );
		return block_info;
	}
	U_ASSERT( function_context.loops_stack.back().block_for_continue != nullptr );

	CallDestructorsForLoopInnerVariables( names, function_context, continue_operator.file_pos_ );
	function_context.llvm_ir_builder.CreateBr( function_context.loops_stack.back().block_for_continue );

	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::IfOperator& if_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	U_ASSERT( !if_operator.branches_.empty() );

	BlockBuildInfo if_operator_blocks_build_info;
	if_operator_blocks_build_info.have_terminal_instruction_inside= true;

	// TODO - optimize this method. Make less basic blocks.
	//

	llvm::BasicBlock* const block_after_if= llvm::BasicBlock::Create( llvm_context_ );

	llvm::BasicBlock* next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
	// Break to first condition. We must push terminal instruction at end of current block.
	function_context.llvm_ir_builder.CreateBr( next_condition_block );

	ReferencesGraph conditions_variable_state= function_context.variables_state;
	std::vector<ReferencesGraph> bracnhes_variables_state( if_operator.branches_.size() );

	for( unsigned int i= 0u; i < if_operator.branches_.size(); i++ )
	{
		const Synt::IfOperator::Branch& branch= if_operator.branches_[i];

		llvm::BasicBlock* const body_block= llvm::BasicBlock::Create( llvm_context_ );
		llvm::BasicBlock* const current_condition_block= next_condition_block;

		if( i + 1u < if_operator.branches_.size() )
			next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
		else
			next_condition_block= block_after_if;

		// Build condition block.
		function_context.function->getBasicBlockList().push_back( current_condition_block );
		function_context.llvm_ir_builder.SetInsertPoint( current_condition_block );

		if( std::get_if<Synt::EmptyVariant>(&branch.condition) != nullptr )
		{
			U_ASSERT( i + 1u == if_operator.branches_.size() );

			// Make empty condition block - move to it unconditional break to body.
			function_context.llvm_ir_builder.CreateBr( body_block );
		}
		else
		{
			function_context.variables_state= conditions_variable_state;
			{
				const StackVariablesStorage temp_variables_storage( function_context );
				const Variable condition_expression= BuildExpressionCodeEnsureVariable( branch.condition, names, function_context );
				if( condition_expression.type != bool_type_ )
				{
					REPORT_ERROR( TypesMismatch,
						names.GetErrors(),
						Synt::GetExpressionFilePos( branch.condition ),
						bool_type_,
						condition_expression.type );

					// Create instruction even in case of error, because we needs to store basic blocs somewhere.
					function_context.llvm_ir_builder.CreateCondBr( llvm::UndefValue::get( fundamental_llvm_types_.bool_ ), body_block, next_condition_block );
				}
				else
				{
					llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
					CallDestructors( temp_variables_storage, names, function_context, Synt::GetExpressionFilePos( branch.condition ) );

					function_context.llvm_ir_builder.CreateCondBr( condition_in_register, body_block, next_condition_block );
				}
			}
			conditions_variable_state= function_context.variables_state;
		}

		// Make body block code.
		function_context.function->getBasicBlockList().push_back( body_block );
		function_context.llvm_ir_builder.SetInsertPoint( body_block );

		const BlockBuildInfo block_build_info= BuildBlockElement( branch.block, names, function_context );

		if( !block_build_info.have_terminal_instruction_inside )
		{
			// Create break instruction, only if block does not contains terminal instructions.
			if_operator_blocks_build_info.have_terminal_instruction_inside= false;
			function_context.llvm_ir_builder.CreateBr( block_after_if );
		}

		bracnhes_variables_state[i]= function_context.variables_state;
		function_context.variables_state= conditions_variable_state;
	}

	U_ASSERT( next_condition_block == block_after_if );

	if( std::get_if<Synt::EmptyVariant>( &if_operator.branches_.back().condition ) == nullptr ) // Have no unconditional "else" at end.
	{
		bracnhes_variables_state.push_back( conditions_variable_state );
		if_operator_blocks_build_info.have_terminal_instruction_inside= false;
	}

	function_context.variables_state= MergeVariablesStateAfterIf( bracnhes_variables_state, names.GetErrors(), if_operator.end_file_pos_ );

	// Block after if code.
	if( if_operator_blocks_build_info.have_terminal_instruction_inside )
		delete block_after_if;
	else
	{
		function_context.function->getBasicBlockList().push_back( block_after_if );
		function_context.llvm_ir_builder.SetInsertPoint( block_after_if );
	}

	return if_operator_blocks_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::StaticIfOperator& static_if_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	const auto& branches= static_if_operator.if_operator_.branches_;
	for( unsigned int i= 0u; i < branches.size(); i++ )
	{
		const auto& branch= branches[i];
		if( std::get_if<Synt::EmptyVariant>(&branch.condition) == nullptr )
		{
			const Synt::Expression& condition= branch.condition;
			const FilePos condition_file_pos= Synt::GetExpressionFilePos( condition );

			const StackVariablesStorage temp_variables_storage( function_context );

			const Variable condition_expression= BuildExpressionCodeEnsureVariable( condition, names, function_context );
			if( condition_expression.type != bool_type_ )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), condition_file_pos, bool_type_, condition_expression.type );
				continue;
			}
			if( condition_expression.constexpr_value == nullptr )
			{
				REPORT_ERROR( ExpectedConstantExpression, names.GetErrors(), condition_file_pos );
				continue;
			}

			if( condition_expression.constexpr_value->getUniqueInteger().getLimitedValue() != 0u )
				return BuildBlockElement( branch.block, names, function_context ); // Ok, this static if produdes block.

			CallDestructors( temp_variables_storage, names, function_context, condition_file_pos );
		}
		else
		{
			U_ASSERT( i == branches.size() - 1u );
			return BuildBlockElement( branch.block, names, function_context );
		}
	}

	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::SingleExpressionOperator& single_expression_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	BuildExpressionCodeAndDestroyTemporaries( single_expression_operator.expression_, names, function_context );
	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::AssignmentOperator& assignment_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expressions.
	const StackVariablesStorage temp_variables_storage( function_context );

	if(
		TryCallOverloadedBinaryOperator(
			OverloadedOperator::Assign,
			assignment_operator,
			assignment_operator.l_value_,
			assignment_operator.r_value_,
			true, // evaluate args in reverse order
			assignment_operator.file_pos_,
			names,
			function_context ) == std::nullopt )
	{ // Here process default assignment operator for fundamental types.
		// Evaluate right part
		Variable r_var= BuildExpressionCodeEnsureVariable( assignment_operator.r_value_, names, function_context );

		if( r_var.type.GetFundamentalType() != nullptr || r_var.type.GetEnumType() != nullptr || r_var.type.GetFunctionPointerType() != nullptr )
		{
			// We must read value, because referenced by reference value may be changed in l_var evaluation.
			if( r_var.location != Variable::Location::LLVMRegister )
			{
				r_var.llvm_value= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
				r_var.location= Variable::Location::LLVMRegister;
			}
			r_var.value_type= ValueType::Value;
		}
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), assignment_operator.file_pos_ ); // Destroy temporaries of right expression.

		// Evaluate left part.
		const Variable l_var= BuildExpressionCodeEnsureVariable( assignment_operator.l_value_, names, function_context );

		if( l_var.type == invalid_type_ || r_var.type == invalid_type_ )
			return BlockBuildInfo();

		if( l_var.value_type != ValueType::Reference )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), assignment_operator.file_pos_ );
			return BlockBuildInfo();
		}
		if( l_var.type != r_var.type )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), assignment_operator.file_pos_, l_var.type, r_var.type );
			return BlockBuildInfo();
		}

		// Check references of destination.
		if( l_var.node != nullptr && function_context.variables_state.HaveOutgoingLinks( l_var.node ) )
			REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), assignment_operator.file_pos_, l_var.node->name );

		if( l_var.type.GetFundamentalType() != nullptr || l_var.type.GetEnumType() != nullptr || l_var.type.GetFunctionPointerType() != nullptr )
		{
			if( l_var.location != Variable::Location::Pointer )
			{
				U_ASSERT(false);
				return BlockBuildInfo();
			}
			U_ASSERT( r_var.location == Variable::Location::LLVMRegister );
			function_context.llvm_ir_builder.CreateStore( r_var.llvm_value, l_var.llvm_value );
		}
		else
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), assignment_operator.file_pos_, l_var.type );
			return BlockBuildInfo();
		}
	}
	// Destruct temporary variables of right and left expressions.
	CallDestructors( temp_variables_storage, names, function_context, assignment_operator.file_pos_ );
	
	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::AdditiveAssignmentOperator& additive_assignment_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expressions.
	const StackVariablesStorage temp_variables_storage( function_context );

	if( // TODO - create temp variables frame here.
		TryCallOverloadedBinaryOperator(
			GetOverloadedOperatorForAdditiveAssignmentOperator( additive_assignment_operator.additive_operation_ ),
			additive_assignment_operator,
			additive_assignment_operator.l_value_,
			additive_assignment_operator.r_value_,
			true, // evaluate args in reverse order
			additive_assignment_operator.file_pos_,
			names,
			function_context ) == std::nullopt )
	{ // Here process default additive assignment operators for fundamental types.
		Variable r_var=
			BuildExpressionCodeEnsureVariable(
				additive_assignment_operator.r_value_,
				names,
				function_context );

		if( r_var.type.GetFundamentalType() != nullptr )
		{
			// We must read value, because referenced by reference value may be changed in l_var evaluation.
			if( r_var.location != Variable::Location::LLVMRegister )
			{
				r_var.llvm_value= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
				r_var.location= Variable::Location::LLVMRegister;
			}
			r_var.value_type= ValueType::Value;
		}
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), additive_assignment_operator.file_pos_ ); // Destroy temporaries of right expression.

		const Variable l_var=
			BuildExpressionCodeEnsureVariable(
				additive_assignment_operator.l_value_,
				names,
				function_context );

		if( l_var.type == invalid_type_ || r_var.type == invalid_type_ )
			return BlockBuildInfo();

		// Check references of destination.
		if( l_var.node != nullptr && function_context.variables_state.HaveOutgoingLinks( l_var.node ) )
			REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), additive_assignment_operator.file_pos_, l_var.node->name );

		const FundamentalType* const l_var_fundamental_type= l_var.type.GetFundamentalType();
		const FundamentalType* const r_var_fundamental_type= r_var.type.GetFundamentalType();
		if( l_var_fundamental_type != nullptr && r_var_fundamental_type != nullptr )
		{
			// Generate binary operator and assignment for fundamental types.
			const Value operation_result_value=
				BuildBinaryOperator(
					l_var, r_var,
					additive_assignment_operator.additive_operation_,
					additive_assignment_operator.file_pos_,
					names,
					function_context );
			if( operation_result_value.GetVariable() == nullptr ) // Not variable in case of error or if template-dependent stuff.
				return BlockBuildInfo();
			const Variable& operation_result= *operation_result_value.GetVariable();

			if( l_var.value_type != ValueType::Reference )
			{
				REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), additive_assignment_operator.file_pos_ );
				return BlockBuildInfo();
			}

			if( operation_result.type != l_var.type )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), additive_assignment_operator.file_pos_, l_var.type, operation_result.type );
				return BlockBuildInfo();
			}

			U_ASSERT( l_var.location == Variable::Location::Pointer );
			llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( operation_result, function_context );
			function_context.llvm_ir_builder.CreateStore( value_in_register, l_var.llvm_value );
		}
		else
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), additive_assignment_operator.file_pos_, l_var.type );
			return BlockBuildInfo();
		}
	}
	// Destruct temporary variables of right and left expressions.
	CallDestructors( temp_variables_storage, names, function_context, additive_assignment_operator.file_pos_ );
	
	return  BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::IncrementOperator& increment_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	BuildDeltaOneOperatorCode(
		increment_operator.expression,
		increment_operator.file_pos_,
		true,
		names,
		function_context );

	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::DecrementOperator& decrement_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	BuildDeltaOneOperatorCode(
		decrement_operator.expression,
		decrement_operator.file_pos_,
		false,
		names,
		function_context );

	return  BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::StaticAssert& static_assert_,
	NamesScope& names,
	FunctionContext& function_context )
{
	BlockBuildInfo block_info;
	
	// Destruction frame for temporary variables of static assert expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	const Variable variable= BuildExpressionCodeEnsureVariable( static_assert_.expression, names, function_context );

	// Destruct temporary variables of right and left expressions.
	// In non-error case, this call produces no code.
	CallDestructors( temp_variables_storage, names, function_context, static_assert_.file_pos_ );

	if( variable.type != bool_type_ )
	{
		REPORT_ERROR( StaticAssertExpressionMustHaveBoolType, names.GetErrors(), static_assert_.file_pos_ );
		return block_info;
	}
	if( variable.constexpr_value == nullptr )
	{
		REPORT_ERROR( StaticAssertExpressionIsNotConstant, names.GetErrors(), static_assert_.file_pos_ );
		return block_info;
	}
	if( llvm::dyn_cast<llvm::UndefValue>(variable.constexpr_value) != nullptr )
	{
		// Undef value means, that value is constexpr, but we are in template prepass, and exact value is unknown. Skip this static_assert
		return block_info;
	}

	if( !variable.constexpr_value->isOneValue() )
	{
		REPORT_ERROR( StaticAssertionFailed, names.GetErrors(), static_assert_.file_pos_ );
	}
	
	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::Halt&,
	NamesScope&,
	FunctionContext& function_context )
{
	function_context.llvm_ir_builder.CreateCall( halt_func_ );

	// We needs terminal , because call to "halt" is not terminal instruction.
	function_context.llvm_ir_builder.CreateUnreachable();

	BlockBuildInfo block_info;
	block_info.have_terminal_instruction_inside= true;
	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	const Synt::HaltIf& halt_if,
	NamesScope& names,
	FunctionContext& function_context )
{
	BlockBuildInfo block_info;
	
	llvm::BasicBlock* const true_block = llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const false_block= llvm::BasicBlock::Create( llvm_context_ );

	const StackVariablesStorage temp_variables_storage( function_context );
	const Variable condition_expression= BuildExpressionCodeEnsureVariable( halt_if.condition, names, function_context );
	const FilePos condition_expression_file_pos= Synt::GetExpressionFilePos( halt_if.condition );
	if( condition_expression.type!= bool_type_ )
	{
		REPORT_ERROR( TypesMismatch,
			names.GetErrors(),
			condition_expression_file_pos,
			bool_type_,
			condition_expression.type );
		return block_info;
	}

	llvm::Value* const condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
	CallDestructors( temp_variables_storage, names, function_context, condition_expression_file_pos );

	function_context.llvm_ir_builder.CreateCondBr( condition_in_register, true_block, false_block );

	// True branch
	function_context.function->getBasicBlockList().push_back( true_block );
	function_context.llvm_ir_builder.SetInsertPoint( true_block );

	function_context.llvm_ir_builder.CreateCall( halt_func_ );
	function_context.llvm_ir_builder.CreateUnreachable();

	// False branch
	function_context.function->getBasicBlockList().push_back( false_block );
	function_context.llvm_ir_builder.SetInsertPoint( false_block );
	
	return block_info;
}

void CodeBuilder::BuildDeltaOneOperatorCode(
	const Synt::Expression& expression,
	const FilePos& file_pos,
	bool positive, // true - increment, false - decrement
	NamesScope& block_names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expressions.
	const StackVariablesStorage temp_variables_storage( function_context );

	const Value value= BuildExpressionCode( expression, block_names, function_context );
	const Variable* const variable= value.GetVariable();
	if( variable == nullptr )
	{
		REPORT_ERROR( ExpectedVariable, block_names.GetErrors(), file_pos, value.GetKindName() );
		return;
	}

	ArgsVector<Function::Arg> args;
	args.emplace_back();
	args.back().type= variable->type;
	args.back().is_mutable= variable->value_type == ValueType::Reference;
	args.back().is_reference= variable->value_type != ValueType::Value;
	const FunctionVariable* const overloaded_operator=
		GetOverloadedOperator( args, positive ? OverloadedOperator::Increment : OverloadedOperator::Decrement, block_names.GetErrors(), file_pos );
	if( overloaded_operator != nullptr )
	{
		if( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::NonConstexpr )
			function_context.have_non_constexpr_operations_inside= true;

		if( overloaded_operator->is_this_call )
		{
			const auto fetch_result= TryFetchVirtualFunction( *variable, *overloaded_operator, function_context, block_names.GetErrors(), file_pos );
			DoCallFunction( fetch_result.second, *overloaded_operator->type.GetFunctionType(), file_pos, { fetch_result.first }, {}, false, block_names, function_context );
		}
		else
			DoCallFunction( overloaded_operator->llvm_function, *overloaded_operator->type.GetFunctionType(), file_pos, { *variable }, {}, false, block_names, function_context );
	}
	else if( const FundamentalType* const fundamental_type= variable->type.GetFundamentalType() )
	{
		if( !IsInteger( fundamental_type->fundamental_type ) )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, block_names.GetErrors(), file_pos, variable->type );
			return;
		}
		if( variable->value_type != ValueType::Reference )
		{
			REPORT_ERROR( ExpectedReferenceValue, block_names.GetErrors(), file_pos );
			return;
		}

		if( variable->node != nullptr && function_context.variables_state.HaveOutgoingLinks( variable->node ) )
			REPORT_ERROR( ReferenceProtectionError, block_names.GetErrors(), file_pos, variable->node->name );

		llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( *variable, function_context );
		llvm::Value* const one=
			llvm::Constant::getIntegerValue(
				fundamental_type->llvm_type,
				llvm::APInt( fundamental_type->llvm_type->getIntegerBitWidth(), uint64_t(1u) ) );

		llvm::Value* const new_value=
			positive
				? function_context.llvm_ir_builder.CreateAdd( value_in_register, one )
				: function_context.llvm_ir_builder.CreateSub( value_in_register, one );

		U_ASSERT( variable->location == Variable::Location::Pointer );
		function_context.llvm_ir_builder.CreateStore( new_value, variable->llvm_value );
	}
	else
	{
		REPORT_ERROR( OperationNotSupportedForThisType, block_names.GetErrors(), file_pos, variable->type );
		return;
	}

	CallDestructors( temp_variables_storage, block_names, function_context, file_pos );
}


void CodeBuilder::BuildStaticAssert( StaticAssert& static_assert_, NamesScope& names, FunctionContext& function_context )
{
	if( static_assert_.syntax_element == nullptr )
		return;

	BuildBlockElement( *static_assert_.syntax_element, names, function_context );
	static_assert_.syntax_element= nullptr;
}

Value* CodeBuilder::ResolveValue( const FilePos& file_pos, NamesScope& names_scope, const Synt::ComplexName& complex_name, const ResolveMode resolve_mode )
{
	return ResolveValue( file_pos, names_scope, complex_name.components.data(), complex_name.components.size(), resolve_mode );
}

Value* CodeBuilder::ResolveValue(
	const FilePos& file_pos,
	NamesScope& names_scope,
	const Synt::ComplexName::Component* components,
	size_t component_count,
	const ResolveMode resolve_mode )
{
	U_ASSERT( component_count > 0u );
	const std::string& last_component_name= components[component_count-1u].name;

	NamesScope* last_space= &names_scope;
	if( components[0].name.empty() )
	{
		U_ASSERT( component_count >= 2u );
		last_space= names_scope.GetRoot();
		++components;
		--component_count;
	}
	else
	{
		const std::string& start= components[0].name;
		NamesScope* space= &names_scope;
		while(true)
		{
			if( space->GetThisScopeValue( start ) != nullptr )
				break;
			space= space->GetParent();
			if( space == nullptr )
				return nullptr;
		}
		last_space= space;
	}

	Value* value= nullptr;
	while( true )
	{
		value= last_space->GetThisScopeValue( components[0].name );
		if( value == nullptr )
			return nullptr;

		if( components[0].have_template_parameters && value->GetTypeTemplatesSet() == nullptr && value->GetFunctionsSet() == nullptr )
		{
			REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), file_pos );
			return nullptr;
		}

		NamesScope* next_space= nullptr;
		ClassProxyPtr next_space_class= nullptr;

		// In case of typedef convert it to type before other checks.
		if( value->GetTypedef() != nullptr )
			GlobalThingBuildTypedef( *last_space, *value );

		if( const NamesScopePtr inner_namespace= value->GetNamespace() )
			next_space= inner_namespace.get();
		else if( const Type* const type= value->GetTypeName() )
		{
			if( Class* const class_= type->GetClassType() )
			{
				if( component_count >= 2u )
				{
					if( class_->syntax_element != nullptr && class_->syntax_element->is_forward_declaration_ )
					{
						REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), file_pos, type );
						return nullptr;
					}
					if( resolve_mode != ResolveMode::ForDeclaration )
						GlobalThingBuildClass( type->GetClassTypeProxy(), TypeCompleteness::Complete );
				}
				next_space= &class_->members;
				next_space_class= type->GetClassTypeProxy();
			}
			else if( EnumPtr const enum_= type->GetEnumType() )
			{
				GlobalThingBuildEnum( enum_, TypeCompleteness::Complete );
				next_space= &enum_->members;
			}
		}
		else if( TypeTemplatesSet* const type_templates_set = value->GetTypeTemplatesSet() )
		{
			GlobalThingBuildTypeTemplatesSet( *last_space, *type_templates_set );
			if( components[0].have_template_parameters && !( resolve_mode == ResolveMode::ForTemplateSignatureParameter && component_count == 1u ) )
			{
				Value* const generated_type=
					GenTemplateType(
						file_pos,
						*type_templates_set,
						components[0].template_parameters,
						names_scope );
				if( generated_type == nullptr )
					return nullptr;

				const Type* const type= generated_type->GetTypeName();
				U_ASSERT( type != nullptr );
				if( Class* const class_= type->GetClassType() )
				{
					next_space= &class_->members;
					next_space_class= type->GetClassTypeProxy();
				}
				value= generated_type;
			}
			else if( component_count >= 2u )
			{
				REPORT_ERROR( TemplateInstantiationRequired, names_scope.GetErrors(), file_pos, type_templates_set->type_templates.front()->syntax_element->name_ );
				return nullptr;
			}
		}
		else if( OverloadedFunctionsSet* const functions_set= value->GetFunctionsSet() )
		{
			if( resolve_mode != ResolveMode::ForDeclaration )
				GlobalThingBuildFunctionsSet( *last_space, *functions_set, false );
			if( components[0].have_template_parameters )
			{
				if( functions_set->template_functions.empty() )
				{
					REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), file_pos );
					return nullptr;
				}

				value=
					GenTemplateFunctionsUsingTemplateParameters(
						file_pos,
						functions_set->template_functions,
						components[0].template_parameters,
						names_scope );
			}
		}

		if( component_count == 1u )
			break;
		else if( next_space != nullptr )
		{
			value= next_space->GetThisScopeValue( components[1].name );

			if( next_space_class != nullptr && resolve_mode != ResolveMode::ForDeclaration &&
				names_scope.GetAccessFor( next_space_class ) < next_space_class->class_->GetMemberVisibility( components[1].name ) )
				REPORT_ERROR( AccessingNonpublicClassMember, names_scope.GetErrors(), file_pos, components[1].name, next_space_class->class_->members.GetThisNamespaceName() );
		}
		else
			return nullptr;

		++components;
		--component_count;
		last_space= next_space;
	}

	if( value != nullptr && value->GetYetNotDeducedTemplateArg() != nullptr )
		REPORT_ERROR( TemplateArgumentIsNotDeducedYet, names_scope.GetErrors(), file_pos, value == nullptr ? "" : last_component_name );

	// Complete some things in resolve.
	if( value != nullptr && resolve_mode != ResolveMode::ForDeclaration )
	{
		if( OverloadedFunctionsSet* const functions_set= value->GetFunctionsSet() )
			GlobalThingBuildFunctionsSet( *last_space, *functions_set, false );
		else if( TypeTemplatesSet* const type_templates_set= value->GetTypeTemplatesSet() )
			GlobalThingBuildTypeTemplatesSet( *last_space, *type_templates_set );
		else if( value->GetTypedef() != nullptr )
			GlobalThingBuildTypedef( *last_space, *value );
		else if( value->GetIncompleteGlobalVariable() != nullptr )
			GlobalThingBuildVariable( *last_space, *value );
	}
	return value;
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
	return llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
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
			llvm::GlobalValue::InternalLinkage, // We have no external variables, so, use internal linkage.
			initializer,
			mangled_name );

	val->setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );

	return val;
}

void CodeBuilder::SetupGeneratedFunctionLinkageAttributes( llvm::Function& function )
{
	// Merge functions with identical code.
	// We doesn`t need different addresses for different functions.
	function.setUnnamedAddr( llvm::GlobalValue::UnnamedAddr::Global );

	// Set comdat for correct linkage of same functions, emitted in several modules.
	llvm::Comdat* const comdat= module_->getOrInsertComdat( function.getName() );
	comdat->setSelectionKind( llvm::Comdat::Any ); // Actually, we needs something, like ExactMatch, but it works not in all cases.
	function.setComdat( comdat );
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

} // namespace CodeBuilderLLVMPrivate

} // namespace U
