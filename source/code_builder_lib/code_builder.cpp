#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include "pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "../lex_synt_lib/lang_types.hpp"
#include "mangling.hpp"

#include "code_builder.hpp"

namespace U
{

namespace
{

typedef std::map< ProgramString, U_FundamentalType > TypesMap;

const TypesMap g_types_map=
{
	{ Keyword( Keywords::void_ ), U_FundamentalType::Void },
	{ Keyword( Keywords::bool_ ), U_FundamentalType::Bool },
	{ Keyword( Keywords::i8_  ), U_FundamentalType::i8  },
	{ Keyword( Keywords::u8_  ), U_FundamentalType::u8  },
	{ Keyword( Keywords::i16_ ), U_FundamentalType::i16 },
	{ Keyword( Keywords::u16_ ), U_FundamentalType::u16 },
	{ Keyword( Keywords::i32_ ), U_FundamentalType::i32 },
	{ Keyword( Keywords::u32_ ), U_FundamentalType::u32 },
	{ Keyword( Keywords::i64_ ), U_FundamentalType::i64 },
	{ Keyword( Keywords::u64_ ), U_FundamentalType::u64 },
	{ Keyword( Keywords::f32_ ), U_FundamentalType::f32 },
	{ Keyword( Keywords::f64_ ), U_FundamentalType::f64 },
	{ Keyword( Keywords::char8_  ), U_FundamentalType::char8  },
	{ Keyword( Keywords::char16_ ), U_FundamentalType::char16 },
	{ Keyword( Keywords::char32_ ), U_FundamentalType::char32 },
};

} // namespace

namespace CodeBuilderPrivate
{

CodeBuilder::FunctionContext::FunctionContext(
	const Type in_return_type,
	const bool in_return_value_is_mutable,
	const bool in_return_value_is_reference,
	llvm::LLVMContext& llvm_context,
	llvm::Function* in_function )
	: return_type(in_return_type)
	, return_value_is_mutable(in_return_value_is_mutable)
	, return_value_is_reference(in_return_value_is_reference)
	, function(in_function)
	, alloca_basic_block( llvm::BasicBlock::Create( llvm_context, "allocations", function ) )
	, alloca_ir_builder( alloca_basic_block )
	, function_basic_block( llvm::BasicBlock::Create( llvm_context, "func_code", function ) )
	, llvm_ir_builder( function_basic_block )
{
}

CodeBuilder::StackVariablesStorage::StackVariablesStorage( FunctionContext& in_function_context )
	: function_context(in_function_context)
{
	function_context.stack_variables_stack.push_back(this);
}

CodeBuilder::StackVariablesStorage::~StackVariablesStorage()
{
	for( const StoredVariablePtr& var : variables )
		function_context.variables_state.RemoveVariable(var);
	function_context.stack_variables_stack.pop_back();
}

void CodeBuilder::StackVariablesStorage::RegisterVariable( const StoredVariablePtr& variable )
{
	function_context.variables_state.AddVariable( variable );
	variables.push_back( variable );
}

CodeBuilder::CodeBuilder(
	std::string target_triple_str,
	const llvm::DataLayout& data_layout )
	: llvm_context_( llvm::getGlobalContext() )
	, target_triple_str_(std::move(target_triple_str))
	, data_layout_(data_layout)
	, constexpr_function_evaluator_( data_layout_ )
{
	fundamental_llvm_types_. i8= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_. u8= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.i16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.u16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.i32= llvm::Type::getInt32Ty( llvm_context_ );
	fundamental_llvm_types_.u32= llvm::Type::getInt32Ty( llvm_context_ );
	fundamental_llvm_types_.i64= llvm::Type::getInt64Ty( llvm_context_ );
	fundamental_llvm_types_.u64= llvm::Type::getInt64Ty( llvm_context_ );

	fundamental_llvm_types_.f32= llvm::Type::getFloatTy( llvm_context_ );
	fundamental_llvm_types_.f64= llvm::Type::getDoubleTy( llvm_context_ );

	fundamental_llvm_types_.char8 = llvm::Type::getInt8Ty ( llvm_context_ );
	fundamental_llvm_types_.char16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.char32= llvm::Type::getInt32Ty( llvm_context_ );

	fundamental_llvm_types_.invalid_type_= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.void_= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.void_for_ret_= llvm::Type::getVoidTy( llvm_context_ );
	fundamental_llvm_types_.bool_= llvm::Type::getInt1Ty( llvm_context_ );

	fundamental_llvm_types_.int_ptr= data_layout_.getIntPtrType(llvm_context_);

	invalid_type_= FundamentalType( U_FundamentalType::InvalidType, fundamental_llvm_types_.invalid_type_ );
	void_type_= FundamentalType( U_FundamentalType::Void, fundamental_llvm_types_.void_ );
	void_type_for_ret_= FundamentalType( U_FundamentalType::Void, fundamental_llvm_types_.void_for_ret_ );
	bool_type_= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );
	size_type_=
		fundamental_llvm_types_.int_ptr->getIntegerBitWidth() == 32u
		? FundamentalType( U_FundamentalType::u32, fundamental_llvm_types_.u32 )
		: FundamentalType( U_FundamentalType::u64, fundamental_llvm_types_.u64 );
}

CodeBuilder::~CodeBuilder()
{
}

ICodeBuilder::BuildResult CodeBuilder::BuildProgram( const SourceGraph& source_graph )
{
	errors_.clear();
	error_count_= 0u;

	module_.reset(
		new llvm::Module(
			ToUTF8( source_graph.nodes_storage[ source_graph.root_node_index ].file_path ),
			llvm_context_ ) );

	// Setup data layout
	module_->setDataLayout(data_layout_);
	module_->setTargetTriple(target_triple_str_);

	// Prepare halt func.
	{
		llvm::FunctionType* void_function_type= llvm::FunctionType::get( fundamental_llvm_types_.void_for_ret_, false );
		halt_func_= llvm::Function::Create( void_function_type, llvm::Function::ExternalLinkage, "__U_halt", module_.get() );
		halt_func_->setDoesNotReturn();
		halt_func_->setDoesNotThrow();
		halt_func_->setUnnamedAddr( true );
	}

	// In some places outside functions we need to execute expression evaluation.
	// Create for this dummy function context.
	llvm::Function* const dummy_function=
		llvm::Function::Create(
			llvm::FunctionType::get( fundamental_llvm_types_.void_, false ),
			llvm::Function::LinkageTypes::ExternalLinkage,
			"",
			module_.get() );

	FunctionContext dummy_function_context(
		void_type_for_ret_,
		false, false,
		llvm_context_,
		dummy_function );
	const StackVariablesStorage dummy_function_variables_storage( dummy_function_context );
	dummy_function_context_= &dummy_function_context;

	// Build graph.
	BuildResultInternal build_result_internal=
		BuildProgramInternal( source_graph, source_graph.root_node_index );

	dummy_function->eraseFromParent(); // Kill dummy function.

	// Fix incomplete typeinfo.
	for( const auto& typeinfo_entry : typeinfo_cache_ )
	{
		if( !typeinfo_entry.second.type.GetLLVMType()->isSized() )
			typeinfo_entry.second.type.GetClassType()->llvm_type->setBody( llvm::ArrayRef<llvm::Type*>() );
	}

	// Clear internal structures.
	compiled_sources_cache_.clear();
	current_class_table_= nullptr;
	enums_table_.clear();
	template_classes_cache_.clear();
	typeinfo_cache_.clear();
	typeinfo_list_end_node_.reset();
	typeinfo_is_end_variable_[0]= typeinfo_is_end_variable_[1]= nullptr;
	typeinfo_class_table_.clear();

	// Soprt by file/line and left only unique error messages.
	// TODO - provide template arguments for error messages inside templates.
	std::sort( errors_.begin(), errors_.end() );
	errors_.erase( std::unique( errors_.begin(), errors_.end() ), errors_.end() );

	BuildResult build_result;
	build_result.errors= errors_;
	build_result.module= std::move( module_ );

	return build_result;
}

CodeBuilder::BuildResultInternal CodeBuilder::BuildProgramInternal(
	const SourceGraph& source_graph,
	const size_t node_index )
{
	BuildResultInternal result;

	result.names_map.reset( new NamesScope( ""_SpC, nullptr ) );
	result.class_table.reset( new ClassTable );
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
	NamesScopeFill( *result.names_map, source_graph_node.ast.program_elements );
	NamesScopeFillOutOfLineElements( *result.names_map, source_graph_node.ast.program_elements );
	GlobalThingBuildNamespace( *result.names_map );

	return result;
}

void CodeBuilder::MergeNameScopes( NamesScope& dst, const NamesScope& src, ClassTable& dst_class_table )
{
	src.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& src_member )
		{
			NamesScope::InsertedName* const dst_member= dst.GetThisScopeName( src_member.first );
			if( dst_member == nullptr )
			{
				// All ok - name form "src" does not exists in "dst".
				if( const NamesScopePtr names_scope= src_member.second.GetNamespace() )
				{
					// We copy namespaces, instead of taking same shared pointer,
					// because using same shared pointer we can change state of "src".
					const NamesScopePtr names_scope_copy= std::make_shared<NamesScope>( names_scope->GetThisNamespaceName(), &dst );
					MergeNameScopes( *names_scope_copy, *names_scope, dst_class_table );
					dst.AddName( src_member.first, Value( names_scope_copy, src_member.second.GetFilePos() ) );

					names_scope_copy->CopyAccessRightsFrom( *names_scope );
				}
				else
				{
					bool class_copied= false;
					if( const Type* const type= src_member.second.GetTypeName() )
					{
						if( const ClassProxyPtr class_proxy= type->GetClassTypeProxy() )
						{
							// If current namespace is parent for this class and name is primary.
							if( class_proxy->class_->members.GetParent() == &src &&
								class_proxy->class_->members.GetThisNamespaceName() == src_member.first )
							{
								CopyClass( src_member.second.GetFilePos(), class_proxy, dst_class_table, dst );
								class_copied= true;
							}
						}
					}

					if( !class_copied )
						dst.AddName( src_member.first, src_member.second );
				}
				return;
			}

			if( dst_member->second.GetKindIndex() != src_member.second.GetKindIndex() )
			{
				// Different kind of symbols - 100% error.
				errors_.push_back( ReportRedefinition( src_member.second.GetFilePos(), src_member.first ) );
				return;
			}

			if( const NamesScopePtr sub_namespace= src_member.second.GetNamespace() )
			{
				// Merge namespaces.
				// TODO - detect here template instantiation namespaces.
				const NamesScopePtr dst_sub_namespace= dst_member->second.GetNamespace();
				U_ASSERT( dst_sub_namespace != nullptr );
				MergeNameScopes( *dst_sub_namespace, *sub_namespace, dst_class_table );
				return;
			}
			else if(
				OverloadedFunctionsSet* const dst_funcs_set=
				dst_member->second.GetFunctionsSet() )
			{
				const OverloadedFunctionsSet* const src_funcs_set= src_member.second.GetFunctionsSet();
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
							errors_.push_back( ReportFunctionPrototypeDuplication( src_func.prototype_file_pos, src_member.first ) );
							continue;
						}

						if( !same_dst_func->have_body &&  src_func.have_body )
							*same_dst_func= src_func; // Take this function - it have body.
						if(  same_dst_func->have_body && !src_func.have_body )
						{} // Ok, prototype imported later.
						if(  same_dst_func->have_body &&  src_func.have_body &&
							same_dst_func->body_file_pos != src_func.body_file_pos )
							errors_.push_back( ReportFunctionBodyDuplication( src_func.body_file_pos, src_member.first ) );
					}
					else
						ApplyOverloadedFunction( *dst_funcs_set, src_func, src_func.prototype_file_pos );
				}
				return;
			}
			else if( const Type* const type= dst_member->second.GetTypeName() )
			{
				if( const ClassProxyPtr dst_class_proxy= type->GetClassTypeProxy() )
				{
					const ClassProxyPtr src_class_proxy= src_member.second.GetTypeName()->GetClassTypeProxy();

					if( src_class_proxy == nullptr || dst_class_proxy != src_class_proxy )
					{
						// Differnet proxy means 100% different classes.
						errors_.push_back( ReportRedefinition( src_member.second.GetFilePos(), src_member.first ) );
						return;
					}

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
						errors_.push_back( ReportClassBodyDuplication( src_class.body_file_pos ) );
					}
					if(  dst_class->completeness == TypeCompleteness::Incomplete && src_class.completeness != TypeCompleteness::Incomplete )
					{
						// Take body of more complete class and store in destintation class table.
						CopyClass( src_class.forward_declaration_file_pos, src_class_proxy, dst_class_table, dst );
					}

					return;
				}
			}

			if( dst_member->second.GetFilePos() == src_member.second.GetFilePos() )
				return; // All ok - things from one source.

			// Can not merge other kinds of values.
			errors_.push_back( ReportRedefinition( src_member.second.GetFilePos(), src_member.first ) );
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
	std::unique_ptr<Class> copy( new Class( src.members.GetThisNamespaceName(), &dst_namespace ) );

	// Make deep copy of inner namespace.
	MergeNameScopes( copy->members, src.members, dst_class_table );

	// Copy fields.
	copy->members_visibility= src.members_visibility;

	copy->syntax_element= src.syntax_element;
	copy->field_count= src.field_count;
	copy->references_tags_count= src.references_tags_count;
	copy->completeness= src.completeness;

	copy->is_typeinfo= src.is_typeinfo;
	copy->have_explicit_noncopy_constructors= src.have_explicit_noncopy_constructors;
	copy->is_default_constructible= src.is_default_constructible;
	copy->is_copy_constructible= src.is_copy_constructible;
	copy->have_destructor= src.have_destructor;
	copy->is_copy_assignable= src.is_copy_assignable;
	copy->can_be_constexpr= src.can_be_constexpr;

	copy->forward_declaration_file_pos= src.forward_declaration_file_pos;
	copy->body_file_pos= src.body_file_pos;

	copy->llvm_type= src.llvm_type;
	copy->base_template= src.base_template;

	copy->kind= src.kind;
	copy->base_class= src.base_class;
	copy->base_class_field_number= src.base_class_field_number;
	copy->parents= src.parents;
	copy->parents_fields_numbers= src.parents_fields_numbers;

	copy->virtual_table= src.virtual_table;
	copy->virtual_table_llvm_type= src.virtual_table_llvm_type;
	copy->this_class_virtual_table= src.this_class_virtual_table;
	copy->virtual_table_field_number= src.virtual_table_field_number;
	copy->ancestors_virtual_tables= src.ancestors_virtual_tables;

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
	fundamental_globals_file_pos.file_index= static_cast<unsigned short>(~0u);
	fundamental_globals_file_pos.line= static_cast<unsigned short>(~0u);
	fundamental_globals_file_pos.pos_in_line= static_cast<unsigned short>(~0u);

	for( const auto& fundamental_type_value : g_types_map )
	{
		global_names_scope.AddName(
			fundamental_type_value.first,
			Value(
				FundamentalType( fundamental_type_value.second, GetFundamentalLLVMType( fundamental_type_value.second ) ),
				fundamental_globals_file_pos ) );
	}

	global_names_scope.AddName( Keyword( Keywords::size_type_ ), Value( size_type_, fundamental_globals_file_pos ) );
}

Type CodeBuilder::PrepareType(
	const Synt::ITypeNamePtr& type_name,
	NamesScope& names_scope )
{
	U_ASSERT( type_name != nullptr );

	Type result= invalid_type_;

	if( const auto array_type_name= dynamic_cast<const Synt::ArrayTypeName*>(type_name.get()) )
	{
		result= Array();
		Array& array_type= *result.GetArrayType();

		array_type.type= PrepareType( array_type_name->element_type, names_scope );

		const Synt::IExpressionComponent& num= *array_type_name->size;

		const Variable size_variable= BuildExpressionCodeEnsureVariable( num, names_scope, *dummy_function_context_ );
		if( size_variable.constexpr_value != nullptr )
		{
			if( const FundamentalType* const size_fundamental_type= size_variable.type.GetFundamentalType() )
			{
				if( IsInteger( size_fundamental_type->fundamental_type ) )
				{
					if( llvm::dyn_cast<llvm::UndefValue>(size_variable.constexpr_value) != nullptr )
						array_type.size= Array::c_undefined_size;
					else
					{
						const llvm::APInt& size_value= size_variable.constexpr_value->getUniqueInteger();
						if( IsSignedInteger( size_fundamental_type->fundamental_type ) && size_value.isNegative() )
							errors_.push_back( ReportArraySizeIsNegative( num.GetFilePos() ) );
						else
							array_type.size= SizeType( size_value.getLimitedValue() );
					}
				}
				else
					errors_.push_back( ReportArraySizeIsNotInteger( num.GetFilePos() ) );
			}
			else
				U_ASSERT( false && "Nonfundamental constexpr? WTF?" );
		}
		else
			errors_.push_back( ReportExpectedConstantExpression( num.GetFilePos() ) );

		array_type.llvm_type= llvm::ArrayType::get( array_type.type.GetLLVMType(), array_type.ArraySizeOrZero() );

		// TODO - generate error, if total size of type (incuding arrays) is more, than half of address space of target architecture.
	}
	else if( const auto function_type_name= dynamic_cast<const Synt::FunctionType*>(type_name.get()) )
	{
		FunctionPointer function_pointer_type;
		Function& function_type= function_pointer_type.function;

		if( function_type_name->return_type_ == nullptr )
			function_type.return_type= void_type_for_ret_;
		else
			function_type.return_type= PrepareType( function_type_name->return_type_, names_scope );
		function_type.return_value_is_mutable= function_type_name->return_value_mutability_modifier_ == MutabilityModifier::Mutable;
		function_type.return_value_is_reference= function_type_name->return_value_reference_modifier_ == ReferenceModifier::Reference;

		if( !function_type.return_value_is_reference &&
			!( function_type.return_type.GetFundamentalType() != nullptr ||
			   function_type.return_type.GetClassType() != nullptr ||
			   function_type.return_type.GetEnumType() != nullptr ||
			   function_type.return_type.GetFunctionPointerType() != nullptr ) )
			errors_.push_back( ReportNotImplemented( function_type_name->file_pos_, "return value types except fundamentals, enums, classes, function pointers" ) );

		for( const Synt::FunctionArgumentPtr& arg : function_type_name->arguments_ )
		{
			if( IsKeyword( arg->name_ ) )
				errors_.push_back( ReportUsingKeywordAsName( arg->file_pos_ ) );

			function_type.args.emplace_back();
			Function::Arg& out_arg= function_type.args.back();
			out_arg.type= PrepareType( arg->type_, names_scope );

			out_arg.is_mutable= arg->mutability_modifier_ == MutabilityModifier::Mutable;
			out_arg.is_reference= arg->reference_modifier_ == ReferenceModifier::Reference;

			if( !out_arg.is_reference &&
				!( out_arg.type.GetFundamentalType() != nullptr ||
				   out_arg.type.GetClassType() != nullptr ||
				   out_arg.type.GetEnumType() != nullptr ||
				   out_arg.type.GetFunctionPointerType() != nullptr ) )
				errors_.push_back( ReportNotImplemented( arg->file_pos_, "parameters types except fundamentals, classes, enums, functionpointers" ) );

			ProcessFunctionArgReferencesTags( *function_type_name, function_type, *arg, out_arg, function_type.args.size() - 1u );
		}

		function_type.unsafe= function_type_name->unsafe_;

		TryGenerateFunctionReturnReferencesMapping( *function_type_name, function_type );
		ProcessFunctionTypeReferencesPollution( *function_type_name, function_type );

		function_type.llvm_function_type= GetLLVMFunctionType( function_type );
		function_pointer_type.llvm_function_pointer_type= llvm::PointerType::get( function_type.llvm_function_type, 0u );

		return function_pointer_type;
	}
	else if( const auto named_type_name= dynamic_cast<const Synt::NamedTypeName*>(type_name.get()) )
	{
		if( const NamesScope::InsertedName* name=
			ResolveName( named_type_name->file_pos_, names_scope, named_type_name->name ) )
		{
			if( const Type* const type= name->second.GetTypeName() )
				result= *type;
			else
				errors_.push_back( ReportNameIsNotTypeName( named_type_name->file_pos_, name->first ) );
		}
		else
			errors_.push_back( ReportNameNotFound( named_type_name->file_pos_, named_type_name->name ) );
	}
	else U_ASSERT(false);

	return result;
}

llvm::FunctionType* CodeBuilder::GetLLVMFunctionType( const Function& function_type )
{
	std::vector<llvm::Type*> args_llvm_types;

	bool first_arg_is_sret= false;
	if( !function_type.return_value_is_reference )
	{
		if( function_type.return_type.GetFundamentalType() != nullptr ||
			function_type.return_type.GetEnumType() != nullptr ||
			function_type.return_type.GetFunctionPointerType() != nullptr )
		{}
		else if( const Class* const class_type= function_type.return_type.GetClassType() )
		{
			// Add return-value ponter as "sret" argument for class types.
			args_llvm_types.push_back( llvm::PointerType::get( class_type->llvm_type, 0u ) );
			first_arg_is_sret= true;
		}
		else U_ASSERT( false );
	}

	for( const Function::Arg& arg : function_type.args )
	{
		llvm::Type* type= arg.type.GetLLVMType();
		if( arg.is_reference )
			type= llvm::PointerType::get( type, 0u );
		else
		{
			if( arg.type.GetFundamentalType() != nullptr || arg.type.GetEnumType() != nullptr || arg.type.GetFunctionPointerType() )
			{}
			else if( arg.type.GetClassType() != nullptr )
			{
				// Mark value-parameters of class types as pointer. Lately this parameters will be marked as "byval".
				type= llvm::PointerType::get( type, 0u );
			}
			else U_ASSERT( false );
		}
		args_llvm_types.push_back( type );
	}

	llvm::Type* llvm_function_return_type= nullptr;
	if( first_arg_is_sret )
		llvm_function_return_type= fundamental_llvm_types_.void_for_ret_;
	else
	{
		llvm_function_return_type= function_type.return_type.GetLLVMType();
		if( function_type.return_value_is_reference )
			llvm_function_return_type= llvm::PointerType::get( llvm_function_return_type, 0u );
	}

	return llvm::FunctionType::get( llvm_function_return_type, args_llvm_types, false );
}

void CodeBuilder::TryCallCopyConstructor(
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
		errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, class_type.ToString() ) );
		return;
	}

	// Search for copy-constructor.
	const NamesScope::InsertedName* const constructos_name= class_.members.GetThisScopeName( Keyword( Keywords::constructor_ ) );
	U_ASSERT( constructos_name != nullptr );
	const OverloadedFunctionsSet* const constructors= constructos_name->second.GetFunctionsSet();
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
	llvm::Value* const constructor_args[2u]= { this_, src };
	function_context.llvm_ir_builder.CreateCall(
		constructor->llvm_function,
		llvm::ArrayRef<llvm::Value*>( constructor_args, 2u ) );

	if( constructor->type.GetFunctionType()->unsafe && !function_context.is_in_unsafe_block )
		errors_.push_back( ReportUnsafeFunctionCallOutsideUnsafeBlock( file_pos ) );
}

void CodeBuilder::GenerateLoop(
	const SizeType iteration_count,
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
	llvm::Value* const couter_address= function_context.alloca_ir_builder.CreateAlloca( size_type_llvm );
	couter_address->setName( "loop_counter" );
	function_context.llvm_ir_builder.CreateStore( zero_value, couter_address );

	llvm::BasicBlock* const loop_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const block_after_loop= llvm::BasicBlock::Create( llvm_context_ );

	function_context.llvm_ir_builder.CreateBr( loop_block );
	function_context.function->getBasicBlockList().push_back( loop_block );
	function_context.llvm_ir_builder.SetInsertPoint( loop_block );

	llvm::Value* const current_counter_value= function_context.llvm_ir_builder.CreateLoad( couter_address );
	loop_body( current_counter_value );

	llvm::Value* const counter_value_plus_one= function_context.llvm_ir_builder.CreateAdd( current_counter_value, one_value );
	function_context.llvm_ir_builder.CreateStore( counter_value_plus_one, couter_address );
	llvm::Value* const counter_test= function_context.llvm_ir_builder.CreateICmpULT( counter_value_plus_one, loop_count_value );
	function_context.llvm_ir_builder.CreateCondBr( counter_test, loop_block, block_after_loop );

	function_context.function->getBasicBlockList().push_back( block_after_loop );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_loop );
}

void CodeBuilder::CallDestructorsImpl(
	const StackVariablesStorage& stack_variables_storage,
	FunctionContext& function_context,
	DestroyedVariableReferencesCount& destroyed_variable_references,
	const FilePos& file_pos )
{
	// Call destructors in reverse order.
	for( auto it = stack_variables_storage.variables.rbegin(); it != stack_variables_storage.variables.rend(); ++it )
	{
		StoredVariable& stored_variable= **it;

		if( function_context.variables_state.VariableIsMoved( *it ) )
			continue;

		if( stored_variable.kind == StoredVariable::Kind::Reference )
		{
			// Increment coounter of destroyed reference for referenced variables.
			for( const StoredVariablePtr& referenced_variable : stored_variable.content.referenced_variables )
			{
				if( destroyed_variable_references.find( referenced_variable ) == destroyed_variable_references.end() )
					destroyed_variable_references[ referenced_variable ]= 1u;
				else
					++destroyed_variable_references[ referenced_variable ];
			}
		}
		else if( stored_variable.kind == StoredVariable::Kind::Variable )
		{
			// Increment coounter of destroyed reference for referenced variables of references inside this variable.
			for( const auto& referenced_variable_pair : function_context.variables_state.GetVariableReferences( *it ) )
			{
				if( destroyed_variable_references.find( referenced_variable_pair.first ) == destroyed_variable_references.end() )
					destroyed_variable_references[ referenced_variable_pair.first ]= 1u;
				else
					++destroyed_variable_references[ referenced_variable_pair.first ];
			}

			// Check references.
			U_ASSERT( stored_variable.imut_use_counter.use_count() >= 1u && stored_variable.mut_use_counter.use_count() >= 1u );

			size_t alive_ref_count= ( stored_variable.imut_use_counter.use_count() - 1u ) + ( stored_variable.mut_use_counter.use_count() - 1u );
			const auto map_it= destroyed_variable_references.find( *it );
			if( map_it != destroyed_variable_references.end() )
				alive_ref_count-= map_it->second;

			if( alive_ref_count > 0u )
				errors_.push_back( ReportDestroyedVariableStillHaveReferences( file_pos, stored_variable.name ) );
		}
		else
			U_ASSERT( false );

		// Call destructors.
		const Variable& var= stored_variable.content;
		if( stored_variable.kind == StoredVariable::Kind::Variable && var.type.HaveDestructor() )
			CallDestructor( var.llvm_value, var.type, function_context, file_pos );
	}
}

void CodeBuilder::CallDestructors(
	const StackVariablesStorage& stack_variables_storage,
	FunctionContext& function_context,
	const FilePos& file_pos )
{
	DestroyedVariableReferencesCount destroyed_variable_references;
	CallDestructorsImpl( stack_variables_storage, function_context, destroyed_variable_references, file_pos );
}

void CodeBuilder::CallDestructor(
	llvm::Value* const ptr,
	const Type& type,
	FunctionContext& function_context,
	const FilePos& file_pos )
{
	U_ASSERT( type.HaveDestructor() );

	if( const Class* const class_= type.GetClassType() )
	{
		const NamesScope::InsertedName* const destructor_name= class_->members.GetThisScopeName( Keyword( Keywords::destructor_ ) );
		U_ASSERT( destructor_name != nullptr );
		const OverloadedFunctionsSet* const destructors= destructor_name->second.GetFunctionsSet();
		U_ASSERT(destructors != nullptr && destructors->functions.size() == 1u );

		const FunctionVariable& destructor= destructors->functions.front();
		llvm::Value* const destructor_args[]= { ptr };
		function_context.llvm_ir_builder.CreateCall(
			destructor.llvm_function,
			llvm::ArrayRef<llvm::Value*>( destructor_args, 1u ) );

		if( destructor.type.GetFunctionType()->unsafe && !function_context.is_in_unsafe_block )
			errors_.push_back( ReportUnsafeFunctionCallOutsideUnsafeBlock( file_pos ) );
	}
	else if( const Array* const array_type= type.GetArrayType() )
	{
		// SPRACHE_TODO - maybe call destructors of arrays in reverse order?
		GenerateLoop(
			array_type->ArraySizeOrZero(),
			[&]( llvm::Value* const index )
			{
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= index;
				CallDestructor(
					function_context.llvm_ir_builder.CreateGEP( ptr, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) ),
					array_type->type,
					function_context,
					file_pos );
			},
			function_context );
	}
	else
		U_ASSERT( false && "WTF? strange type for variable" );
}

void CodeBuilder::CallDestructorsForLoopInnerVariables( FunctionContext& function_context, const FilePos& file_pos )
{
	U_ASSERT( !function_context.loops_stack.empty() );

	DestroyedVariableReferencesCount destroyed_variable_references;

	// Destroy all local variables before "break"/"continue" in all blocks inside loop.
	size_t undestructed_stack_size= function_context.stack_variables_stack.size();
	for(
		auto it= function_context.stack_variables_stack.rbegin();
		it != function_context.stack_variables_stack.rend() &&
		undestructed_stack_size > function_context.loops_stack.back().stack_variables_stack_size;
		++it, --undestructed_stack_size )
	{
		CallDestructorsImpl( **it, function_context, destroyed_variable_references, file_pos );
	}
}

void CodeBuilder::CallDestructorsBeforeReturn( FunctionContext& function_context, const FilePos& file_pos )
{
	DestroyedVariableReferencesCount destroyed_variable_references;

	// We must call ALL destructors of local variables, arguments, etc before each return.
	for( auto it= function_context.stack_variables_stack.rbegin(); it != function_context.stack_variables_stack.rend(); ++it )
		CallDestructorsImpl( **it, function_context, destroyed_variable_references, file_pos );
}

void CodeBuilder::CallMembersDestructors( FunctionContext& function_context, const FilePos& file_pos )
{
	U_ASSERT( function_context.this_ != nullptr );
	const Class* const class_= function_context.this_->type.GetClassType();
	U_ASSERT( class_ != nullptr );

	for( size_t i= 0u; i < class_->parents.size(); ++i )
	{
		U_ASSERT( class_->parents[i]->class_->have_destructor ); // Parents are polymorph, polymorph classes always have destructors.
		llvm::Value* index_list[2];
		index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(class_->parents_fields_numbers[i]) ) );
		CallDestructor(
			function_context.llvm_ir_builder.CreateGEP( function_context.this_->llvm_value, index_list ),
			class_->parents[i],
			function_context,
			file_pos );
	}

	class_->members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr || field->is_reference || !field->type.HaveDestructor() ||
				field->class_.lock()->class_ != class_ )
				return;

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			CallDestructor(
				function_context.llvm_ir_builder.CreateGEP( function_context.this_->llvm_value, index_list ),
				field->type,
				function_context,
				file_pos );
		} );
}

void CodeBuilder::PrepareFunction(
	NamesScope& names_scope,
	const ClassProxyPtr& base_class,
	OverloadedFunctionsSet& functions_set,
	const Synt::Function& func,
	const bool is_out_of_line_function )
{
	const ProgramString& func_name= func.name_.components.back().name;
	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	const bool is_special_method= is_constructor || is_destructor;

	if( !is_special_method && IsKeyword( func_name ) )
		errors_.push_back( ReportUsingKeywordAsName( func.file_pos_ ) );

	if( is_special_method && base_class == nullptr )
	{
		errors_.push_back( ReportConstructorOrDestructorOutsideClass( func.file_pos_ ) );
		return;
	}
	if( !is_constructor && func.constructor_initialization_list_ != nullptr )
	{
		errors_.push_back( ReportInitializationListInNonconstructor( func.constructor_initialization_list_->file_pos_ ) );
		return;
	}
	if( is_destructor && !func.type_.arguments_.empty() )
	{
		errors_.push_back( ReportExplicitArgumentsInDestructor( func.file_pos_ ) );
		return;
	}

	if( func.condition_ != nullptr )
	{
		const Variable expression= BuildExpressionCodeEnsureVariable( *func.condition_, names_scope, *dummy_function_context_ );
		if( expression.type == bool_type_ )
		{
			if( expression.constexpr_value != nullptr )
			{
				if( expression.constexpr_value->isZeroValue() )
					return; // Function disabled.
			}
			else
				errors_.push_back( ReportExpectedConstantExpression( func.condition_->GetFilePos() ) );
		}
		else
			errors_.push_back( ReportTypesMismatch( func.condition_->GetFilePos(), bool_type_.ToString(), expression.type.ToString() ) );
	}

	FunctionVariable func_variable;
	func_variable.type= Function();
	{ // Prepare function type
		Function& function_type= *func_variable.type.GetFunctionType();

		if( func.type_.return_type_ == nullptr )
			function_type.return_type= void_type_for_ret_;
		else
		{
			function_type.return_type= PrepareType( func.type_.return_type_, names_scope );
			if( function_type.return_type == invalid_type_ )
				return;
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
			   function_type.return_type.GetEnumType() != nullptr ||
			   function_type.return_type.GetFunctionPointerType() != nullptr ) )
		{
			errors_.push_back( ReportNotImplemented( func.file_pos_, "return value types except fundamentals, enums, classes, function pointers" ) );
			return;
		}

		if( is_special_method && function_type.return_type != void_type_ )
			errors_.push_back( ReportConstructorAndDestructorMustReturnVoid( func.file_pos_ ) );

		ProcessFunctionReturnValueReferenceTags( func.type_, function_type );

		// Args.
		function_type.args.reserve( func.type_.arguments_.size() );

		// Generate "this" arg for constructors.
		if( is_special_method )
		{
			func_variable.is_this_call= true;

			function_type.args.emplace_back();
			Function::Arg& arg= function_type.args.back();
			arg.type= base_class;
			arg.is_reference= true;
			arg.is_mutable= true;
		}

		for( const Synt::FunctionArgumentPtr& arg : func.type_.arguments_ )
		{
			const bool is_this= arg == func.type_.arguments_.front() && arg->name_ == Keywords::this_;

			if( !is_this && IsKeyword( arg->name_ ) )
				errors_.push_back( ReportUsingKeywordAsName( arg->file_pos_ ) );

			if( is_this && is_destructor )
				errors_.push_back( ReportExplicitThisInDestructor( arg->file_pos_ ) );
			if( is_this && is_constructor )
			{
				// Explicit this for constructor.
				U_ASSERT( function_type.args.size() == 1u );
				ProcessFunctionArgReferencesTags( func.type_, function_type, *arg, function_type.args.back(), function_type.args.size() - 1u );
				continue;
			}

			function_type.args.emplace_back();
			Function::Arg& out_arg= function_type.args.back();

			if( is_this )
			{
				func_variable.is_this_call= true;
				if( base_class == nullptr )
				{
					errors_.push_back( ReportThisInNonclassFunction( func.file_pos_, func_name ) );
					return;
				}
				out_arg.type= base_class;
			}
			else
				out_arg.type= PrepareType( arg->type_, names_scope );

			out_arg.is_mutable= arg->mutability_modifier_ == MutabilityModifier::Mutable;
			out_arg.is_reference= is_this || arg->reference_modifier_ == ReferenceModifier::Reference;

			if( !out_arg.is_reference &&
				!( out_arg.type.GetFundamentalType() != nullptr ||
				   out_arg.type.GetClassType() != nullptr ||
				   out_arg.type.GetEnumType() != nullptr ||
				   out_arg.type.GetFunctionPointerType() != nullptr ) )
			{
				errors_.push_back( ReportNotImplemented( func.file_pos_, "parameters types except fundamentals, classes, enums, functionpointers" ) );
				return;
			}

			ProcessFunctionArgReferencesTags( func.type_, function_type, *arg, out_arg, function_type.args.size() - 1u );
		} // for arguments

		function_type.unsafe= func.type_.unsafe_;

		TryGenerateFunctionReturnReferencesMapping( func.type_, function_type );
		ProcessFunctionReferencesPollution( func, function_type, base_class );
		CheckOverloadedOperator( base_class, function_type, func.overloaded_operator_, func.file_pos_ );

	} // end prepare function type

	// Set constexpr.
	if( func.constexpr_ )
	{
		if( func.block_ == nullptr )
			errors_.push_back( ReportConstexprFunctionsMustHaveBody( func.file_pos_ ) );
		if( func.virtual_function_kind_ != Synt::VirtualFunctionKind::None )
			errors_.push_back( ReportConstexprFunctionCanNotBeVirtual( func.file_pos_ ) );

		func_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprIncomplete;
	}

	// Set virtual.
	if( func.virtual_function_kind_ != Synt::VirtualFunctionKind::None )
	{
		if( base_class == nullptr )
			errors_.push_back( ReportVirtualForNonclassFunction( func.file_pos_, func_name ) );
		if( !func_variable.is_this_call )
			errors_.push_back( ReportVirtualForNonThisCallFunction( func.file_pos_, func_name ) );
		if( is_constructor )
			errors_.push_back( ReportFunctionCanNotBeVirtual( func.file_pos_, func_name ) );
		if( base_class != nullptr && ( base_class->class_->kind == Class::Kind::Struct || base_class->class_->kind == Class::Kind::NonPolymorph ) )
			errors_.push_back( ReportVirtualForNonpolymorphClass( func.file_pos_, func_name ) );
		if( is_out_of_line_function )
			errors_.push_back( ReportVirtualForFunctionImplementation( func.file_pos_, func_name ) );

		func_variable.virtual_function_kind= func.virtual_function_kind_;
	}

	// Set no_mangle
	if( func.no_mangle_ )
	{
		// Allow only global no-mangle function. This prevents existing of multiple "nomangle" functions with same name in different namespaces.
		// If function is operator, it can not be global.
		if( names_scope.GetParent() != nullptr )
			errors_.push_back( ReportNoMangleForNonglobalFunction( func.file_pos_, func_name ) );
		func_variable.no_mangle= true;
	}

	// Set conversion constructor.
	func_variable.is_conversion_constructor= func.is_conversion_constructor_;
	U_ASSERT( !( func.is_conversion_constructor_ && !is_constructor ) );
	if( func.is_conversion_constructor_ && func_variable.type.GetFunctionType()->args.size() != 2u )
		errors_.push_back( ReportConversionConstructorMustHaveOneArgument( func.file_pos_ ) );

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
			errors_.push_back( ReportInvalidMethodForBodyGeneration( func.file_pos_ ) );
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
			errors_.push_back( ReportFunctionPrototypeDuplication( func.file_pos_, func_name ) );
		else if( prev_function->syntax_element->block_ != nullptr && func.block_ != nullptr )
			errors_.push_back( ReportFunctionBodyDuplication( func.file_pos_, func_name ) );

		if( prev_function->is_this_call != func_variable.is_this_call )
			errors_.push_back( ReportThiscallMismatch( func.file_pos_, func_name ) );

		if( !is_out_of_line_function )
		{
			if( prev_function->virtual_function_kind != func.virtual_function_kind_ )
				errors_.push_back( ReportVirtualMismatch( func.file_pos_, func_name ) );
		}
		if( prev_function->is_deleted != func_variable.is_deleted )
			errors_.push_back( ReportBodyForDeletedFunction( prev_function->prototype_file_pos, func_name ) );
		if( prev_function->is_generated != func_variable.is_generated )
			errors_.push_back( ReportBodyForGeneratedFunction( prev_function->prototype_file_pos, func_name ) );

		if( !prev_function->no_mangle && func_variable.no_mangle )
			errors_.push_back( ReportNoMangleMismatch( func.file_pos_, func_name ) );

		if( prev_function->is_conversion_constructor != func_variable.is_conversion_constructor )
			errors_.push_back( ReportCouldNotOverloadFunction( func.file_pos_ ) ); // Maybe generate separate error?
	}
	else
	{
		if( is_out_of_line_function )
		{
			errors_.push_back( ReportFunctionDeclarationOutsideItsScope( func.file_pos_ ) );
			return;
		}
		if( functions_set.have_nomangle_function || ( !functions_set.functions.empty() && func_variable.no_mangle ) )
		{
			errors_.push_back( ReportCouldNotOverloadFunction( func.file_pos_ ) );
			return;
		}

		const bool overloading_ok= ApplyOverloadedFunction( functions_set, func_variable, func.file_pos_ );
		if( !overloading_ok )
			return;

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
	}
}

void CodeBuilder::CheckOverloadedOperator(
	const ClassProxyPtr& base_class,
	const Function& func_type,
	const OverloadedOperator overloaded_operator,
	const FilePos& file_pos )
{
	if( overloaded_operator == OverloadedOperator::None )
		return; // Not operator

	if( base_class == nullptr )
	{
		errors_.push_back( ReportOperatorDeclarationOutsideClass( file_pos ) );
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
		errors_.push_back( ReportOperatorDoesNotHaveParentClassArguments( file_pos ) );

	switch( overloaded_operator )
	{
	case OverloadedOperator::Add:
	case OverloadedOperator::Sub:
		if( !( func_type.args.size() == 1u || func_type.args.size() == 2u ) )
			errors_.push_back( ReportInvalidArgumentCountForOperator( file_pos ) );
		break;

	case OverloadedOperator::Mul:
	case OverloadedOperator::Div:
	case OverloadedOperator::Rem:
	case OverloadedOperator::Equal:
	case OverloadedOperator::NotEqual:
	case OverloadedOperator::Less:
	case OverloadedOperator::LessEqual:
	case OverloadedOperator::Greater:
	case OverloadedOperator::GreaterEqual:
	case OverloadedOperator::And:
	case OverloadedOperator::Or :
	case OverloadedOperator::Xor:
	case OverloadedOperator::ShiftLeft :
	case OverloadedOperator::ShiftRight:
		if( func_type.args.size() != 2u )
			errors_.push_back( ReportInvalidArgumentCountForOperator( file_pos ) );
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
			errors_.push_back( ReportInvalidArgumentCountForOperator( file_pos ) );
		if( func_type.return_type != void_type_ )
			errors_.push_back( ReportInvalidReturnTypeForOperator( file_pos, void_type_.ToString() ) );
		break;

	case OverloadedOperator::LogicalNot:
	case OverloadedOperator::BitwiseNot:
		if( func_type.args.size() != 1u )
			errors_.push_back( ReportInvalidArgumentCountForOperator( file_pos ) );
		break;

	case OverloadedOperator::Assign:
		if( func_type.args.size() != 2u )
			errors_.push_back( ReportInvalidArgumentCountForOperator( file_pos ) );
		if( func_type.return_type != void_type_ )
			errors_.push_back( ReportInvalidReturnTypeForOperator( file_pos, void_type_.ToString() ) );
		break;

	case OverloadedOperator::Increment:
	case OverloadedOperator::Decrement:
		if( func_type.args.size() != 1u )
			errors_.push_back( ReportInvalidArgumentCountForOperator( file_pos ) );
		if( func_type.return_type != void_type_ )
			errors_.push_back( ReportInvalidReturnTypeForOperator( file_pos, void_type_.ToString() ) );
		break;

	case OverloadedOperator::Indexing:
		if( func_type.args.size() != 2u )
			errors_.push_back( ReportInvalidArgumentCountForOperator( file_pos ) );
		// Indexing operator must have first argument of parent class.
		if( !func_type.args.empty() && func_type.args[0].type != base_class )
			errors_.push_back( ReportOperatorDoesNotHaveParentClassArguments( file_pos ) );
		break;

	case OverloadedOperator::Call:
		if( func_type.args.empty() )
			errors_.push_back( ReportInvalidArgumentCountForOperator( file_pos ) );
		// Call operator must have first argument of parent class.
		if( !func_type.args.empty() && func_type.args[0].type != base_class )
			errors_.push_back( ReportOperatorDoesNotHaveParentClassArguments( file_pos ) );
		break;

	case OverloadedOperator::None:
		U_ASSERT(false);
	};
}

void CodeBuilder::BuildFuncCode(
	FunctionVariable& func_variable,
	const ClassProxyPtr& base_class,
	NamesScope& parent_names_scope,
	const ProgramString& func_name,
	const Synt::FunctionArgumentsDeclaration& args,
	const Synt::Block* const block,
	const Synt::StructNamedInitializer* const constructor_initialization_list )
{
	Function* const function_type= func_variable.type.GetFunctionType();
	function_type->llvm_function_type= GetLLVMFunctionType( *function_type );

	const bool first_arg_is_sret=
		function_type->llvm_function_type->getReturnType()->isVoidTy() && function_type->return_type != void_type_;

	llvm::Function* llvm_function;
	if( func_variable.llvm_function == nullptr )
	{
		llvm_function=
			llvm::Function::Create(
				function_type->llvm_function_type,
				llvm::Function::LinkageTypes::ExternalLinkage, // External - for prototype.
				func_variable.no_mangle ? ToUTF8( func_name ) : MangleFunction( parent_names_scope, func_name, *function_type ),
				module_.get() );

		// Merge functions with identical code.
		// We doesn`t need different addresses for different functions.
		llvm_function->setUnnamedAddr( true );

		// Mark reference-parameters as nonnull.
		// Mark fake-pointer parameters of struct type as "byvall".
		// TODO - maybe mark immutable reference-parameters as "noalias"?
		for( size_t i= 0u; i < function_type->args.size(); i++ )
		{
			const unsigned int arg_attr_index= static_cast<unsigned int>(i + 1u + (first_arg_is_sret ? 1u : 0u ));
			if (function_type->args[i].is_reference )
				llvm_function->addAttribute( arg_attr_index, llvm::Attribute::NonNull );
			else
			{
				if( function_type->args[i].type.GetClassType() != nullptr )
				{
					llvm_function->addAttribute( arg_attr_index, llvm::Attribute::NonNull );
					llvm_function->addAttribute( arg_attr_index, llvm::Attribute::ByVal );
				}
			}
		}

		if( first_arg_is_sret )
			llvm_function->addAttribute( 1u, llvm::Attribute::StructRet );

		func_variable.llvm_function= llvm_function;
	}
	else
		llvm_function= llvm::dyn_cast<llvm::Function>( func_variable.llvm_function );

	if( block == nullptr )
	{
		// This is only prototype, then, function preparing work is done.
		func_variable.have_body= false;
		return;
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
	for( const Function::Arg& arg : function_type->args )
	{
		if( !arg.is_reference && arg.type != void_type_ &&
			!EnsureTypeCompleteness( arg.type, TypeCompleteness::Complete ) )
			errors_.push_back( ReportUsingIncompleteType( args.front()->file_pos_, arg.type.ToString() ) );
	}
	if( !function_type->return_value_is_reference && function_type->return_type != void_type_ &&
		!EnsureTypeCompleteness( function_type->return_type, TypeCompleteness::Complete ) )
		errors_.push_back( ReportUsingIncompleteType( func_variable.body_file_pos, function_type->return_type.ToString() ) );

	NamesScope function_names( ""_SpC, &parent_names_scope );
	FunctionContext function_context(
		function_type->return_type,
		function_type->return_value_is_mutable,
		function_type->return_value_is_reference,
		llvm_context_,
		llvm_function );
	const StackVariablesStorage args_storage( function_context );

	std::vector< std::pair< StoredVariablePtr, StoredVariablePtr > > args_stored_variables;
	args_stored_variables.resize( function_type->args.size() );

	// push args
	Variable this_;
	unsigned int arg_number= 0u;

	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	const bool have_implicit_this= is_destructor || ( is_constructor && ( args.empty() || args.front()->name_ != Keywords::this_ ) );

	for( llvm::Argument& llvm_arg : llvm_function->args() )
	{
		// Skip "sret".
		if( first_arg_is_sret && &llvm_arg == &*llvm_function->arg_begin() )
		{
			llvm_arg.setName( "_return_value" );
			function_context.s_ret_= &llvm_arg;
			continue;
		}

		const Function::Arg& arg= function_type->args[ arg_number ];

		if( arg_number == 0u && ( have_implicit_this || is_constructor ) )
		{
			this_.location= Variable::Location::Pointer;
			this_.value_type= ValueType::Reference;
			this_.type= arg.type;
			this_.llvm_value= &llvm_arg;
			llvm_arg.setName( KeywordAscii( Keywords::this_ ) );
			function_context.this_= &this_;

			const StoredVariablePtr this_storage= std::make_shared<StoredVariable>( Keyword(Keywords::this_), this_ );
			this_.referenced_variables.emplace(this_storage);

			function_context.variables_state.AddVariable( this_storage );
			args_stored_variables[arg_number].first= this_storage;

			if (arg.type.ReferencesTagsCount() > 0u )
			{
				const StoredVariablePtr inner_variable = std::make_shared<StoredVariable>( Keyword(Keywords::this_) + " inner reference"_SpC, Variable(), StoredVariable::Kind::ArgInnerVariable );
				function_context.variables_state.AddPollutionForArgInnerVariable( this_storage, inner_variable );
				function_context.variables_state.AddVariable( inner_variable );
				args_stored_variables[arg_number].second= inner_variable;
			}

			arg_number++;
			continue;
		}

		const Synt::FunctionArgument& declaration_arg= *args[ have_implicit_this ? ( arg_number - 1u ) : arg_number ];
		const ProgramString& arg_name= declaration_arg.name_;

		const bool is_this= arg_number == 0u && arg_name == Keywords::this_;
		U_ASSERT( !( is_this && !arg.is_reference ) );
		U_ASSERT( !( have_implicit_this && is_this ) );

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
				address->setName( ToUTF8( arg_name ) );
				function_context.llvm_ir_builder.CreateStore( var.llvm_value, address );

				var.llvm_value= address;
				var.location= Variable::Location::Pointer;
			}
			else if( arg.type.GetClassType() != nullptr )
			{
				// Value classes parameters using llvm-pointers with "byval" attribute.
				var.location= Variable::Location::Pointer;
			}
			else U_ASSERT(false);
		}

		const StoredVariablePtr var_storage=
			std::make_shared<StoredVariable>( arg_name, var, arg.is_reference ? StoredVariable::Kind::ReferenceArg : StoredVariable::Kind::Variable );
		if( arg.is_reference )
			function_context.variables_state.AddVariable( var_storage );
		else
			function_context.stack_variables_stack.back()->RegisterVariable( var_storage );
		var.referenced_variables.emplace(var_storage);

		args_stored_variables[arg_number].first= var_storage;
		// For arguments with references inside create variable storage.
		if( arg.type.ReferencesTagsCount() > 0u )
		{
			U_ASSERT( arg.type.ReferencesTagsCount() == 1u ); // Currently, support 0 or 1 tags.
			const StoredVariablePtr inner_variable = std::make_shared<StoredVariable>( arg_name + " inner reference"_SpC, Variable(), StoredVariable::Kind::ArgInnerVariable );
			function_context.variables_state.AddVariable( inner_variable );
			function_context.variables_state.AddPollutionForArgInnerVariable( var_storage, inner_variable );
			args_stored_variables[arg_number].second= inner_variable;
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
			{
				errors_.push_back( ReportDeclarationShadowsTemplateArgument( declaration_arg.file_pos_, arg_name ) );
				return;
			}

			const NamesScope::InsertedName* const inserted_arg=
				function_names.AddName( arg_name, Value( var_storage, declaration_arg.file_pos_ ) );
			if( !inserted_arg )
			{
				errors_.push_back( ReportRedefinition( declaration_arg.file_pos_, arg_name ) );
				return;
			}
		}

		llvm_arg.setName( "_arg_" + ToUTF8( arg_name ) );
		++arg_number;
	}

	if( function_type->return_value_is_reference || function_type->return_type.ReferencesTagsCount() > 0u )
	{
		// Fill list of allowed for returning references.
		for (size_t i= 0u; i < function_type->args.size(); ++i )
		{
			const Function::Arg& arg= function_type->args[i];

			// For reference arguments try add reference to list of allowed for returning references.
			if( arg.is_reference )
			{
				for( const size_t arg_n : function_type->return_references.args_references )
				{
					if( arg_n == i )
					{
						function_context.allowed_for_returning_references.emplace( args_stored_variables[i].first );
						break;
					}
				}
			}
			if( arg.type.ReferencesTagsCount() > 0u )
			{
				for( const Function::ArgReference& arg_and_tag : function_type->return_references.inner_args_references )
				{
					if( arg_and_tag.first == i && arg_and_tag.second == 0u )
					{
						function_context.allowed_for_returning_references.emplace( args_stored_variables[i].second );
						break;
					}
				}
			}
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

	const BlockBuildInfo block_build_info= BuildBlockCode( *block, function_names, function_context );
	U_ASSERT( function_context.stack_variables_stack.size() == 1u );


	if( func_variable.constexpr_kind != FunctionVariable::ConstexprKind::NonConstexpr )
	{
		// Check function type and function body.
		// Function type checked here, because in case of constexpr methods not all types are complete yet.

		// For auto-constexpr functions we do not force type completeness. If function is really-constexpr, it must already make complete using types.

		const bool auto_contexpr= func_variable.constexpr_kind == FunctionVariable::ConstexprKind::ConstexprAuto;
		bool can_be_constexpr= true;

		if( !auto_contexpr )
		{
			if( function_type->return_type != void_type_for_ret_ && !EnsureTypeCompleteness( function_type->return_type, TypeCompleteness::Complete ) )
				errors_.push_back( ReportUsingIncompleteType( func_variable.body_file_pos, function_type->return_type.ToString() ) ); // Completeness required for constexpr possibility check.
		}

		if( function_type->unsafe ||
			!function_type->return_type.CanBeConstexpr() ||
			!function_type->references_pollution.empty() ) // Side effects, such pollution, not allowed.
			can_be_constexpr= false;

		if( function_type->return_type.GetFunctionPointerType() != nullptr ) // Currently function pointers not supported.
			can_be_constexpr= false;

		for( const Function::Arg& arg : function_type->args )
		{
			if( !auto_contexpr )
			{
				if( arg.type != void_type_ && !EnsureTypeCompleteness( arg.type, TypeCompleteness::Complete ) )
					errors_.push_back( ReportUsingIncompleteType( func_variable.body_file_pos, arg.type.ToString() ) ); // Completeness required for constexpr possibility check.
			}

			if( !arg.type.CanBeConstexpr() ) // Incomplete types are not constexpr.
				can_be_constexpr= false; // Allowed only constexpr types.
			if( arg.type == void_type_ ) // Disallow "void" arguments, because we currently can not constantly convert any reference to "void" in constexpr function call.
				can_be_constexpr= false;
			if( arg.type.GetFunctionPointerType() != nullptr ) // Currently function pointers not supported.
				can_be_constexpr= false;

			// We support constexpr functions with mutable reference-arguments, but such functions can not be used as root for constexpr function evaluation.
			// We support also constexpr constructors (except constexpr copy constructors), but constexpr constructors currently can not e used for constexpr variables initialization.
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
				errors_.push_back( ReportInvalidTypeForConstexprFunction( func_variable.body_file_pos ) );
				func_variable.constexpr_kind= FunctionVariable::ConstexprKind::NonConstexpr;
			}
			else if( function_context.have_non_constexpr_operations_inside )
			{
				errors_.push_back( ReportConstexprFunctionContainsUnallowedOperations( func_variable.body_file_pos ) );
				func_variable.constexpr_kind= FunctionVariable::ConstexprKind::NonConstexpr;
			}
			else
				func_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprComplete;
		}
	}

	// We need call destructors for arguments only if function returns "void".
	// In other case, we have "return" in all branches and destructors call before each "return".
	if( !block_build_info.have_unconditional_return_inside )
	{
		if( function_type->return_type == void_type_ && !function_type->return_value_is_reference )
		{
			// Manually generate "return" for void-return functions.
			CallDestructors( *function_context.stack_variables_stack.back(), function_context, block->end_file_pos_ );

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
			errors_.push_back( ReportNoReturnInFunctionReturningNonVoid( block->end_file_pos_ ) );
			return;
		}
	}

	// Now, we can check references pollution. After this point only code is destructors calls, which can not link references.
	const auto find_reference=
	[&]( const StoredVariablePtr& stored_variable ) -> boost::optional<Function::ArgReference>
	{
		for( size_t i= 0u; i < function_type->args.size(); ++i )
		{
			if( stored_variable == args_stored_variables[i].first )
				return Function::ArgReference( i, Function::c_arg_reference_tag_number );
			if( stored_variable == args_stored_variables[i].second )
				return Function::ArgReference( i, 0u );
		}
		return boost::none;
	};
	for( size_t i= 0u; i < function_type->args.size(); ++i )
	{
		if( args_stored_variables[i].first == nullptr ) // May be in templates.
			continue;

		const auto check_reference=
		[&]( const StoredVariablePtr& referenced_variable )
		{
			const boost::optional<Function::ArgReference> reference= find_reference( referenced_variable );
			if( reference == boost::none )
			{
				errors_.push_back( ReportUnallowedReferencePollution( block->end_file_pos_ ) );
				return;
			}

			Function::ReferencePollution pollution;
			pollution.src= *reference;
			pollution.dst.first= i;
			pollution.dst.second= 0u;
			// Currently check both mutable and immutable. TODO - maybe akt more smarter?
			pollution.src_is_mutable= true;
			if( function_type->references_pollution.count( pollution ) != 0u )
				return;
			pollution.src_is_mutable= false;
			if( function_type->references_pollution.count( pollution ) != 0u )
				return;
			errors_.push_back( ReportUnallowedReferencePollution( block->end_file_pos_ ) );
		};

		if( function_type->args[i].is_reference )
		{
			for( const auto& referenced_variable_pair : function_context.variables_state.GetVariableReferences( args_stored_variables[i].first ) )
			{
				if( referenced_variable_pair.first->kind == StoredVariable::Kind::ArgInnerVariable &&
					referenced_variable_pair.first == args_stored_variables[i].second ) // Ok, arg inner variable.
					continue;
				// TODO - what if self-linking of inner variable occurs?

				check_reference( referenced_variable_pair.first );
			}
		}

		if( args_stored_variables[i].second == nullptr ) // May be in templates.
			continue;

		if( function_type->args[i].type.ReferencesTagsCount() > 0u )
		{
			for( const auto& referenced_variable_pair : function_context.variables_state.GetVariableReferences( args_stored_variables[i].second ) )
				check_reference( referenced_variable_pair.first );
		}
	}

	llvm::Function::BasicBlockListType& bb_list= llvm_function->getBasicBlockList();

	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );

	if( is_destructor )
	{
		// Fill destructors block.
		U_ASSERT( function_context.destructor_end_block != nullptr );
		function_context.llvm_ir_builder.SetInsertPoint( function_context.destructor_end_block );
		bb_list.push_back( function_context.destructor_end_block );

		CallMembersDestructors( function_context, block->end_file_pos_ );
		function_context.llvm_ir_builder.CreateRetVoid();
	}

	// Remove duplicated terminator instructions at end of all function blocks.
	// This needs, for example, when "return" is last operator inside "if" or "while" blocks.
	for (llvm::BasicBlock& block : bb_list)
	{
		llvm::BasicBlock::InstListType& instr_list = block.getInstList();
		while (instr_list.size() >= 2u && instr_list.back().isTerminator() &&
			(std::next(instr_list.rbegin()))->isTerminator())
		{
			instr_list.pop_back();
		}
	}

	// Remove basic blocks without instructions. Such block can exist after if/else with return in all branches, for example.
	auto it= bb_list.begin();
	while(it != bb_list.end())
	{
		if( &*it != function_context.function_basic_block && it->empty() )
			it= bb_list.erase(it);
		else
			++it;
	}
}

void CodeBuilder::BuildConstructorInitialization(
	const Variable& this_,
	const Class& base_class,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::StructNamedInitializer& constructor_initialization_list )
{
	std::set<ProgramString> initialized_fields;

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
				errors_.push_back( ReportBaseUnavailable( constructor_initialization_list.file_pos_ ) );
				continue;
			}
			if( base_initialized )
			{
				have_fields_errors= true;
				errors_.push_back( ReportDuplicatedStructMemberInitializer( constructor_initialization_list.file_pos_, field_initializer.name ) );
				continue;
			}
			base_initialized= true;
			function_context.base_initialized= false;
			continue;
		}

		const NamesScope::InsertedName* const class_member=
			base_class.members.GetThisScopeName( field_initializer.name );

		if( class_member == nullptr )
		{
			have_fields_errors= true;
			errors_.push_back( ReportNameNotFound( constructor_initialization_list.file_pos_, field_initializer.name ) );
			continue;
		}

		const ClassField* const field= class_member->second.GetClassField();
		if( field == nullptr )
		{
			have_fields_errors= true;
			errors_.push_back( ReportInitializerForNonfieldStructMember( constructor_initialization_list.file_pos_, field_initializer.name ) );
			continue;
		}
		if( field->class_.lock()->class_ != &base_class )
		{
			have_fields_errors= true;
			errors_.push_back( ReportInitializerForBaseClassField( constructor_initialization_list.file_pos_, field_initializer.name ) );
			continue;
		}

		if( initialized_fields.find( field_initializer.name ) != initialized_fields.end() )
		{
			have_fields_errors= true;
			errors_.push_back( ReportDuplicatedStructMemberInitializer( constructor_initialization_list.file_pos_, field_initializer.name ) );
			continue;
		}

		initialized_fields.insert( field_initializer.name );
		function_context.uninitialized_this_fields.insert( field );
	} // for fields initializers

	std::set<ProgramString> uninitialized_fields;

	base_class.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;
			if( field->class_.lock()->class_ != &base_class ) // Parent class field.
				return;

			if( initialized_fields.find( member.first ) == initialized_fields.end() )
				uninitialized_fields.insert( member.first );
		} );

	// Initialize fields, missing in initializer list.
	for( const ProgramString& field_name : uninitialized_fields )
	{
		const StackVariablesStorage temp_variables_storage( function_context );

		const NamesScope::InsertedName* const class_member=
			base_class.members.GetThisScopeName( field_name );
		U_ASSERT( class_member != nullptr );
		const ClassField* const field= class_member->second.GetClassField();
		U_ASSERT( field != nullptr );

		if( field->is_reference )
		{
			errors_.push_back( ReportExpectedInitializer( class_member->second.GetFilePos(), field_name ) );
		}
		else
		{
			Variable field_variable;
			field_variable.type= field->type;
			field_variable.location= Variable::Location::Pointer;
			field_variable.value_type= ValueType::Reference;

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			field_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( this_.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			ApplyEmptyInitializer( field_name, constructor_initialization_list.file_pos_, field_variable, function_context );
		}

		CallDestructors( *function_context.stack_variables_stack.back(), function_context, constructor_initialization_list.file_pos_ );
	}
	if( !base_initialized && base_class.base_class != nullptr )
	{
		const StackVariablesStorage temp_variables_storage( function_context );

		// Apply default initializer for base class.
		Variable base_variable;
		base_variable.type= base_class.base_class;
		base_variable.location= Variable::Location::Pointer;
		base_variable.value_type= ValueType::Reference;

		llvm::Value* index_list[2];
		index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(base_class.base_class_field_number) ) );
		base_variable.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( this_.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

		ApplyEmptyInitializer( base_class.base_class->class_->members.GetThisNamespaceName(), constructor_initialization_list.file_pos_, base_variable, function_context );
		function_context.base_initialized= true;

		CallDestructors( *function_context.stack_variables_stack.back(), function_context, constructor_initialization_list.file_pos_ );
	}

	if( have_fields_errors )
		return;

	for( const Synt::StructNamedInitializer::MemberInitializer& field_initializer : constructor_initialization_list.members_initializers )
	{
		const StackVariablesStorage temp_variables_storage( function_context );

		U_ASSERT( this_.referenced_variables.size() == 1u );
		const StoredVariablePtr& this_storage= *this_.referenced_variables.begin();

		if( field_initializer.name == Keywords::base_ )
		{
			Variable base_variable;
			base_variable.type= base_class.base_class;
			base_variable.location= Variable::Location::Pointer;
			base_variable.value_type= ValueType::Reference;

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(base_class.base_class_field_number) ) );
			base_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( this_.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			ApplyInitializer( base_variable, this_storage, *field_initializer.initializer, names_scope, function_context );
			function_context.base_initialized= true;
			continue;
		}

		const NamesScope::InsertedName* const class_member=
			base_class.members.GetThisScopeName( field_initializer.name );
		U_ASSERT( class_member != nullptr );
		const ClassField* const field= class_member->second.GetClassField();
		U_ASSERT( field != nullptr );

		if( field->is_reference )
			InitializeReferenceField( this_, this_storage, *field, *field_initializer.initializer, names_scope, function_context );
		else
		{
			Variable field_variable;
			field_variable.type= field->type;
			field_variable.location= Variable::Location::Pointer;
			field_variable.value_type= ValueType::Reference;

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			field_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( this_.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			U_ASSERT( field_initializer.initializer != nullptr );
			ApplyInitializer( field_variable, this_storage, *field_initializer.initializer, names_scope, function_context );
		}

		function_context.uninitialized_this_fields.erase( field );

		CallDestructors( *function_context.stack_variables_stack.back(), function_context, field_initializer.initializer->GetFilePos() );
	} // for fields initializers

	SetupVirtualTablePointers( this_.llvm_value, base_class, function_context );
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockCode(
	const Synt::Block& block,
	NamesScope& names,
	FunctionContext& function_context )
{
	NamesScope block_names( ""_SpC, &names );
	BlockBuildInfo block_build_info;

	const StackVariablesStorage block_variables_storage( function_context );

	for( const Synt::IBlockElementPtr& block_element : block.elements_ )
	{
		const Synt::IBlockElement* const block_element_ptr= block_element.get();

		const auto try_report_unreachable_code=
		[&]
		{
			const size_t block_element_index= size_t(&block_element - block.elements_.data());
			if( block_element_index + 1u < block.elements_.size() )
				errors_.push_back( ReportUnreachableCode( block.elements_[ block_element_index + 1u ]->GetFilePos() ) );
		};

		if( const auto variables_declaration= dynamic_cast<const Synt::VariablesDeclaration*>( block_element_ptr ) )
			BuildVariablesDeclarationCode( *variables_declaration, block_names, function_context );
		else if( const auto auto_variable_declaration= dynamic_cast<const Synt::AutoVariableDeclaration*>( block_element_ptr ) )
			BuildAutoVariableDeclarationCode( *auto_variable_declaration, block_names, function_context );
		else if( const auto expression= dynamic_cast<const Synt::SingleExpressionOperator*>( block_element_ptr ) )
			BuildExpressionCodeAndDestroyTemporaries(
				*expression->expression_,
				block_names,
				function_context );
		else if( const auto assignment_operator= dynamic_cast<const Synt::AssignmentOperator*>( block_element_ptr ) )
			BuildAssignmentOperatorCode( *assignment_operator, block_names, function_context );
		else if( const auto additive_assignment_operator= dynamic_cast<const Synt::AdditiveAssignmentOperator*>( block_element_ptr ) )
			BuildAdditiveAssignmentOperatorCode( *additive_assignment_operator, block_names, function_context );
		else if( const auto increment_operator= dynamic_cast<const Synt::IncrementOperator*>( block_element_ptr ) )
			BuildDeltaOneOperatorCode(
				*increment_operator->expression,
				increment_operator->file_pos_,
				true,
				block_names,
				function_context );
		else if( const auto decrement_operator= dynamic_cast<const Synt::DecrementOperator*>( block_element_ptr ) )
			BuildDeltaOneOperatorCode(
				*decrement_operator->expression,
				decrement_operator->file_pos_,
				false,
				block_names,
				function_context );
		else if( const auto return_operator= dynamic_cast<const Synt::ReturnOperator*>( block_element_ptr ) )
		{
			BuildReturnOperatorCode(
				*return_operator,
				block_names,
				function_context );

			block_build_info.have_unconditional_return_inside= true;
			try_report_unreachable_code();
		}
		else if( const auto while_operator= dynamic_cast<const Synt::WhileOperator*>( block_element_ptr ) )
			BuildWhileOperatorCode(
				*while_operator,
				block_names,
				function_context );
		else if( const auto break_operator= dynamic_cast<const Synt::BreakOperator*>( block_element_ptr ) )
		{
			BuildBreakOperatorCode(
				*break_operator,
				function_context );

			block_build_info.have_uncodnitional_break_or_continue= true;
			try_report_unreachable_code();
		}
		else if( const auto continue_operator= dynamic_cast<const Synt::ContinueOperator*>( block_element_ptr ) )
		{
			BuildContinueOperatorCode(
				*continue_operator,
				function_context );

			block_build_info.have_uncodnitional_break_or_continue= true;
			try_report_unreachable_code();
		}
		else if( const auto if_operator= dynamic_cast<const Synt::IfOperator*>( block_element_ptr ) )
		{
			const CodeBuilder::BlockBuildInfo if_block_info=
				BuildIfOperatorCode(
					*if_operator,
					block_names,
					function_context );

			block_build_info.have_unconditional_return_inside=
				block_build_info.have_unconditional_return_inside || if_block_info.have_unconditional_return_inside;
			block_build_info.have_uncodnitional_break_or_continue=
				block_build_info.have_uncodnitional_break_or_continue || if_block_info.have_uncodnitional_break_or_continue;

			if( if_block_info.have_unconditional_return_inside ||
				block_build_info.have_uncodnitional_break_or_continue )
				try_report_unreachable_code();
		}
		else if( const auto static_if_operator= dynamic_cast<const Synt::StaticIfOperator*>( block_element_ptr ) )
		{
			const CodeBuilder::BlockBuildInfo static_if_block_info=
				BuildStaticIfOperatorCode(
					*static_if_operator,
					block_names,
					function_context );

			block_build_info.have_unconditional_return_inside=
				block_build_info.have_unconditional_return_inside || static_if_block_info.have_unconditional_return_inside;
			block_build_info.have_uncodnitional_break_or_continue=
				block_build_info.have_uncodnitional_break_or_continue || static_if_block_info.have_uncodnitional_break_or_continue;

			if( static_if_block_info.have_unconditional_return_inside ||
				block_build_info.have_uncodnitional_break_or_continue )
				try_report_unreachable_code();
		}
		else if( const auto static_assert_= dynamic_cast<const Synt::StaticAssert*>( block_element_ptr ) )
			BuildStaticAssert( *static_assert_, block_names );
		else if( const auto halt= dynamic_cast<const Synt::Halt*>( block_element_ptr ) )
		{
			BuildHalt( *halt, function_context );

			block_build_info.have_unconditional_return_inside= true;
			try_report_unreachable_code();
		}
		else if( const auto halt_if= dynamic_cast<const Synt::HaltIf*>( block_element_ptr ) )
			BuildHaltIf( *halt_if, block_names, function_context );
		else if( const auto block= dynamic_cast<const Synt::Block*>( block_element_ptr ) )
		{
			const BlockBuildInfo inner_block_build_info=
				BuildBlockCode( *block, block_names, function_context );

			block_build_info.have_unconditional_return_inside=
				block_build_info.have_unconditional_return_inside || inner_block_build_info.have_unconditional_return_inside;
			block_build_info.have_uncodnitional_break_or_continue=
				block_build_info.have_uncodnitional_break_or_continue || inner_block_build_info.have_uncodnitional_break_or_continue;

			if( inner_block_build_info.have_unconditional_return_inside ||
				block_build_info.have_uncodnitional_break_or_continue )
				try_report_unreachable_code();
		}
		else if( const auto unsafe_block= dynamic_cast<const Synt::UnsafeBlock*>( block_element_ptr ) )
		{
			function_context.have_non_constexpr_operations_inside= true; // Unsafe operations can not be used in constexpr functions.

			const bool prev_unsafe= function_context.is_in_unsafe_block;
			function_context.is_in_unsafe_block= true;

			const BlockBuildInfo inner_block_build_info=
				BuildBlockCode( *unsafe_block->block_, block_names, function_context );

			block_build_info.have_unconditional_return_inside=
				block_build_info.have_unconditional_return_inside || inner_block_build_info.have_unconditional_return_inside;
			block_build_info.have_uncodnitional_break_or_continue=
				block_build_info.have_uncodnitional_break_or_continue || inner_block_build_info.have_uncodnitional_break_or_continue;

			if( inner_block_build_info.have_unconditional_return_inside ||
				block_build_info.have_uncodnitional_break_or_continue )
				try_report_unreachable_code();

			function_context.is_in_unsafe_block= prev_unsafe;
		}
		else U_ASSERT(false);
	}

	// If there are undconditional "break", "continue", "return" operators,
	// we didn`t need call destructors, it must be called in this operators.
	if( !( block_build_info.have_uncodnitional_break_or_continue || block_build_info.have_unconditional_return_inside ) )
		CallDestructors( *function_context.stack_variables_stack.back(), function_context, block.end_file_pos_ );


	return block_build_info;
}

void CodeBuilder::BuildVariablesDeclarationCode(
	const Synt::VariablesDeclaration& variables_declaration,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	const Type type= PrepareType( variables_declaration.type, block_names );

	for( const Synt::VariablesDeclaration::VariableEntry& variable_declaration : variables_declaration.variables )
	{
		if( variable_declaration.reference_modifier != ReferenceModifier::Reference ||
			variable_declaration.mutability_modifier == Synt::MutabilityModifier::Constexpr )
		{
			// Full completeness required for value-variables and any constexpr variable.
			if( !EnsureTypeCompleteness( type, TypeCompleteness::Complete ) )
			{
				errors_.push_back( ReportUsingIncompleteType( variables_declaration.file_pos_, type.ToString() ) );
				continue;
			}
		}

		if( variable_declaration.reference_modifier != ReferenceModifier::Reference && !type.CanBeConstexpr() )
			function_context.have_non_constexpr_operations_inside= true; // Declaring variable with non-constexpr type in constexpr function not allowed.

		if( IsKeyword( variable_declaration.name ) )
		{
			errors_.push_back( ReportUsingKeywordAsName( variables_declaration.file_pos_ ) );
			continue;
		}

		if( variable_declaration.mutability_modifier == MutabilityModifier::Constexpr && !type.CanBeConstexpr() )
		{
			errors_.push_back( ReportInvalidTypeForConstantExpressionVariable( variables_declaration.file_pos_ ) );
			continue;
		}

		// Destruction frame for temporary variables of initializer expression.
		const StackVariablesStorage temp_variables_storage( function_context );

		const StoredVariablePtr stored_variable=
			std::make_shared<StoredVariable>(
				variable_declaration.name,
				Variable(),
				variable_declaration.reference_modifier == ReferenceModifier::Reference ? StoredVariable::Kind::Reference : StoredVariable::Kind::Variable,
				false );
		function_context.stack_variables_stack[ function_context.stack_variables_stack.size() - 2u ]->RegisterVariable( stored_variable );

		Variable& variable= stored_variable->content;
		variable.type= type;
		variable.location= Variable::Location::Pointer;
		variable.value_type= ValueType::Reference;

		if( variable_declaration.reference_modifier == ReferenceModifier::None )
		{
			variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType() );
			variable.llvm_value->setName( ToUTF8( variable_declaration.name ) );

			variable.referenced_variables.insert( stored_variable );
			if( variable_declaration.initializer != nullptr )
				variable.constexpr_value=
					ApplyInitializer( variable, stored_variable, *variable_declaration.initializer, block_names, function_context );
			else
				ApplyEmptyInitializer( variable_declaration.name, variable_declaration.file_pos, variable, function_context );
			variable.referenced_variables.erase( stored_variable );

			// Make immutable, if needed, only after initialization, because in initialization we need call constructors, which is mutable methods.
			if( variable_declaration.mutability_modifier != MutabilityModifier::Mutable )
				variable.value_type= ValueType::ConstReference;

			for( const auto& referenced_variable_pair : function_context.variables_state.GetVariableReferences( stored_variable ) )
				CheckVariableReferences( *referenced_variable_pair.first, variable_declaration.file_pos );
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			// Mark references immutable before initialization.
			if( variable_declaration.mutability_modifier != MutabilityModifier::Mutable )
				variable.value_type= ValueType::ConstReference;

			if( variable_declaration.initializer == nullptr )
			{
				errors_.push_back( ReportExpectedInitializer( variables_declaration.file_pos_, variable_declaration.name ) );
				continue;
			}

			const Synt::IExpressionComponent* initializer_expression= nullptr;
			if( const auto expression_initializer= dynamic_cast<const Synt::ExpressionInitializer*>( variable_declaration.initializer.get() ) )
				initializer_expression= expression_initializer->expression.get();
			else if( const auto constructor_initializer= dynamic_cast<const Synt::ConstructorInitializer*>( variable_declaration.initializer.get() ) )
			{
				if( constructor_initializer->call_operator.arguments_.size() != 1u )
				{
					errors_.push_back( ReportReferencesHaveConstructorsWithExactlyOneParameter( constructor_initializer->file_pos_ ) );
					continue;
				}
				initializer_expression= constructor_initializer->call_operator.arguments_.front().get();
			}
			else
			{
				errors_.push_back( ReportUnsupportedInitializerForReference( variable_declaration.initializer->GetFilePos() ) );
				continue;
			}

			const Variable expression_result= BuildExpressionCodeEnsureVariable( *initializer_expression, block_names, function_context );
			if( !ReferenceIsConvertible( expression_result.type, variable.type, variables_declaration.file_pos_ ) )
			{
				errors_.push_back( ReportTypesMismatch( variables_declaration.file_pos_, variable.type.ToString(), expression_result.type.ToString() ) );
				continue;
			}

			if( expression_result.value_type == ValueType::Value )
			{
				errors_.push_back( ReportExpectedReferenceValue( variables_declaration.file_pos_ ) );
				continue;
			}
			if( expression_result.value_type == ValueType::ConstReference && variable.value_type == ValueType::Reference )
			{
				errors_.push_back( ReportBindingConstReferenceToNonconstReference( variables_declaration.file_pos_ ) );
				continue;
			}

			variable.referenced_variables= expression_result.referenced_variables;

			const bool is_mutable= variable.value_type == ValueType::Reference;
			for( const StoredVariablePtr& referenced_variable : variable.referenced_variables )
				function_context.variables_state.AddPollution( stored_variable, referenced_variable, is_mutable );
			CheckReferencedVariables( stored_variable->content, variable_declaration.file_pos );

			// TODO - maybe make copy of varaible address in new llvm register?
			llvm::Value* result_ref= expression_result.llvm_value;
			if( variable.type != expression_result.type )
				result_ref= CreateReferenceCast( result_ref, expression_result.type, variable.type, function_context );
			variable.llvm_value= result_ref;
			variable.constexpr_value= expression_result.constexpr_value;
		}
		else U_ASSERT(false);

		if( variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
			variable.constexpr_value == nullptr )
		{
			errors_.push_back( ReportVariableInitializerIsNotConstantExpression( variable_declaration.file_pos ) );
			continue;
		}

		// Reset constexpr initial value for mutable variables.
		if( variable.value_type != ValueType::ConstReference )
			variable.constexpr_value= nullptr;

		if( NameShadowsTemplateArgument( variable_declaration.name, block_names ) )
		{
			errors_.push_back( ReportDeclarationShadowsTemplateArgument( variables_declaration.file_pos_, variable_declaration.name ) );
			continue;
		}

		const NamesScope::InsertedName* const inserted_name=
			block_names.AddName( variable_declaration.name, Value( stored_variable, variable_declaration.file_pos ) );
		if( !inserted_name )
		{
			errors_.push_back( ReportRedefinition( variables_declaration.file_pos_, variable_declaration.name ) );
			continue;
		}

		// After lock of references we can call destructors.
		CallDestructors( *function_context.stack_variables_stack.back(), function_context, variable_declaration.file_pos );
	} // for variables
}

void CodeBuilder::BuildAutoVariableDeclarationCode(
	const Synt::AutoVariableDeclaration& auto_variable_declaration,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of initializer expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	const Variable initializer_experrsion= BuildExpressionCodeEnsureVariable( *auto_variable_declaration.initializer_expression, block_names, function_context );

	{ // Check expression type. Expression can have exotic types, such "Overloading functions set", "class name", etc.
		const bool type_is_ok=
			initializer_experrsion.type.GetFundamentalType() != nullptr ||
			initializer_experrsion.type.GetArrayType() != nullptr ||
			initializer_experrsion.type.GetClassType() != nullptr ||
			initializer_experrsion.type.GetEnumType() != nullptr ||
			initializer_experrsion.type.GetFunctionPointerType() != nullptr;
		if( !type_is_ok || initializer_experrsion.type == invalid_type_ )
		{
			errors_.push_back( ReportInvalidTypeForAutoVariable( auto_variable_declaration.file_pos_, initializer_experrsion.type.ToString() ) );
			return;
		}
	}

	const StoredVariablePtr stored_variable=
		std::make_shared<StoredVariable>(
			auto_variable_declaration.name,
			Variable(),
			auto_variable_declaration.reference_modifier == ReferenceModifier::Reference ? StoredVariable::Kind::Reference : StoredVariable::Kind::Variable,
			false );
	function_context.stack_variables_stack[ function_context.stack_variables_stack.size() - 2u ]->RegisterVariable( stored_variable );

	Variable& variable= stored_variable->content;
	variable.type= initializer_experrsion.type;
	variable.value_type= auto_variable_declaration.mutability_modifier == MutabilityModifier::Mutable ? ValueType::Reference : ValueType::ConstReference;
	variable.location= Variable::Location::Pointer;

	if( auto_variable_declaration.reference_modifier != ReferenceModifier::Reference ||
		auto_variable_declaration.mutability_modifier == Synt::MutabilityModifier::Constexpr )
	{
		// Full completeness required for value-variables and any constexpr variable.
		if( !EnsureTypeCompleteness( variable.type, TypeCompleteness::Complete ) )
		{
			errors_.push_back( ReportUsingIncompleteType( auto_variable_declaration.file_pos_, variable.type.ToString() ) );
			return;
		}
	}

	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr && !variable.type.CanBeConstexpr() )
	{
		errors_.push_back( ReportInvalidTypeForConstantExpressionVariable( auto_variable_declaration.file_pos_ ) );
		return;
	}

	if( auto_variable_declaration.reference_modifier == ReferenceModifier::Reference )
	{
		if( initializer_experrsion.value_type == ValueType::Value )
		{
			errors_.push_back( ReportExpectedReferenceValue( auto_variable_declaration.file_pos_ ) );
			return;
		}
		if( initializer_experrsion.value_type == ValueType::ConstReference && variable.value_type != ValueType::ConstReference )
		{
			errors_.push_back( ReportBindingConstReferenceToNonconstReference( auto_variable_declaration.file_pos_ ) );
			return;
		}

		variable.referenced_variables= initializer_experrsion.referenced_variables;

		variable.llvm_value= initializer_experrsion.llvm_value;
		variable.constexpr_value= initializer_experrsion.constexpr_value;

		const bool is_mutable= variable.value_type == ValueType::Reference;
		for( const StoredVariablePtr& referenced_variable : variable.referenced_variables )
			function_context.variables_state.AddPollution( stored_variable, referenced_variable, is_mutable );
		CheckReferencedVariables( variable, auto_variable_declaration.file_pos_ );
	}
	else if( auto_variable_declaration.reference_modifier == ReferenceModifier::None )
	{	
		VariablesState::VariableReferences moved_variable_referenced_variables;

		if( !variable.type.CanBeConstexpr() )
			function_context.have_non_constexpr_operations_inside= true; // Declaring variable with non-constexpr type in constexpr function not allowed.

		variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType() );
		variable.llvm_value->setName( ToUTF8( auto_variable_declaration.name ) );

		if( variable.type.GetFundamentalType() != nullptr || variable.type.GetEnumType() != nullptr || variable.type.GetFunctionPointerType() != nullptr )
		{
			llvm::Value* const value_for_assignment= CreateMoveToLLVMRegisterInstruction( initializer_experrsion, function_context );
			function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );
			variable.constexpr_value= initializer_experrsion.constexpr_value;
		}
		else if( const ClassProxyPtr class_type= variable.type.GetClassTypeProxy() )
		{
			U_ASSERT( class_type->class_->completeness == TypeCompleteness::Complete );
			if( initializer_experrsion.value_type == ValueType::Value )
			{
				U_ASSERT( initializer_experrsion.referenced_variables.size() == 1u );
				const StoredVariablePtr& variable_for_move= *initializer_experrsion.referenced_variables.begin();

				moved_variable_referenced_variables= function_context.variables_state.GetVariableReferences( variable_for_move );
				function_context.variables_state.Move( variable_for_move );

				CopyBytes( initializer_experrsion.llvm_value, variable.llvm_value, variable.type, function_context );
				variable.constexpr_value= initializer_experrsion.constexpr_value; // Move can preserve constexpr.
			}
			else
				TryCallCopyConstructor(
					auto_variable_declaration.file_pos_,
					variable.llvm_value, initializer_experrsion.llvm_value,
					variable.type.GetClassTypeProxy(),
					function_context );
		}
		else
		{
			errors_.push_back( ReportNotImplemented( auto_variable_declaration.file_pos_, "expression initialization for nonfundamental types" ) );
			return;
		}

		// Take references inside variables in initializer expression.
		for( const auto& ref : moved_variable_referenced_variables )
			function_context.variables_state.AddPollution( stored_variable, ref.first, ref.second.IsMutable() );
		for( const StoredVariablePtr& referenced_variable : initializer_experrsion.referenced_variables )
		{
			for( const auto& inner_variable_pair : function_context.variables_state.GetVariableReferences( referenced_variable ) )
			{
				const bool ok= function_context.variables_state.AddPollution( stored_variable, inner_variable_pair.first, inner_variable_pair.second.IsMutable() );
				if(!ok)
					errors_.push_back( ReportReferenceProtectionError( auto_variable_declaration.file_pos_, inner_variable_pair.first->name ) );
			}
		}
	}
	else U_ASSERT(false);

	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
		variable.constexpr_value == nullptr )
	{
		errors_.push_back( ReportVariableInitializerIsNotConstantExpression( auto_variable_declaration.file_pos_ ) );
		return;
	}

	// Reset constexpr initial value for mutable variables.
	if( variable.value_type != ValueType::ConstReference )
		variable.constexpr_value= nullptr;

	if( NameShadowsTemplateArgument( auto_variable_declaration.name, block_names ) )
	{
		errors_.push_back( ReportDeclarationShadowsTemplateArgument( auto_variable_declaration.file_pos_, auto_variable_declaration.name ) );
		return;
	}

	const NamesScope::InsertedName* const inserted_name=
		block_names.AddName( auto_variable_declaration.name, Value( stored_variable, auto_variable_declaration.file_pos_ ) );
	if( inserted_name == nullptr )
		errors_.push_back( ReportRedefinition( auto_variable_declaration.file_pos_, auto_variable_declaration.name ) );

	// After lock of references we can call destructors.
	CallDestructors( *function_context.stack_variables_stack.back(), function_context, auto_variable_declaration.file_pos_ );
}

void CodeBuilder::BuildAssignmentOperatorCode(
	const Synt::AssignmentOperator& assignment_operator,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expressions.
	const StackVariablesStorage temp_variables_storage( function_context );

	if(
		TryCallOverloadedBinaryOperator(
			OverloadedOperator::Assign,
			assignment_operator,
			*assignment_operator.l_value_,
			*assignment_operator.r_value_,
			true, // evaluate args in reverse order
			assignment_operator.file_pos_,
			block_names,
			function_context ) != boost::none )
	{}
	else
	{ // Here process default assignment operator for fundamental types.
		// Evalueate right part
		Variable r_var= BuildExpressionCodeEnsureVariable( *assignment_operator.r_value_, block_names, function_context );

		if(  r_var.type.GetFundamentalType() != nullptr || r_var.type.GetEnumType() != nullptr || r_var.type.GetFunctionPointerType() != nullptr )
		{
			// We must read value, because referenced by reference value may be changed in l_var evaluation.
			if( r_var.location != Variable::Location::LLVMRegister )
			{
				r_var.llvm_value= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
				r_var.location= Variable::Location::LLVMRegister;
			}
			r_var.value_type= ValueType::Value;
		}

		// Evaluate left part.
		const Variable l_var= BuildExpressionCodeEnsureVariable( *assignment_operator.l_value_, block_names, function_context );

		if( l_var.type == invalid_type_ || r_var.type == invalid_type_ )
			return;

		if( l_var.value_type != ValueType::Reference )
		{
			errors_.push_back( ReportExpectedReferenceValue( assignment_operator.file_pos_ ) );
			return;
		}
		if( l_var.type != r_var.type )
		{
			errors_.push_back( ReportTypesMismatch( assignment_operator.file_pos_, l_var.type.ToString(), r_var.type.ToString() ) );
			return;
		}

		// Check references of destination.
		for( const StoredVariablePtr& referenced_variable : l_var.referenced_variables )
		{
			if( referenced_variable->imut_use_counter.use_count() > 1u )
			{
				// Assign to variable, that have nonzero immutable references.
				errors_.push_back( ReportReferenceProtectionError( assignment_operator.file_pos_, referenced_variable->name ) );
			}
		}

		if( l_var.type.GetFundamentalType() != nullptr || l_var.type.GetEnumType() != nullptr || l_var.type.GetFunctionPointerType() != nullptr )
		{
			if( l_var.location != Variable::Location::Pointer )
			{
				U_ASSERT(false);
				return;
			}
			U_ASSERT( r_var.location == Variable::Location::LLVMRegister );
			function_context.llvm_ir_builder.CreateStore( r_var.llvm_value, l_var.llvm_value );
		}
		else
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( assignment_operator.file_pos_, l_var.type.ToString() ) );
			return;
		}
	}
	// Destruct temporary variables of right and left expressions.
	CallDestructors( *function_context.stack_variables_stack.back(), function_context, assignment_operator.file_pos_ );
}

void CodeBuilder::BuildAdditiveAssignmentOperatorCode(
	const Synt::AdditiveAssignmentOperator& additive_assignment_operator,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expressions.
	const StackVariablesStorage temp_variables_storage( function_context );

	if( // TODO - create temp variables frame here.
		TryCallOverloadedBinaryOperator(
			GetOverloadedOperatorForAdditiveAssignmentOperator( additive_assignment_operator.additive_operation_ ),
			additive_assignment_operator,
			*additive_assignment_operator.l_value_,
			*additive_assignment_operator.r_value_,
			true, // evaluate args in reverse order
			additive_assignment_operator.file_pos_,
			block_names,
			function_context ) != boost::none )
	{
		return;
	}
	else
	{ // Here process default additive assignment operators for fundamental types.
		Variable r_var=
			BuildExpressionCodeEnsureVariable(
				*additive_assignment_operator.r_value_,
				block_names,
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

		const Variable l_var=
			BuildExpressionCodeEnsureVariable(
				*additive_assignment_operator.l_value_,
				block_names,
				function_context );

		if( l_var.type == invalid_type_ || r_var.type == invalid_type_)
			return;

		// Check references of destination.
		for( const StoredVariablePtr& stored_variable : l_var.referenced_variables )
		{
			if( stored_variable->imut_use_counter.use_count() > 1u )
			{
				// Assign to variable, that have nonzero immutable references.
				errors_.push_back( ReportReferenceProtectionError( additive_assignment_operator.file_pos_, stored_variable->name ) );
			}
		}

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
					function_context );
			if( operation_result_value.GetVariable() == nullptr ) // Not variable in case of error or if template-dependent stuff.
				return;
			const Variable& operation_result= *operation_result_value.GetVariable();

			if( l_var.value_type != ValueType::Reference )
			{
				errors_.push_back( ReportExpectedReferenceValue( additive_assignment_operator.file_pos_ ) );
				return;
			}

			if( operation_result.type != l_var.type )
			{
				errors_.push_back( ReportTypesMismatch( additive_assignment_operator.file_pos_, l_var.type.ToString(), operation_result.type.ToString() ) );
				return;
			}

			U_ASSERT( l_var.location == Variable::Location::Pointer );
			llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( operation_result, function_context );
			function_context.llvm_ir_builder.CreateStore( value_in_register, l_var.llvm_value );
		}
		else
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( additive_assignment_operator.file_pos_, l_var.type.ToString() ) );
			return;
		}
	}
	// Destruct temporary variables of right and left expressions.
	CallDestructors( *function_context.stack_variables_stack.back(), function_context, additive_assignment_operator.file_pos_ );
}

void CodeBuilder::BuildDeltaOneOperatorCode(
	const Synt::IExpressionComponent& expression,
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
		errors_.push_back( ReportExpectedVariable( file_pos, value.GetKindName() ) );
		return;
	}

	std::vector<Function::Arg> args;
	args.emplace_back();
	args.back().type= variable->type;
	args.back().is_mutable= variable->value_type == ValueType::Reference;
	args.back().is_reference= variable->value_type != ValueType::Value;
	const FunctionVariable* const overloaded_operator=
		GetOverloadedOperator( args, positive ? OverloadedOperator::Increment : OverloadedOperator::Decrement, file_pos );
	if( overloaded_operator != nullptr )
	{
		if( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::NonConstexpr )
			function_context.have_non_constexpr_operations_inside= true;

		if( overloaded_operator->is_this_call )
		{
			const auto fetch_result= TryFetchVirtualFunction( *variable, *overloaded_operator, function_context );
			DoCallFunction( fetch_result.second, *overloaded_operator->type.GetFunctionType(), file_pos, { fetch_result.first }, {}, false, block_names, function_context );
		}
		else
			DoCallFunction( overloaded_operator->llvm_function, *overloaded_operator->type.GetFunctionType(), file_pos, { *variable }, {}, false, block_names, function_context );
	}
	else if( const FundamentalType* const fundamental_type= variable->type.GetFundamentalType() )
	{
		if( !IsInteger( fundamental_type->fundamental_type ) )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, variable->type.ToString() ) );
			return;
		}
		if( variable->value_type != ValueType::Reference )
		{
			errors_.push_back( ReportExpectedReferenceValue( file_pos ) );
			return;
		}

		for( const StoredVariablePtr& referenced_variable : variable->referenced_variables )
		{
			if( referenced_variable->imut_use_counter.use_count() > 1u ) // Changing variable, that have immutable references.
				errors_.push_back( ReportReferenceProtectionError( file_pos, referenced_variable->name ) );
			// If "mut_counter" is not 0 or 1, error must be generated previosly.
		}

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
		errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, variable->type.ToString() ) );
		return;
	}

	CallDestructors( *function_context.stack_variables_stack.back(), function_context, file_pos );
}

void CodeBuilder::BuildReturnOperatorCode(
	const Synt::ReturnOperator& return_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( return_operator.expression_ == nullptr )
	{
		if( !( function_context.return_type == void_type_ && !function_context.return_value_is_reference ) )
		{
			errors_.push_back( ReportTypesMismatch( return_operator.file_pos_, void_type_.ToString(), function_context.return_type.ToString() ) );
			return;
		}

		CallDestructorsBeforeReturn( function_context, return_operator.file_pos_ );

		if( function_context.destructor_end_block == nullptr )
			function_context.llvm_ir_builder.CreateRetVoid();
		else
		{
			// In explicit destructor, break to block with destructor calls for class members.
			function_context.llvm_ir_builder.CreateBr( function_context.destructor_end_block );
		}

		return;
	}

	// Destruction frame for temporary variables of result expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	const Variable expression_result= BuildExpressionCodeEnsureVariable( *return_operator.expression_, names, function_context );
	if( expression_result.type == invalid_type_ )
	{
		// Add "ret void", because we do not need to break llvm basic blocks structure.
		function_context.llvm_ir_builder.CreateRetVoid();
		return;
	}

	if( function_context.return_value_is_reference )
	{
		if( !ReferenceIsConvertible( expression_result.type, function_context.return_type, return_operator.file_pos_ ) )
		{
			errors_.push_back( ReportTypesMismatch( return_operator.file_pos_, function_context.return_type.ToString(), expression_result.type.ToString() ) );
			return;
		}

		if( expression_result.value_type == ValueType::Value )
		{
			errors_.push_back( ReportExpectedReferenceValue( return_operator.file_pos_ ) );
			return;
		}
		if( expression_result.value_type == ValueType::ConstReference && function_context.return_value_is_mutable )
		{
			errors_.push_back( ReportBindingConstReferenceToNonconstReference( return_operator.file_pos_ ) );
			return;
		}

		// Lock references to return value variables.
		std::vector<VariableStorageUseCounter> return_value_locks= LockReferencedVariables( expression_result );

		CallDestructorsBeforeReturn( function_context, return_operator.file_pos_ );
		return_value_locks.clear(); // Reset locks AFTER destructors call. We must get error in case of returning of reference to stack variable or value-argument.

		// Check correctness of returning reference.
		for( const StoredVariablePtr& var : expression_result.referenced_variables )
		{
			if( var->is_global_constant ) // Always allow return of global constants.
				continue;
			if( function_context.allowed_for_returning_references.count(var) == 0u )
				errors_.push_back( ReportReturningUnallowedReference( return_operator.file_pos_ ) );
		}

		llvm::Value* ret_value= expression_result.llvm_value;
		if( expression_result.type != function_context.return_type )
			ret_value= CreateReferenceCast( ret_value, expression_result.type, function_context.return_type, function_context );
		function_context.llvm_ir_builder.CreateRet( ret_value );
	}
	else
	{
		if( expression_result.type != function_context.return_type )
		{
			errors_.push_back( ReportTypesMismatch( return_operator.file_pos_, function_context.return_type.ToString(), expression_result.type.ToString() ) );
			return;
		}

		if( expression_result.type.ReferencesTagsCount() > 0u )
		{
			// Check correctness of returning references.
			for( const StoredVariablePtr& var : expression_result.referenced_variables )
			for( const auto& inner_var :function_context.variables_state.GetVariableReferences( var ) )
			{
				if( inner_var.first->is_global_constant ) // Always allow return of global constants.
					continue;
				if( function_context.allowed_for_returning_references.count(inner_var.first) == 0u )
					errors_.push_back( ReportReturningUnallowedReference( return_operator.file_pos_ ) );
			}
		}

		if( function_context.s_ret_ != nullptr )
		{
			const ClassProxyPtr class_= function_context.return_type.GetClassTypeProxy();
			U_ASSERT( class_ != nullptr );
			if( expression_result.value_type == ValueType::Value )
			{
				U_ASSERT( expression_result.referenced_variables.size() == 1u );
				function_context.variables_state.Move( *expression_result.referenced_variables.begin() );
				CopyBytes( expression_result.llvm_value, function_context.s_ret_, function_context.return_type, function_context );
			}
			else
				TryCallCopyConstructor( return_operator.file_pos_, function_context.s_ret_, expression_result.llvm_value, class_, function_context );

			CallDestructorsBeforeReturn( function_context, return_operator.file_pos_ );
			function_context.llvm_ir_builder.CreateRetVoid();
		}
		else
		{
			// Now we can return by value only fundamentals end enums.
			U_ASSERT( expression_result.type.GetFundamentalType() != nullptr || expression_result.type.GetEnumType() != nullptr || expression_result.type.GetFunctionPointerType() != nullptr );

			// We must read return value before call of destructors.
			llvm::Value* const value_for_return= CreateMoveToLLVMRegisterInstruction( expression_result, function_context );

			CallDestructorsBeforeReturn( function_context, return_operator.file_pos_ );
			function_context.llvm_ir_builder.CreateRet( value_for_return );
		}
	}
}

void CodeBuilder::BuildWhileOperatorCode(
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
	const Variable condition_expression= BuildExpressionCodeEnsureVariable( *while_operator.condition_, names, function_context );

	VariablesState variables_state_before_while= function_context.variables_state;
	variables_state_before_while.DeactivateLocks();

	if( condition_expression.type != bool_type_ )
	{
		errors_.push_back(
			ReportTypesMismatch(
				while_operator.condition_->GetFilePos(),
				bool_type_.ToString(),
				condition_expression.type.ToString() ) );
		return;
	}

	llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
	CallDestructors( *function_context.stack_variables_stack.back(), function_context, while_operator.condition_->GetFilePos() );

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

	BuildBlockCode( *while_operator.block_, names, function_context );
	function_context.llvm_ir_builder.CreateBr( test_block );

	function_context.loops_stack.pop_back();

	// Block after while code.
	function_context.function->getBasicBlockList().push_back( block_after_while );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_while );

	CheckWhileBlokVariablesState( variables_state_before_while, function_context.variables_state, while_operator.block_->end_file_pos_ );
}

void CodeBuilder::BuildBreakOperatorCode(
	const Synt::BreakOperator& break_operator,
	FunctionContext& function_context )
{
	if( function_context.loops_stack.empty() )
	{
		errors_.push_back( ReportBreakOutsideLoop( break_operator.file_pos_ ) );
		return;
	}
	U_ASSERT( function_context.loops_stack.back().block_for_break != nullptr );

	CallDestructorsForLoopInnerVariables( function_context, break_operator.file_pos_ );
	function_context.llvm_ir_builder.CreateBr( function_context.loops_stack.back().block_for_break );
}

void CodeBuilder::BuildContinueOperatorCode(
	const Synt::ContinueOperator& continue_operator,
	FunctionContext& function_context )
{
	if( function_context.loops_stack.empty() )
	{
		errors_.push_back( ReportContinueOutsideLoop( continue_operator.file_pos_ ) );
		return;
	}
	U_ASSERT( function_context.loops_stack.back().block_for_continue != nullptr );

	CallDestructorsForLoopInnerVariables( function_context, continue_operator.file_pos_ );
	function_context.llvm_ir_builder.CreateBr( function_context.loops_stack.back().block_for_continue );
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildIfOperatorCode(
	const Synt::IfOperator& if_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	U_ASSERT( !if_operator.branches_.empty() );

	BlockBuildInfo if_operator_blocks_build_info;
	bool have_return_in_all_branches= true;
	bool have_break_or_continue_in_all_branches= true;

	// TODO - optimize this method. Make less basic blocks.
	//

	llvm::BasicBlock* const block_after_if= llvm::BasicBlock::Create( llvm_context_ );

	llvm::BasicBlock* next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
	// Break to first condition. We must push terminal instruction at end of current block.
	function_context.llvm_ir_builder.CreateBr( next_condition_block );

	VariablesState conditions_variable_state= function_context.variables_state;
	conditions_variable_state.DeactivateLocks();
	std::vector<VariablesState> bracnhes_variables_state( if_operator.branches_.size() );

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

		if( branch.condition == nullptr )
		{
			U_ASSERT( i + 1u == if_operator.branches_.size() );

			// Make empty condition block - move to it unconditional break to body.
			function_context.llvm_ir_builder.CreateBr( body_block );
		}
		else
		{
			function_context.variables_state= conditions_variable_state;
			function_context.variables_state.ActivateLocks();
			conditions_variable_state.DeactivateLocks();
			{
				const StackVariablesStorage temp_variables_storage( function_context );
				const Variable condition_expression= BuildExpressionCodeEnsureVariable( *branch.condition, names, function_context );
				if( condition_expression.type != bool_type_ )
				{
					errors_.push_back(
						ReportTypesMismatch(
							branch.condition->GetFilePos(),
							bool_type_.ToString(),
							condition_expression.type.ToString() ) );

					// Create instruction even in case of error, because we needs to store basic blocs somewhere.
					function_context.llvm_ir_builder.CreateCondBr( llvm::UndefValue::get( fundamental_llvm_types_.bool_ ), body_block, next_condition_block );
				}
				else
				{
					llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
					CallDestructors( *function_context.stack_variables_stack.back(), function_context, branch.condition->GetFilePos() );

					function_context.llvm_ir_builder.CreateCondBr( condition_in_register, body_block, next_condition_block );
				}
			}
			conditions_variable_state= function_context.variables_state;
			conditions_variable_state.DeactivateLocks();
		}

		// Make body block code.
		function_context.function->getBasicBlockList().push_back( body_block );
		function_context.llvm_ir_builder.SetInsertPoint( body_block );

		const BlockBuildInfo block_build_info=
			BuildBlockCode( *branch.block, names, function_context );

		have_return_in_all_branches= have_return_in_all_branches && block_build_info.have_unconditional_return_inside;
		have_break_or_continue_in_all_branches= have_break_or_continue_in_all_branches && block_build_info.have_uncodnitional_break_or_continue;

		function_context.llvm_ir_builder.CreateBr( block_after_if );

		bracnhes_variables_state[i]= function_context.variables_state;
		bracnhes_variables_state[i].DeactivateLocks();
		function_context.variables_state= conditions_variable_state;
		function_context.variables_state.ActivateLocks();
	}

	U_ASSERT( next_condition_block == block_after_if );

	if( if_operator.branches_.back().condition != nullptr ) // Have no unconditional "else" at end.
		bracnhes_variables_state.push_back( conditions_variable_state );

	if( if_operator.branches_.back().condition != nullptr )
	{
		have_return_in_all_branches= false;
		have_break_or_continue_in_all_branches= false;
	}

	function_context.variables_state= MergeVariablesStateAfterIf( bracnhes_variables_state, if_operator.end_file_pos_ );
	function_context.variables_state.ActivateLocks();
	for( const auto& var_pair : function_context.variables_state.GetVariables() )
		CheckVariableReferences( *var_pair.first, if_operator.end_file_pos_ );

	// Block after if code.
	function_context.function->getBasicBlockList().push_back( block_after_if );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_if );

	if_operator_blocks_build_info.have_unconditional_return_inside= have_return_in_all_branches;
	if_operator_blocks_build_info.have_uncodnitional_break_or_continue= have_break_or_continue_in_all_branches;
	return if_operator_blocks_build_info;
}

void CodeBuilder::BuildStaticAssert( StaticAssert& static_assert_, NamesScope& names )
{
	if( static_assert_.syntax_element == nullptr )
		return;

	BuildStaticAssert( *static_assert_.syntax_element, names );
	static_assert_.syntax_element= nullptr;
}

void CodeBuilder::BuildStaticAssert( const Synt::StaticAssert& static_assert_, NamesScope& names )
{
	const Variable variable= BuildExpressionCodeEnsureVariable( *static_assert_.expression, names, *dummy_function_context_ );
	if( variable.type != bool_type_ )
	{
		errors_.push_back( ReportStaticAssertExpressionMustHaveBoolType( static_assert_.file_pos_ ) );
		return;
	}

	if( variable.constexpr_value == nullptr )
	{
		errors_.push_back( ReportStaticAssertExpressionIsNotConstant( static_assert_.file_pos_ ) );
		return;
	}
	if( llvm::dyn_cast<llvm::UndefValue>(variable.constexpr_value) != nullptr )
	{
		// Undef value means, that value is constexpr, but we are in template prepass, and exact value is unknown. Skip this static_assert
		return;
	}

	if( !variable.constexpr_value->isOneValue() )
	{
		errors_.push_back( ReportStaticAssertionFailed( static_assert_.file_pos_ ) );
		return;
	}
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildStaticIfOperatorCode(
	const Synt::StaticIfOperator& static_if_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	const auto& branches= static_if_operator.if_operator_->branches_;
	for( unsigned int i= 0u; i < branches.size(); i++ )
	{
		const auto& branch= branches[i];
		if( branch.condition != nullptr )
		{
			const Synt::IExpressionComponent& condition= *branch.condition;

			const StackVariablesStorage temp_variables_storage( function_context );

			const Variable condition_expression= BuildExpressionCodeEnsureVariable( condition, names, function_context );
			if( condition_expression.type != bool_type_ )
			{
				errors_.push_back( ReportTypesMismatch( condition.GetFilePos(), bool_type_.ToString(), condition_expression.type.ToString() ) );
				continue;
			}
			if( condition_expression.constexpr_value == nullptr )
			{
				errors_.push_back( ReportExpectedConstantExpression( condition.GetFilePos() ) );
				continue;
			}

			if( condition_expression.constexpr_value->getUniqueInteger().getLimitedValue() != 0u )
				return BuildBlockCode( *branch.block, names, function_context ); // Ok, this static if produdes block.

			CallDestructors( *function_context.stack_variables_stack.back(), function_context, condition.GetFilePos() );

		}
		else
		{
			U_ASSERT( i == branches.size() - 1u );
			return BuildBlockCode( *branch.block, names, function_context );
		}
	}

	return BlockBuildInfo();
}

void CodeBuilder::BuildHalt( const Synt::Halt& halt, FunctionContext& function_context )
{
	U_UNUSED( halt );

	function_context.llvm_ir_builder.CreateCall( halt_func_ );

	// We needs terminal , because call to "halt" is not terminal instruction.
	function_context.llvm_ir_builder.CreateUnreachable();
}

void CodeBuilder::BuildHaltIf(const Synt::HaltIf& halt_if, NamesScope& names, FunctionContext& function_context )
{
	llvm::BasicBlock* const true_block = llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const false_block= llvm::BasicBlock::Create( llvm_context_ );

	const StackVariablesStorage temp_variables_storage( function_context );
	const Variable condition_expression= BuildExpressionCodeEnsureVariable( *halt_if.condition, names, function_context );
	if( condition_expression.type!= bool_type_ )
	{
		errors_.push_back(
			ReportTypesMismatch(
				halt_if.condition->GetFilePos(),
				bool_type_.ToString(),
				condition_expression.type.ToString() ) );
		return;
	}

	llvm::Value* const condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
	CallDestructors( *function_context.stack_variables_stack.back(), function_context, halt_if.condition->GetFilePos() );

	function_context.llvm_ir_builder.CreateCondBr( condition_in_register, true_block, false_block );

	// True branch
	function_context.function->getBasicBlockList().push_back( true_block );
	function_context.llvm_ir_builder.SetInsertPoint( true_block );

	function_context.llvm_ir_builder.CreateCall( halt_func_ );
	function_context.llvm_ir_builder.CreateUnreachable();

	// False branch
	function_context.function->getBasicBlockList().push_back( false_block );
	function_context.llvm_ir_builder.SetInsertPoint( false_block );
}

void CodeBuilder::BuildTypedef(
	const Synt::Typedef& typedef_,
	NamesScope& names )
{
	const Type type= PrepareType( typedef_.value, names );
	if( type == invalid_type_ )
		return;

	if( NameShadowsTemplateArgument( typedef_.name, names ) )
		errors_.push_back( ReportDeclarationShadowsTemplateArgument( typedef_.file_pos_, typedef_.name ) );

	const NamesScope::InsertedName* const inserted_name= names.AddName( typedef_.name, Value( type, typedef_.file_pos_ ) );
	if( inserted_name == nullptr )
		errors_.push_back( ReportRedefinition( typedef_.file_pos_, typedef_.name ) );
}

NamesScope::InsertedName* CodeBuilder::ResolveName( const FilePos& file_pos, NamesScope& names_scope, const Synt::ComplexName& complex_name, const ResolveMode resolve_mode )
{
	return ResolveName( file_pos, names_scope, complex_name.components.data(), complex_name.components.size(), resolve_mode );
}

NamesScope::InsertedName* CodeBuilder::ResolveName(
	const FilePos& file_pos,
	NamesScope& names_scope,
	const Synt::ComplexName::Component* components,
	size_t component_count,
	const ResolveMode resolve_mode  )
{
	U_ASSERT( component_count > 0u );

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
		const ProgramString& start= components[0].name;
		NamesScope* space= &names_scope;
		while(true)
		{
			NamesScope::InsertedName* const find= space->GetThisScopeName( start );
			if( find != nullptr )
				break;
			space= space->GetParent();
			if( space == nullptr )
				return nullptr;
		}
		last_space= space;
	}

	NamesScope::InsertedName* name= nullptr;
	while( true )
	{
		name= last_space->GetThisScopeName( components[0].name );
		if( name == nullptr )
			return nullptr;

		if( components[0].have_template_parameters && name->second.GetTypeTemplatesSet() == nullptr && name->second.GetFunctionsSet() == nullptr )
		{
			errors_.push_back( ReportValueIsNotTemplate( file_pos ) );
			return nullptr;
		}

		NamesScope* next_space= nullptr;
		ClassProxyPtr next_space_class= nullptr;

		if( const NamesScopePtr inner_namespace= name->second.GetNamespace() )
			next_space= inner_namespace.get();
		else if( const Type* const type= name->second.GetTypeName() )
		{
			if( Class* const class_= type->GetClassType() )
			{
				if( component_count >= 2u )
				{
					if( class_->syntax_element != nullptr && class_->syntax_element->is_forward_declaration_ )
					{
						errors_.push_back( ReportUsingIncompleteType( file_pos, type->ToString() ) );
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
		else if( TypeTemplatesSet* const type_templates_set = name->second.GetTypeTemplatesSet() )
		{
			GlobalThingBuildTypeTemplatesSet( *last_space, *type_templates_set );
			if( components[0].have_template_parameters && !( resolve_mode == ResolveMode::ForTemplateSignatureParameter && component_count == 1u ) )
			{
				NamesScope::InsertedName* generated_type=
					GenTemplateType(
						file_pos,
						*type_templates_set,
						components[0].template_parameters,
						names_scope );
				if( generated_type == nullptr )
					return nullptr;

				const Type* const type= generated_type->second.GetTypeName();
				U_ASSERT( type != nullptr );
				if( Class* const class_= type->GetClassType() )
				{
					next_space= &class_->members;
					next_space_class= type->GetClassTypeProxy();
				}
				name= generated_type;
			}
			else if( component_count >= 2u )
			{
				errors_.push_back( ReportTemplateInstantiationRequired( file_pos, type_templates_set->type_templates.front()->syntax_element->name_ ) );
				return nullptr;
			}
		}
		else if( OverloadedFunctionsSet* const functions_set= name->second.GetFunctionsSet() )
		{
			if( resolve_mode != ResolveMode::ForDeclaration )
				GlobalThingBuildFunctionsSet( *last_space, *functions_set, false );
			if( components[0].have_template_parameters )
			{
				if( functions_set->template_functions.empty() )
				{
					errors_.push_back( ReportValueIsNotTemplate( file_pos ) );
					return nullptr;
				}

				name=
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
			name= next_space->GetThisScopeName( components[1].name );

			if( next_space_class != nullptr && resolve_mode != ResolveMode::ForDeclaration &&
				names_scope.GetAccessFor( next_space_class ) < next_space_class->class_->GetMemberVisibility( components[1].name ) )
				errors_.push_back( ReportAccessingNonpublicClassMember( file_pos, next_space_class->class_->members.GetThisNamespaceName(), components[1].name ) );
		}
		else
			return nullptr;

		++components;
		--component_count;
		last_space= next_space;
	}

	if( name != nullptr && name->second.GetYetNotDeducedTemplateArg() != nullptr )
		errors_.push_back( ReportTemplateArgumentIsNotDeducedYet( file_pos, name == nullptr ? ""_SpC : name->first ) );

	// Complete some things in resolve.
	if( name != nullptr && resolve_mode != ResolveMode::ForDeclaration )
	{
		if( OverloadedFunctionsSet* const functions_set= name->second.GetFunctionsSet() )
			GlobalThingBuildFunctionsSet( *last_space, *functions_set, false );
		else if( TypeTemplatesSet* const type_templates_set= name->second.GetTypeTemplatesSet() )
			GlobalThingBuildTypeTemplatesSet( *last_space, *type_templates_set );
		else if( name->second.GetTypedef() != nullptr )
			GlobalThingBuildTypedef( *last_space, name->second );
		else if( name->second.GetIncompleteGlobalVariable() != nullptr )
			GlobalThingBuildVariable( *last_space, name->second );
	}
	return name;
}

U_FundamentalType CodeBuilder::GetNumericConstantType( const Synt::NumericConstant& number )
{
	if( number.type_suffix_.empty() )
	{
		if( number.has_fractional_point_ )
			return U_FundamentalType::f64;
		else
			return U_FundamentalType::i32;
	}

	// Allow simple "u" suffix for unsigned 32bit values.
	// SPRACHE_TODO - maybe add "i" suffix for i32 type?
	if( number.type_suffix_ == "u"_SpC )
		return U_FundamentalType::u32;
	// Simple "f" suffix for 32bit floats.
	else if( number.type_suffix_ == "f"_SpC )
		return U_FundamentalType::f32;
	// Short suffixes for chars
	else if( number.type_suffix_ ==  "c8"_SpC )
		return U_FundamentalType::char8 ;
	else if( number.type_suffix_ == "c16"_SpC )
		return U_FundamentalType::char16;
	else if( number.type_suffix_ == "c32"_SpC )
		return U_FundamentalType::char32;

	auto it= g_types_map.find( number.type_suffix_ );
	if( it == g_types_map.end() )
		return U_FundamentalType::InvalidType;

	return it->second;
}

llvm::Type* CodeBuilder::GetFundamentalLLVMType( const U_FundamentalType fundmantal_type )
{
	switch( fundmantal_type )
	{
	case U_FundamentalType::InvalidType:
		return fundamental_llvm_types_.invalid_type_;
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

llvm::Value* CodeBuilder::CreateReferenceCast( llvm::Value* const ref, const Type& src_type, const Type& dst_type, FunctionContext& function_context )
{
	U_ASSERT( src_type.ReferenceIsConvertibleTo( dst_type ) );

	if( dst_type == void_type_ )
		return function_context.llvm_ir_builder.CreatePointerCast( ref, llvm::PointerType::get( dst_type.GetLLVMType(), 0 ) );
	else
	{
		const Class* const src_class_type= src_type.GetClassType();
		U_ASSERT( src_class_type != nullptr );

		for( const ClassProxyPtr& src_parent_class : src_class_type->parents )
		{
			const size_t parent_index= &src_parent_class - src_class_type->parents.data();
			if( src_parent_class == dst_type )
			{
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(src_class_type->parents_fields_numbers[parent_index]) ) );
				return function_context.llvm_ir_builder.CreateGEP( ref, llvm::ArrayRef< llvm::Value*> ( index_list, 2u ) );
			}
			else if( Type(src_parent_class).ReferenceIsConvertibleTo( dst_type ) )
			{
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(src_class_type->parents_fields_numbers[parent_index]) ) );
				llvm::Value* const sub_ref= function_context.llvm_ir_builder.CreateGEP( ref, llvm::ArrayRef< llvm::Value*> ( index_list, 2u ) );
				return CreateReferenceCast( sub_ref, src_parent_class, dst_type, function_context );
			}
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
	llvm::GlobalVariable* const val=
		new llvm::GlobalVariable(
			*module_,
			type.GetLLVMType(),
			true, // is constant
			llvm::GlobalValue::InternalLinkage, // We have no external variables, so, use internal linkage.
			initializer,
			mangled_name );

	val->setUnnamedAddr( true );

	return val;
}

void CodeBuilder::SetupGeneratedFunctionLinkageAttributes( llvm::Function& function )
{
	// Merge functions with identical code.
	// We doesn`t need different addresses for different functions.
	function.setUnnamedAddr( true );

	// Set comdat for correct linkage of same functions, emitted in several modules.
	llvm::Comdat* const comdat= module_->getOrInsertComdat( function.getName() );
	comdat->setSelectionKind( llvm::Comdat::Any ); // Actually, we needs something, like ExactMatch, but it works not in all cases.
	function.setComdat( comdat );
}

} // namespace CodeBuilderLLVMPrivate

} // namespace U
