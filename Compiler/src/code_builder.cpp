#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include "pop_llvm_warnings.hpp"

#include "assert.hpp"
#include "keywords.hpp"
#include "lang_types.hpp"
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
	function_context.stack_variables_stack.pop_back();
}

void CodeBuilder::StackVariablesStorage::RegisterVariable( const StoredVariablePtr& variable )
{
	variables.push_back( variable );
}

CodeBuilder::CodeBuilder()
	: llvm_context_( llvm::getGlobalContext() )
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

	fundamental_llvm_types_.invalid_type_= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.void_= llvm::Type::getVoidTy( llvm_context_ );
	fundamental_llvm_types_.bool_= llvm::Type::getInt1Ty( llvm_context_ );

	invalid_type_= FundamentalType( U_FundamentalType::InvalidType, fundamental_llvm_types_.invalid_type_ );
	void_type_= FundamentalType( U_FundamentalType::Void, fundamental_llvm_types_.void_ );
	bool_type_= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );

	// Default resolve handler - push first to stack.
	resolving_funcs_stack_.emplace_back( new PreResolveFunc(
		[this](
			NamesScope& names_scope,
			const Synt::ComplexName::Component* components,
			size_t component_count,
			size_t& out_skip_components  )
		{
			return PreResolveDefault( names_scope, components, component_count, out_skip_components );
		} ) );
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
			ToStdString( source_graph.nodes_storage[ source_graph.root_node_index ].file_path ),
			llvm_context_ ) );

	// In some places outside functions we need to execute expression evaluation.
	// Create for this dummy function context.
	llvm::Function* const dummy_function=
		llvm::Function::Create(
			llvm::FunctionType::get( fundamental_llvm_types_.void_, false ),
			llvm::Function::LinkageTypes::ExternalLinkage,
			"",
			module_.get() );

	// Prepare halt func.
	{
		llvm::FunctionType* void_function_type= llvm::FunctionType::get( fundamental_llvm_types_.void_, false );
		halt_func_= llvm::Function::Create( void_function_type, llvm::Function::ExternalLinkage, "__U_halt", module_.get() );
		halt_func_->setDoesNotReturn();
		halt_func_->setDoesNotThrow();
	}

	FunctionContext dummy_function_context(
		void_type_,
		false, false,
		llvm_context_,
		dummy_function );
	const StackVariablesStorage dummy_function_variables_storage( dummy_function_context );
	dummy_function_context_= &dummy_function_context;

	// Build graph.
	BuildResultInternal build_result_internal=
		BuildProgramInternal( source_graph, source_graph.root_node_index );

	dummy_function->eraseFromParent(); // Kill dummy function.

	compiled_sources_cache_.clear();

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
	BuildNamespaceBody( source_graph_node.ast.program_elements, *result.names_map );

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
				}
				else if( const TypeTemplatePtr type_template= src_member.second.GetTypeTemplate() )
				{
					// Type template must know it parent namespace.
					// Change namespace at this point.
					if( type_template->parent_namespace == &src )
						type_template->parent_namespace= &dst;
					dst.AddName( src_member.first, src_member.second );
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

				for( const FunctionVariable& src_func : *src_funcs_set )
				{
					FunctionVariable* same_dst_func=
						GetFunctionWithExactSignature( *src_func.type.GetFunctionType(), *dst_funcs_set );
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

					const std::shared_ptr<Class> dst_class= dst_class_table[dst_class_proxy];
					U_ASSERT( dst_class != nullptr );
					const Class& src_class= *src_class_proxy->class_;

					U_ASSERT( dst_class->forward_declaration_file_pos == src_class.forward_declaration_file_pos );

					if(  dst_class->is_incomplete &&  src_class.is_incomplete )
					{} // Ok
					if( !dst_class->is_incomplete &&  src_class.is_incomplete )
					{} // Dst class is complete, so, use it.
					if( !dst_class->is_incomplete && !src_class.is_incomplete &&
						 dst_class->body_file_pos != src_class.body_file_pos )
					{
						// Different bodies from different files.
						errors_.push_back( ReportClassBodyDuplication( src_class.body_file_pos ) );
					}
					if(  dst_class->is_incomplete && !src_class.is_incomplete )
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
	const std::shared_ptr<Class> copy=
		std::make_shared<Class>( src.members.GetThisNamespaceName(), &dst_namespace );

	// Make deep copy of inner namespace.
	MergeNameScopes( copy->members, src.members, dst_class_table );

	// Copy fields.
	copy->field_count= src.field_count;
	copy->is_incomplete= src.is_incomplete;

	copy->have_explicit_noncopy_constructors= src.have_explicit_noncopy_constructors;
	copy->is_default_constructible= src.is_default_constructible;
	copy->is_copy_constructible= src.is_copy_constructible;
	copy->have_destructor= src.have_destructor;

	copy->forward_declaration_file_pos= src.forward_declaration_file_pos;
	copy->body_file_pos= src.body_file_pos;

	copy->llvm_type= src.llvm_type;
	copy->base_template= src.base_template;

	// Register copy in destination namespace and current class table.
	dst_namespace.AddName( src.members.GetThisNamespaceName(), Value( src_class, file_pos ) );
	dst_class_table[ src_class ]= copy;
}

void CodeBuilder::SetCurrentClassTable( ClassTable& table )
{
	current_class_table_= &table;

	// Make all ClassProxy pointed to Classes from current table.
	for( const ClassTable::value_type& table_entry : table )
		table_entry.first->class_= table_entry.second;
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
}

Type CodeBuilder::PrepareType(
	const FilePos& file_pos,
	const Synt::TypeName& type_name,
	NamesScope& names_scope )
{
	Type result;
	Type* last_type= &result;

	constexpr unsigned int c_max_array_dimensions= 16u;
	Type* arrays_stack[ c_max_array_dimensions ];
	unsigned int arrays_count= 0u;

	// Fill arrays hierarchy.
	for( auto rit= type_name.array_sizes.rbegin(); rit != type_name.array_sizes.rend(); ++rit )
	{
		if( arrays_count >= c_max_array_dimensions )
		{
			U_ASSERT( false && "WTF?" );
		}

		arrays_stack[ arrays_count ]= last_type;
		arrays_count++;

		const Synt::IExpressionComponent& num= * *rit;
		const FilePos& num_file_pos= num.GetFilePos();

		*last_type= Array();
		Array& array_type= *last_type->GetArrayType();

		const Value size_expression= BuildExpressionCode( num, names_scope, *dummy_function_context_ );
		if( size_expression.GetType() == NontypeStub::TemplateDependentValue ||
			size_expression.GetType().GetTemplateDependentType() != nullptr )
		{}
		else if( const Variable* const size_variable= size_expression.GetVariable() )
		{
			if( size_variable->constexpr_value != nullptr )
			{
				if( const FundamentalType* const size_fundamental_type= size_variable->type.GetFundamentalType() )
				{
					if( IsInteger( size_fundamental_type->fundamental_type ) )
					{
						if( llvm::dyn_cast<llvm::UndefValue>(size_variable->constexpr_value) != nullptr )
							array_type.size= Array::c_undefined_size;
						else
						{
							const llvm::APInt& size_value= size_variable->constexpr_value->getUniqueInteger();
							if( IsSignedInteger( size_fundamental_type->fundamental_type ) && size_value.isNegative() )
								errors_.push_back( ReportArraySizeIsNegative( num_file_pos ) );
							else
								array_type.size= SizeType( size_value.getLimitedValue() );
						}
					}
					else
						errors_.push_back( ReportArraySizeIsNotInteger( num_file_pos ) );
				}
				else
					U_ASSERT( false && "Nonfundamental constexpr? WTF?" );
			}
			else
				errors_.push_back( ReportExpectedConstantExpression( num_file_pos ) );
		}
		else
			errors_.push_back( ReprotExpectedVariableInArraySize( num_file_pos, size_expression.GetType().ToString() ) );

		last_type= &array_type.type;
	}

	*last_type= FundamentalType( U_FundamentalType::InvalidType, fundamental_llvm_types_.invalid_type_ );

	if( const NamesScope::InsertedName* name=
		ResolveName( file_pos, names_scope, type_name.name ) )
	{
		if( name->second.GetType() == NontypeStub::TemplateDependentValue )
			return GetNextTemplateDependentType();
		else if( const Type* const type= name->second.GetTypeName() )
			*last_type= *type;
		else
			errors_.push_back( ReportNameIsNotTypeName( file_pos, name->first ) );
	}
	else
		errors_.push_back( ReportNameNotFound( file_pos, type_name.name ) );

	// Setup arrays llvm types.
	if( arrays_count > 0u )
	{
		{
			Array* const array_type= arrays_stack[ arrays_count - 1u ]->GetArrayType();
			U_ASSERT( array_type != nullptr );

				array_type->llvm_type=
				llvm::ArrayType::get(
					last_type->GetLLVMType(),
					array_type->ArraySizeOrZero() );
		}

		for( unsigned int i= arrays_count - 1u; i > 0u; i-- )
		{
			Array* const array_type= arrays_stack[ i - 1u ]->GetArrayType();
			U_ASSERT( array_type != nullptr );

			array_type->llvm_type=
				llvm::ArrayType::get(
					arrays_stack[i]->GetLLVMType(),
					array_type->ArraySizeOrZero() );
		}
	}

	// TODO - generate error, if total size of type (incuding arrays) is more, than half of address space of target architecture.

	return result;
}

ClassProxyPtr CodeBuilder::PrepareClass(
	const Synt::Class& class_declaration,
	const Synt::ComplexName& class_complex_name,
	NamesScope& names_scope,
	const bool force_forward_declaration )
{
	const ProgramString& class_name= class_complex_name.components.back().name;
	if( IsKeyword( class_name ) )
		errors_.push_back( ReportUsingKeywordAsName( class_declaration.file_pos_ ) );

	if( class_declaration.is_forward_declaration_ || force_forward_declaration )
	{
		if( class_declaration.name_.components.size() != 1u )
		{
			errors_.push_back( ReportClassDeclarationOutsideItsScope( class_declaration.file_pos_ ) );
			return nullptr;
		}

		const ClassProxyPtr the_class_proxy= std::make_shared<ClassProxy>( new Class( class_name, &names_scope ) );
		Class* const the_class= the_class_proxy->class_.get();
		(*current_class_table_)[ the_class_proxy ]= the_class_proxy->class_;
		the_class->llvm_type= llvm::StructType::create( llvm_context_, MangleType( the_class_proxy ) );
		the_class->forward_declaration_file_pos= class_declaration.file_pos_;
		const Type class_type= the_class_proxy;

		if( NameShadowsTemplateArgument( class_name, names_scope ) )
		{
			errors_.push_back( ReportDeclarationShadowsTemplateArgument( class_declaration.file_pos_, class_name ) );
			return nullptr;
		}

		const NamesScope::InsertedName* const inserted_name= names_scope.AddName( class_name, Value( class_type, class_declaration.file_pos_ ) );
		if( inserted_name == nullptr )
		{
			errors_.push_back( ReportRedefinition( class_declaration.file_pos_, class_name ) );
			return nullptr;
		}

		return the_class_proxy;
	}

	Class* the_class= nullptr;
	ClassProxyPtr the_class_proxy;

	const NamesScope::InsertedName* previous_declaration= nullptr;
	if( class_complex_name.components.size() == 1u )
	{
		// Simple name - look only in current namespace.
		previous_declaration= names_scope.GetThisScopeName( class_complex_name.components.front().name );
	}
	else
	{
		// Complex name - make full name resolving.
		previous_declaration= ResolveName( class_declaration.file_pos_, names_scope, class_complex_name );
		if( previous_declaration == nullptr )
		{
			errors_.push_back( ReportClassDeclarationOutsideItsScope( class_declaration.file_pos_ ) );
			return nullptr;
		}
	}

	if( previous_declaration == nullptr )
	{
		the_class_proxy= std::make_shared<ClassProxy>( new Class( class_name, &names_scope ) );
		the_class= the_class_proxy->class_.get();
		(*current_class_table_)[ the_class_proxy ]= the_class_proxy->class_;
		the_class->llvm_type= llvm::StructType::create( llvm_context_, MangleType( the_class_proxy ) );
		the_class->forward_declaration_file_pos= class_declaration.file_pos_;
		Type class_type;
		class_type= the_class_proxy;

		if( NameShadowsTemplateArgument( class_name, names_scope ) )
		{
			errors_.push_back( ReportDeclarationShadowsTemplateArgument( class_declaration.file_pos_, class_name ) );
			return nullptr;
		}

		const NamesScope::InsertedName* const inserted_name= names_scope.AddName( class_name, Value( class_type, class_declaration.file_pos_ ) );
		if( inserted_name == nullptr )
		{
			errors_.push_back( ReportRedefinition( class_declaration.file_pos_, class_name ) );
			return nullptr;
		}
	}
	else
	{
		if( const Type* const previous_type= previous_declaration->second.GetTypeName() )
		{
			if( const ClassProxyPtr previous_class= previous_type->GetClassTypeProxy() )
			{
				if( !previous_class->class_->is_incomplete )
				{
					errors_.push_back( ReportClassBodyDuplication( class_declaration.file_pos_ ) );
					return nullptr;
				}
				the_class_proxy= previous_class;
				the_class= previous_class->class_.get();
			}
			else
			{
				errors_.push_back( ReportRedefinition( class_declaration.file_pos_, class_name ) );
				return nullptr;
			}
		}
		else
		{
			errors_.push_back( ReportRedefinition( class_declaration.file_pos_, class_name ) );
			return nullptr;
		}
	}
	U_ASSERT( the_class != nullptr );
	Type class_type;
	class_type= the_class_proxy;
	the_class->body_file_pos= class_declaration.file_pos_;

	std::vector<llvm::Type*> fields_llvm_types;

	std::vector<PrepareFunctionResult> class_functions;
	std::vector<const Synt::Class*> inner_classes;

	for( const Synt::IClassElementPtr& member : class_declaration.elements_ )
	{
		// TODO - maybe apply visitor?
		if( const auto in_field=
			dynamic_cast<const Synt::ClassField*>( member.get() ) )
		{
			ClassField out_field;
			out_field.type= PrepareType( in_field->file_pos_, in_field->type, the_class->members );
			out_field.index= the_class->field_count;
			out_field.class_= the_class_proxy;

			if( out_field.type.IsIncomplete() )
			{
				errors_.push_back( ReportUsingIncompleteType( class_declaration.file_pos_, out_field.type.ToString() ) );
				continue;
			}

			fields_llvm_types.emplace_back( out_field.type.GetLLVMType() );

			if( NameShadowsTemplateArgument( in_field->name, the_class->members ) )
				errors_.push_back( ReportDeclarationShadowsTemplateArgument( in_field->file_pos_, in_field->name ) );
			else
			{
				const NamesScope::InsertedName* const inserted_field=
					the_class->members.AddName( in_field->name, Value( std::move( out_field ), in_field->file_pos_ ) );
				if( inserted_field == nullptr )
					errors_.push_back( ReportRedefinition( in_field->file_pos_, in_field->name ) );

				the_class->field_count++;
			}
		}
		else if( const auto function_declaration=
			dynamic_cast<const Synt::Function*>( member.get() ) )
		{
			// First time, push only prototypes.
			class_functions.push_back( PrepareFunction( *function_declaration, true, the_class_proxy, the_class->members ) );
		}
		else if( const auto inner_class=
			dynamic_cast<const Synt::Class*>( member.get() ) )
		{
			inner_classes.push_back( inner_class );
			PrepareClass( *inner_class, inner_class->name_, the_class->members, true );
		}
		else if( const auto template_=
			dynamic_cast<const Synt::TemplateBase*>( member.get() ) )
		{
			PrepareTypeTemplate( *template_, the_class->members );
		}
		else if( const auto variables_declaration=
			dynamic_cast<const Synt::VariablesDeclaration*>( member.get() ) )
		{
			BuildVariablesDeclarationCode( *variables_declaration, the_class->members, *dummy_function_context_, true );
		}
		else if( const auto auto_variable_declaration=
			dynamic_cast<const Synt::AutoVariableDeclaration*>( member.get() ) )
		{
			BuildAutoVariableDeclarationCode( *auto_variable_declaration, the_class->members, *dummy_function_context_, true );
		}
		else if( const auto static_assert_=
			dynamic_cast<const Synt::StaticAssert*>( member.get() ) )
		{
			BuildStaticAssert( *static_assert_, the_class->members );
		}
		else if( const auto typedef_=
			dynamic_cast<const Synt::Typedef*>( member.get() ) )
		{
			BuildTypedef( *typedef_, the_class->members );
		}
		else if( const auto typedef_template=
			dynamic_cast<const Synt::TypedefTemplate*>( member.get() ) )
		{
			U_UNUSED( typedef_template );
			// TODO
		}
		else
			U_ASSERT( false );
	}

	// Search for explicit noncopy constructors.
	if( const NamesScope::InsertedName* const constructors_name=
		the_class->members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
	{
		const OverloadedFunctionsSet* const constructors= constructors_name->second.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		for( const FunctionVariable& constructor : *constructors )
		{
			const Function& constructor_type= *constructor.type.GetFunctionType();

			U_ASSERT( constructor_type.args.size() >= 1u && constructor_type.args.front().type == class_type );
			if( !( constructor_type.args.size() == 2u && constructor_type.args.back().type == class_type && !constructor_type.args.back().is_mutable ) )
			{
				the_class->have_explicit_noncopy_constructors= true;
				break;
			}
		};
	}

	// Check opaque before set body for cases of errors (class body duplication).
	if( the_class->llvm_type->isOpaque() )
		the_class->llvm_type->setBody( fields_llvm_types );

	the_class->is_incomplete= false;

	TryGenerateDefaultConstructor( *the_class, class_type );
	TryGenerateCopyConstructor( *the_class, class_type );
	TryGenerateDestructor( *the_class, class_type );

	// Prepare inner classes.
	for( const Synt::Class* const inner_class : inner_classes )
	{
		if( !inner_class->is_forward_declaration_ )
			PrepareClass( *inner_class, inner_class->name_, the_class->members );
	}

	// Build functions with body.
	for( const PrepareFunctionResult& func : class_functions )
	{
		if( func.functions_set != nullptr )
		{
			U_ASSERT( func.func_syntax_element != nullptr );
			if( func.func_syntax_element->block_ == nullptr )
				continue; // This is prototype, it is already processed.

			const ProgramString& func_name= func.func_syntax_element->name_.components.back().name;
			FunctionVariable& function_variable= (*func.functions_set)[ func.function_index ];
			if( function_variable.have_body && func.func_syntax_element->block_ != nullptr )
			{
				errors_.push_back( ReportFunctionBodyDuplication( func.func_syntax_element->file_pos_, func_name ) );
				continue;
			}

			function_variable.body_file_pos= func.func_syntax_element->file_pos_;

			BuildFuncCode(
				function_variable,
				the_class_proxy,
				the_class->members,
				func_name,
				func.func_syntax_element->arguments_,
				func.func_syntax_element->block_.get(),
				func.func_syntax_element->constructor_initialization_list_.get() );
		}
	}

	return the_class_proxy;
}

void CodeBuilder::TryGenerateDefaultConstructor( Class& the_class, const Type& class_type )
{
	// Search for explicit default constructor.
	if( const NamesScope::InsertedName* const constructors_name=
		the_class.members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
	{
		const OverloadedFunctionsSet* const constructors= constructors_name->second.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		for( const FunctionVariable& constructor : *constructors )
		{
			const Function& constructor_type= *constructor.type.GetFunctionType();

			U_ASSERT( constructor_type.args.size() >= 1u && constructor_type.args.front().type == class_type );
			if( ( constructor_type.args.size() == 1u ) )
			{
				the_class.is_default_constructible= true;
				return;
			}
		};
	}

	// Generating of default constructor disabled, if class have other explicit constructors, except copy constructors.
	if( the_class.have_explicit_noncopy_constructors )
		return;

	// Generate default constructor, if all fields is default constructible.
	bool all_fields_is_default_constructible= true;

	the_class.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;

			if( !field->type.IsDefaultConstructible() )
				all_fields_is_default_constructible= false;
		} );

	if( !all_fields_is_default_constructible )
		return;

	// Generate function

	Function constructor_type;
	constructor_type.return_type= void_type_;
	constructor_type.args.emplace_back();
	constructor_type.args.back().type= class_type;
	constructor_type.args.back().is_mutable= true;
	constructor_type.args.back().is_reference= true;

	std::vector<llvm::Type*> args_llvm_types;
	args_llvm_types.push_back( llvm::PointerType::get( class_type.GetLLVMType(), 0u ) );

	constructor_type.llvm_function_type=
		llvm::FunctionType::get(
			fundamental_llvm_types_.void_,
			llvm::ArrayRef<llvm::Type*>( args_llvm_types.data(), args_llvm_types.size() ),
			false );

	llvm::Function* const llvm_constructor_function=
		llvm::Function::Create(
			constructor_type.llvm_function_type,
			llvm::Function::LinkageTypes::ExternalLinkage, // TODO - select linkage
			MangleFunction( the_class.members, Keyword( Keywords::constructor_ ), constructor_type, true ),
			module_.get() );

	llvm_constructor_function->setUnnamedAddr( true );
	llvm_constructor_function->addAttribute( 1u, llvm::Attribute::NonNull ); // this is nonnull

	FunctionContext function_context(
		constructor_type.return_type,
		constructor_type.return_value_is_mutable,
		constructor_type.return_value_is_reference,
		llvm_context_,
		llvm_constructor_function );
	StackVariablesStorage function_variables_storage( function_context );
	llvm::Value* const this_llvm_value= llvm_constructor_function->args().begin();
	this_llvm_value->setName( KeywordAscii( Keywords::this_ ) );

	the_class.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;

			Variable field_variable;
			field_variable.type= field->type;
			field_variable.value_type= ValueType::Reference;

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			field_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( this_llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			ApplyEmptyInitializer( member.first, FilePos()/*TODO*/, field_variable, function_context );
		} );

	function_context.llvm_ir_builder.CreateRetVoid();
	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );

	// Add generated constructor
	FunctionVariable constructor_variable;
	constructor_variable.type= std::move( constructor_type );
	constructor_variable.have_body= true;
	constructor_variable.is_this_call= true;
	constructor_variable.is_generated= true;
	constructor_variable.llvm_function= llvm_constructor_function;

	if( NamesScope::InsertedName* const constructors_name=
		the_class.members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
	{
		OverloadedFunctionsSet* const constructors= constructors_name->second.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		constructors->push_back( std::move( constructor_variable ) );
	}
	else
	{
		OverloadedFunctionsSet constructors;
		constructors.push_back( std::move( constructor_variable ) );
		the_class.members.AddName( Keyword( Keywords::constructor_ ), std::move( constructors ) );
	}

	// After default constructor generation, class is default-constructible.
	the_class.is_default_constructible= true;
}

void CodeBuilder::TryGenerateCopyConstructor( Class& the_class, const Type& class_type )
{
	// Search for explicit copy constructor.
	if( const NamesScope::InsertedName* const constructors_name=
		the_class.members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
	{
		const OverloadedFunctionsSet* const constructors= constructors_name->second.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		for( const FunctionVariable& constructor : *constructors )
		{
			const Function& constructor_type= *constructor.type.GetFunctionType();

			U_ASSERT( constructor_type.args.size() >= 1u && constructor_type.args.front().type == class_type );
			if( constructor_type.args.size() == 2u &&
				constructor_type.args.back().type == class_type && !constructor_type.args.back().is_mutable )
			{
				the_class.is_copy_constructible= true;
				return;
			}
		}
	}

	bool all_fields_is_copy_constructible= true;

	the_class.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;

			if( !field->type.IsCopyConstructible() )
				all_fields_is_copy_constructible= false;
		} );

	if( !all_fields_is_copy_constructible )
		return;

	// Generate copy-constructor
	Function constructor_type;
	constructor_type.return_type= void_type_;
	constructor_type.args.resize(2u);
	constructor_type.args[0].type= class_type;
	constructor_type.args[0].is_mutable= true;
	constructor_type.args[0].is_reference= true;
	constructor_type.args[1].type= class_type;
	constructor_type.args[1].is_mutable= false;
	constructor_type.args[1].is_reference= true;

	std::vector<llvm::Type*> args_llvm_types;
	args_llvm_types.push_back( llvm::PointerType::get( class_type.GetLLVMType(), 0u ) );
	args_llvm_types.push_back( llvm::PointerType::get( class_type.GetLLVMType(), 0u ) );

	constructor_type.llvm_function_type=
		llvm::FunctionType::get(
			fundamental_llvm_types_.void_,
			llvm::ArrayRef<llvm::Type*>( args_llvm_types.data(), args_llvm_types.size() ),
			false );

	llvm::Function* const llvm_constructor_function=
		llvm::Function::Create(
			constructor_type.llvm_function_type,
			llvm::Function::LinkageTypes::ExternalLinkage, // TODO - select linkage
			MangleFunction( the_class.members, Keyword( Keywords::constructor_ ), constructor_type, true ),
			module_.get() );

	llvm_constructor_function->setUnnamedAddr( true );
	llvm_constructor_function->addAttribute( 1u, llvm::Attribute::NonNull ); // this is nonnull
	llvm_constructor_function->addAttribute( 2u, llvm::Attribute::NonNull ); // and src is nonnull

	FunctionContext function_context(
		constructor_type.return_type,
		constructor_type.return_value_is_mutable,
		constructor_type.return_value_is_reference,
		llvm_context_,
		llvm_constructor_function );

	llvm::Value* const this_llvm_value= &*llvm_constructor_function->args().begin();
	this_llvm_value->setName( KeywordAscii( Keywords::this_ ) );
	llvm::Value* const src_llvm_value= &*(++llvm_constructor_function->args().begin());
	src_llvm_value->setName( "src" );

	the_class.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;
			U_ASSERT( field->type.IsCopyConstructible() );

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );

			BuildCopyConstructorPart(
				function_context.llvm_ir_builder.CreateGEP( src_llvm_value , llvm::ArrayRef<llvm::Value*>( index_list, 2u ) ),
				function_context.llvm_ir_builder.CreateGEP( this_llvm_value, llvm::ArrayRef<llvm::Value*>( index_list, 2u ) ),
				field->type,
				function_context );

		} ); // For fields.

	function_context.llvm_ir_builder.CreateRetVoid();
	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );

	// Add generated constructor
	FunctionVariable constructor_variable;
	constructor_variable.type= std::move( constructor_type );
	constructor_variable.have_body= true;
	constructor_variable.is_this_call= true;
	constructor_variable.is_generated= true;
	constructor_variable.llvm_function= llvm_constructor_function;

	if( NamesScope::InsertedName* const constructors_name=
		the_class.members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
	{
		OverloadedFunctionsSet* const constructors= constructors_name->second.GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		constructors->push_back( std::move( constructor_variable ) );
	}
	else
	{
		OverloadedFunctionsSet constructors;
		constructors.push_back( std::move( constructor_variable ) );
		the_class.members.AddName( Keyword( Keywords::constructor_ ), std::move( constructors ) );
	}

	// After default constructor generation, class is copy-constructible.
	the_class.is_copy_constructible= true;
}

void CodeBuilder::TryGenerateDestructor( Class& the_class, const Type& class_type )
{
	// Search for explicit destructor.
	if( const NamesScope::InsertedName* const destructor_name=
		the_class.members.GetThisScopeName( Keyword( Keywords::destructor_ ) ) )
	{
		const OverloadedFunctionsSet* const destructors= destructor_name->second.GetFunctionsSet();
		U_ASSERT( destructors != nullptr && destructors->size() == 1u );
		U_UNUSED( destructors );
		the_class.have_destructor= true;
		return;
	}

	// SPRACHE_TODO - maybe not generate default destructor for classes, that have no fields with destructors?
	// SPRACHE_TODO - maybe mark generated destructor for this cases as "empty"?

	// Generate destructor.
	Function destructor_type;
	destructor_type.return_type= void_type_;
	destructor_type.args.resize(1u);
	destructor_type.args[0].type= class_type;
	destructor_type.args[0].is_mutable= true;
	destructor_type.args[0].is_reference= true;

	llvm::Type* const this_llvm_type= llvm::PointerType::get( class_type.GetLLVMType(), 0u );
	destructor_type.llvm_function_type=
		llvm::FunctionType::get(
			fundamental_llvm_types_.void_,
			llvm::ArrayRef<llvm::Type*>( &this_llvm_type, 1u ),
			false );

	llvm::Function* const llvm_destructor_function=
		llvm::Function::Create(
			destructor_type.llvm_function_type,
			llvm::Function::LinkageTypes::ExternalLinkage, // TODO - select linkage
			MangleFunction( the_class.members, Keyword( Keywords::destructor_ ), destructor_type, true ),
			module_.get() );
	llvm_destructor_function->setUnnamedAddr( true );
	llvm_destructor_function->addAttribute( 1u, llvm::Attribute::NonNull ); // this is nonnull

	llvm::Value* const this_llvm_value= &*llvm_destructor_function->args().begin();
	this_llvm_value->setName( KeywordAscii( Keywords::this_ ) );

	Variable this_;
	this_.type= class_type;
	this_.location= Variable::Location::Pointer;
	this_.value_type= ValueType::Reference;
	this_.llvm_value= this_llvm_value;

	FunctionContext function_context(
		destructor_type.return_type,
		destructor_type.return_value_is_mutable,
		destructor_type.return_value_is_reference,
		llvm_context_,
		llvm_destructor_function );
	function_context.this_= &this_;

	CallMembersDestructors( function_context );
	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );
	function_context.llvm_ir_builder.CreateRetVoid();

	// Add generated destructor.
	FunctionVariable destructor_variable;
	destructor_variable.type= std::move( destructor_type );
	destructor_variable.have_body= true;
	destructor_variable.is_this_call= true;
	destructor_variable.is_generated= true;
	destructor_variable.llvm_function= llvm_destructor_function;

	// TODO - destructor have no overloads. Maybe store it as FunctionVariable, not as FunctionsSet?
	OverloadedFunctionsSet destructors;
	destructors.push_back( std::move( destructor_variable ) );

	the_class.members.AddName(
		Keyword( Keywords::destructor_ ),
		Value( std::move( destructors ) ) );

	// Say "we have destructor".
	the_class.have_destructor= true;
}

void CodeBuilder::BuildCopyConstructorPart(
	llvm::Value* const src, llvm::Value* const dst,
	const Type& type,
	FunctionContext& function_context )
{
	if( const FundamentalType* const fundamental_type= type.GetFundamentalType() )
	{
		// Create simple load-store.
		U_UNUSED( fundamental_type );
		llvm::Value* const val= function_context.llvm_ir_builder.CreateLoad( src );
		function_context.llvm_ir_builder.CreateStore( val, dst );
	}
	else if( const Array* const array_type_ptr= type.GetArrayType() )
	{
		const Array& array_type= *array_type_ptr;

		GenerateLoop(
			array_type.ArraySizeOrZero(),
			[&](llvm::Value* const counter_value)
			{
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= counter_value;

				BuildCopyConstructorPart(
					function_context.llvm_ir_builder.CreateGEP( src, llvm::ArrayRef<llvm::Value*>( index_list, 2u ) ),
					function_context.llvm_ir_builder.CreateGEP( dst, llvm::ArrayRef<llvm::Value*>( index_list, 2u ) ),
					array_type.type,
					function_context );
			},
			function_context);
	}
	else if( const ClassProxyPtr class_type_proxy= type.GetClassTypeProxy() )
	{
		const Type filed_class_type= class_type_proxy;
		const Class& class_type= *class_type_proxy->class_;

		// Search copy constructor.
		const NamesScope::InsertedName* constructor_name=
			class_type.members.GetThisScopeName( Keyword( Keywords::constructor_ ) );
		U_ASSERT( constructor_name != nullptr );
		const OverloadedFunctionsSet* const constructors_set= constructor_name->second.GetFunctionsSet();
		U_ASSERT( constructors_set != nullptr );

		const FunctionVariable* constructor= nullptr;;
		for( const FunctionVariable& candidate_constructor : *constructors_set )
		{
			const Function& constructor_type= *candidate_constructor.type.GetFunctionType();

			if( constructor_type.args.size() == 2u &&
				constructor_type.args.back().type == filed_class_type && !constructor_type.args.back().is_mutable )
			{
				constructor= &candidate_constructor;
				break;
			}
		}
		U_ASSERT( constructor != nullptr );

		// Call it.
		std::vector<llvm::Value*> llvm_args;
		llvm_args.push_back( dst );
		llvm_args.push_back( src );
		function_context.llvm_ir_builder.CreateCall(
			llvm::dyn_cast<llvm::Function>(constructor->llvm_function),
			llvm_args );
	}
	else
	{
		U_ASSERT(false);
	}
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
	for( const FunctionVariable& candidate : *constructors )
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
}

void CodeBuilder::GenerateLoop(
	const SizeType iteration_count,
	const std::function<void(llvm::Value* counter_value)>& loop_body,
	FunctionContext& function_context)
{
	U_ASSERT( loop_body != nullptr );
	if( iteration_count == 0u )
		return;

	llvm::Value* const zero_value=
		llvm::Constant::getIntegerValue( fundamental_llvm_types_.u32, llvm::APInt( 32u, uint64_t(0) ) );
	llvm::Value* const one_value=
		llvm::Constant::getIntegerValue( fundamental_llvm_types_.u32, llvm::APInt( 32u, uint64_t(1u) ) );
	llvm::Value* const loop_count_value=
		llvm::Constant::getIntegerValue( fundamental_llvm_types_.u32, llvm::APInt( 32u, uint64_t(iteration_count) ) );
	llvm::Value* const couter_address= function_context.alloca_ir_builder.CreateAlloca( fundamental_llvm_types_.u32 );
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

		if( stored_variable.is_reference )
		{
			U_ASSERT( stored_variable.content.referenced_variables.size() == stored_variable.locked_referenced_variables.size() );

			// Increment coounter of destroyed reference for referenced variables.
			for( const StoredVariablePtr& referenced_variable : stored_variable.content.referenced_variables )
			{
				if( destroyed_variable_references.find( referenced_variable ) == destroyed_variable_references.end() )
					destroyed_variable_references[ referenced_variable ]= 1u;
				else
					++destroyed_variable_references[ referenced_variable ];
			}
		}
		else
		{
			// Check references.
			U_ASSERT( stored_variable.imut_use_counter.use_count() >= 1u && stored_variable.mut_use_counter.use_count() >= 1u );

			size_t alive_ref_count= ( stored_variable.imut_use_counter.use_count() - 1u ) + ( stored_variable.mut_use_counter.use_count() - 1u );
			const auto map_it= destroyed_variable_references.find( *it );
			if( map_it != destroyed_variable_references.end() )
				alive_ref_count-= map_it->second;

			if( alive_ref_count > 0u )
				errors_.push_back( ReportDestroyedVariableStillHaveReferences( file_pos ) );
		}

		// Call destructors.
		const Variable& var= stored_variable.content;
		if( !stored_variable.is_reference && var.type.HaveDestructor() )
			CallDestructor( var.llvm_value, var.type, function_context );
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
	FunctionContext& function_context )
{
	U_ASSERT( type.HaveDestructor() );

	if( const Class* const class_= type.GetClassType() )
	{
		const NamesScope::InsertedName* const destructor_name= class_->members.GetThisScopeName( Keyword( Keywords::destructor_ ) );
		U_ASSERT( destructor_name != nullptr );
		const OverloadedFunctionsSet* const destructors= destructor_name->second.GetFunctionsSet();
		U_ASSERT(destructors != nullptr && destructors->size() == 1u );

		const FunctionVariable& destructor= destructors->front();
		llvm::Value* const destructor_args[]= { ptr };
		function_context.llvm_ir_builder.CreateCall(
			destructor.llvm_function,
			llvm::ArrayRef<llvm::Value*>( destructor_args, 1u ) );
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
					function_context );
			},
			function_context );
	}
	else
	{
		U_ASSERT( false && "WTF? strange type for variable" );
	}
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

void CodeBuilder::CallMembersDestructors( FunctionContext& function_context )
{
	U_ASSERT( function_context.this_ != nullptr );
	const Class* const class_= function_context.this_->type.GetClassType();
	U_ASSERT( class_ != nullptr );

	class_->members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& member )
		{
			const ClassField* const field= member.second.GetClassField();
			if( field == nullptr )
				return;
			if( !field->type.HaveDestructor() )
				return;

			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			CallDestructor(
				function_context.llvm_ir_builder.CreateGEP( function_context.this_->llvm_value, index_list ),
				field->type,
				function_context );
		} );
}

void CodeBuilder::BuildNamespaceBody(
	const Synt::ProgramElements& body_elements,
	NamesScope& names_scope )
{
	for( const Synt::IProgramElementPtr& program_element : body_elements )
	{
		if( const auto func=
			dynamic_cast<const Synt::Function*>( program_element.get() ) )
		{
			PrepareFunction( *func, false, nullptr, names_scope );
		}
		else if( const auto class_=
			dynamic_cast<const Synt::Class*>( program_element.get() ) )
		{
			PrepareClass( *class_, class_->name_, names_scope );
		}
		else if( const auto namespace_=
			dynamic_cast<const Synt::Namespace*>( program_element.get() ) )
		{
			NamesScope* result_scope= &names_scope;

			const NamesScope::InsertedName* const same_name=
				names_scope.GetThisScopeName( namespace_->name_ );
			if( same_name != nullptr )
			{
				if( const NamesScopePtr same_namespace= same_name->second.GetNamespace() )
					result_scope= same_namespace.get(); // Extend existend namespace.
				else
					errors_.push_back( ReportRedefinition( namespace_->file_pos_, namespace_->name_ ) );
			}
			else
			{

				// There are no templates abowe namespace. Namespaces inside classes does not exists.
				U_ASSERT( !NameShadowsTemplateArgument( namespace_->name_, names_scope ) );

				const NamesScopePtr new_names_scope= std::make_shared<NamesScope>( namespace_->name_, &names_scope );
				names_scope.AddName( namespace_->name_, Value( new_names_scope, namespace_->file_pos_ ) );
				result_scope= new_names_scope.get();
			}

			BuildNamespaceBody( namespace_->elements_, *result_scope );
		}
		else if( const auto tempate_=
			dynamic_cast<const Synt::TemplateBase*>( program_element.get() ) )
		{
			PrepareTypeTemplate( *tempate_, names_scope );
		}
		else if( const auto variables_declaration=
			dynamic_cast<const Synt::VariablesDeclaration*>( program_element.get() ) )
		{
			BuildVariablesDeclarationCode( *variables_declaration, names_scope, *dummy_function_context_, true );
		}
		else if( const auto auto_variable_declaration=
			dynamic_cast<const Synt::AutoVariableDeclaration*>( program_element.get() ) )
		{
			BuildAutoVariableDeclarationCode( *auto_variable_declaration, names_scope, *dummy_function_context_, true );
		}
		else if( const auto static_assert_=
			dynamic_cast<const Synt::StaticAssert*>( program_element.get() ) )
		{
			BuildStaticAssert( *static_assert_, names_scope );
		}
		else if( const auto typedef_=
			dynamic_cast<const Synt::Typedef*>( program_element.get() ) )
		{
			BuildTypedef( *typedef_, names_scope );
		}
		else if( const auto typedef_template=
			dynamic_cast<const Synt::TypedefTemplate*>( program_element.get() ) )
		{
			PrepareTypeTemplate( *typedef_template, names_scope );
		}
		else
			U_ASSERT(false);
	} // for program elements
}

CodeBuilder::PrepareFunctionResult CodeBuilder::PrepareFunction(
	const Synt::Function& func,
	const bool is_class_method_predeclaration,
	ClassProxyPtr base_class,
	NamesScope& func_definition_names_scope /* scope, where this function appears */ )
{
	PrepareFunctionResult result;

	const ProgramString& func_name= func.name_.components.back().name;

	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	const bool is_special_method= is_constructor || is_destructor;

	if( !is_special_method && IsKeyword( func_name ) )
		errors_.push_back( ReportUsingKeywordAsName( func.file_pos_ ) );

	const Synt::Block* const block= is_class_method_predeclaration ? nullptr : func.block_.get();

	// Base scope (class, namespace), where function is declared.
	// Arguments, return value, body names all resolved from this scope.
	NamesScope* func_base_names_scope= &func_definition_names_scope;

	if( func.name_.components.size() >= 2u )
	{
		// Complex name - search scope for this function.
		if( const NamesScope::InsertedName* const scope_name=
			ResolveName( func.file_pos_, func_definition_names_scope, func.name_.components.data(), func.name_.components.size() - 1u ) )
		{
			bool base_space_is_class= false;
			if( const Type* const type= scope_name->second.GetTypeName() )
			{
				if( const ClassProxyPtr class_= type->GetClassTypeProxy() )
				{
					func_base_names_scope= &class_->class_->members;
					base_class= class_; // TODO - check here if base_class nonnull and diffrs from class_?
					base_space_is_class= true;
				}
			}

			if( base_space_is_class ) {}
			else if( const NamesScopePtr namespace_= scope_name->second.GetNamespace() )
			{
				func_base_names_scope= namespace_.get();
			}
			else
			{
				errors_.push_back( ReportNameNotFound( func.file_pos_, func.name_ ) );
				return result;
			}
		}
		else
		{
			errors_.push_back( ReportFunctionDeclarationOutsideItsScope( func.file_pos_ ) );
			return result;
		}
	}

	if( is_special_method && base_class == nullptr )
	{
		errors_.push_back( ReportConstructorOrDestructorOutsideClass( func.file_pos_ ) );
		return result;
	}
	if( !is_constructor && func.constructor_initialization_list_ != nullptr )
	{
		errors_.push_back( ReportInitializationListInNonconstructor(  func.constructor_initialization_list_->file_pos_ ) );
		return result;
	}
	if( is_destructor && !func.arguments_.empty() )
	{
		errors_.push_back( ReportExplicitArgumentsInDestructor( func.file_pos_ ) );
		return result;
	}

	FunctionVariable func_variable;
	func_variable.type= Function();
	Function& function_type= *func_variable.type.GetFunctionType();

	if( func.return_type_.name.components.empty() )
		function_type.return_type= void_type_;
	else
	{
		function_type.return_type= PrepareType( func.file_pos_, func.return_type_, *func_base_names_scope );
		if( function_type.return_type == invalid_type_ )
			return result;
	}

	// SPRACHE_TODO - make variables without explicit mutability modifiers immutable.
	if( func.return_value_mutability_modifier_ == MutabilityModifier::Immutable )
		function_type.return_value_is_mutable= false;
	else
		function_type.return_value_is_mutable= true;
	function_type.return_value_is_reference= func.return_value_reference_modifier_ == ReferenceModifier::Reference;

	if( function_type.return_type.GetTemplateDependentType() == nullptr &&
		!function_type.return_value_is_reference &&
		!( function_type.return_type.GetFundamentalType() != nullptr ||
		   function_type.return_type.GetClassType() != nullptr ) )
	{
		errors_.push_back( ReportNotImplemented( func.file_pos_, "return value types except fundamental and classes" ) );
		return result;
	}

	if( is_special_method && function_type.return_type != void_type_ )
		errors_.push_back( ReportConstructorAndDestructorMustReturnVoid( func.file_pos_ ) );

	// Args.
	function_type.args.reserve( func.arguments_.size() );

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

	for( const Synt::FunctionArgumentPtr& arg : func.arguments_ )
	{
		const bool is_this= arg == func.arguments_.front() && arg->name_ == Keywords::this_;

		if( !is_this && IsKeyword( arg->name_ ) )
			errors_.push_back( ReportUsingKeywordAsName( arg->file_pos_ ) );

		if( is_this && is_special_method )
			errors_.push_back( ReportExplicitThisInConstructorOrDestructor( arg->file_pos_ ) );

		function_type.args.emplace_back();
		Function::Arg& out_arg= function_type.args.back();

		if( is_this )
		{
			func_variable.is_this_call= true;
			if( base_class == nullptr )
			{
				errors_.push_back( ReportThisInNonclassFunction( func.file_pos_, func_name ) );
				return result;
			}
			out_arg.type= base_class;
		}
		else
			out_arg.type= PrepareType( arg->file_pos_, arg->type_, *func_base_names_scope );

		// TODO - make variables without explicit mutability modifiers immutable.
		if( arg->mutability_modifier_ == MutabilityModifier::Immutable )
			out_arg.is_mutable= false;
		else
			out_arg.is_mutable= true;
		out_arg.is_reference= is_this || arg->reference_modifier_ == ReferenceModifier::Reference;

		if( !out_arg.is_reference &&
			!( out_arg.type.GetFundamentalType() != nullptr ||
			   out_arg.type.GetClassType() != nullptr ||
			   out_arg.type.GetTemplateDependentType() != nullptr ) )
		{
			errors_.push_back( ReportNotImplemented( func.file_pos_, "parameters types except fundamental and classes" ) );
			return result;
		}

		// If this is not prototype, value-args must have complete type.
		if( block != nullptr && out_arg.type.IsIncomplete() && !out_arg.is_reference )
			errors_.push_back( ReportUsingIncompleteType( arg->file_pos_, out_arg.type.ToString() ) );
	} // for arguments

	NamesScope::InsertedName* const previously_inserted_func=
		func_base_names_scope->GetThisScopeName( func_name );
	if( previously_inserted_func == nullptr )
	{
		if( func.name_.components.size() > 1u )
		{
			errors_.push_back( ReportFunctionDeclarationOutsideItsScope( func.file_pos_ ) );
			return result;
		}

		func_variable.prototype_file_pos= func.file_pos_;
		if( block != nullptr )
			func_variable.body_file_pos= func.file_pos_;

		OverloadedFunctionsSet functions_set;
		functions_set.push_back( std::move( func_variable ) );

		if( NameShadowsTemplateArgument( func_name, *func_base_names_scope ) )
		{
			errors_.push_back( ReportDeclarationShadowsTemplateArgument( func.file_pos_, func_name ) );
			return result;
		}

		// New name in this scope - insert it.
		NamesScope::InsertedName* const inserted_func=
			func_base_names_scope->AddName( func_name, std::move( functions_set ) );
		U_ASSERT( inserted_func != nullptr );

		BuildFuncCode(
			inserted_func->second.GetFunctionsSet()->front(),
			base_class,
			*func_base_names_scope,
			func_name,
			func.arguments_,
			block,
			func.constructor_initialization_list_.get() );

		result.func_syntax_element= &func;
		result.functions_set= inserted_func->second.GetFunctionsSet();
		result.function_index= result.functions_set->size() - 1u;
		return result;
	}
	else
	{
		Value& value= previously_inserted_func->second;
		if( OverloadedFunctionsSet* const functions_set= value.GetFunctionsSet() )
		{
			if( FunctionVariable* const same_function=
				GetFunctionWithExactSignature(
					*func_variable.type.GetFunctionType(),
					*functions_set ) )
			{
				if( func.block_ == nullptr )
				{
					errors_.push_back( ReportFunctionPrototypeDuplication( func.file_pos_, func_name ) );
					return result;
				}
				if( same_function->have_body )
				{
					errors_.push_back( ReportFunctionBodyDuplication( func.file_pos_, func_name ) );
					return result;
				}
				if( same_function->type != func_variable.type )
				{
					// In this place we have only possible error
					errors_.push_back( ReportReturnValueDiffersFromPrototype( func.file_pos_ ) );
					return result;
				}

				same_function->body_file_pos= func.file_pos_;

				BuildFuncCode(
					*same_function,
					base_class,
					*func_base_names_scope,
					func_name,
					func.arguments_,
					block,
					func.constructor_initialization_list_.get() );
			}
			else
			{
				if( func.name_.components.size() > 1u )
				{
					errors_.push_back( ReportFunctionDeclarationOutsideItsScope( func.file_pos_ ) );
					return result;
				}

				const bool overloading_ok=
					ApplyOverloadedFunction( *functions_set, func_variable, func.file_pos_ );
				if( !overloading_ok )
					return result;

				FunctionVariable& inserted_func_variable= functions_set->back();
				inserted_func_variable.prototype_file_pos= func.file_pos_;
				if( block != nullptr )
					inserted_func_variable.body_file_pos= func.file_pos_;

				BuildFuncCode(
					inserted_func_variable,
					base_class,
					*func_base_names_scope,
					func_name,
					func.arguments_,
					block,
					func.constructor_initialization_list_.get() );
			}

			result.func_syntax_element= &func;
			result.functions_set= functions_set;
			result.function_index= result.functions_set->size() - 1u;
			return result;
		}
		else
			errors_.push_back( ReportRedefinition( func.file_pos_, previously_inserted_func->first ) );
	}

	return result;
}

void CodeBuilder::BuildFuncCode(
	FunctionVariable& func_variable,
	const ClassProxyPtr base_class,
	NamesScope& parent_names_scope,
	const ProgramString& func_name,
	const Synt::FunctionArgumentsDeclaration& args,
	const Synt::Block* const block,
	const Synt::StructNamedInitializer* const constructor_initialization_list )
{
	std::vector<llvm::Type*> args_llvm_types;
	Function* const function_type= func_variable.type.GetFunctionType();

	bool first_arg_is_sret= false;
	if( !function_type->return_value_is_reference )
	{
		if( function_type->return_type.GetTemplateDependentType() != nullptr )
		{}
		else if( function_type->return_type.GetFundamentalType() != nullptr )
		{}
		else if( const Class* const class_type= function_type->return_type.GetClassType() )
		{
			// Add return-value ponter as "sret" argument for class types.
			args_llvm_types.push_back( llvm::PointerType::get( class_type->llvm_type, 0u ) );
			first_arg_is_sret= true;
			func_variable.return_value_is_sret= true;
		}
		else
		{ U_ASSERT( false ); }
	}

	for( const Function::Arg& arg : function_type->args )
	{
		llvm::Type* type= arg.type.GetLLVMType();
		if( arg.is_reference )
			type= llvm::PointerType::get( type, 0u );
		else
		{
			if( arg.type.GetTemplateDependentType() != nullptr )
				type= fundamental_llvm_types_.invalid_type_;
			else if( arg.type.GetFundamentalType() != nullptr )
			{}
			else if( arg.type.GetClassType() != nullptr )
			{
				// Mark value-parameters of class types as pointer. Lately this parameters will be marked as "byval".
				type= llvm::PointerType::get( type, 0u );
			}
			else
			{ U_ASSERT( false ); }
		}
		args_llvm_types.push_back( type );
	}

	llvm::Type* llvm_function_return_type;
	if( function_type->return_type.GetTemplateDependentType() != nullptr )
		llvm_function_return_type= fundamental_llvm_types_.void_;
	else if( first_arg_is_sret )
		llvm_function_return_type= fundamental_llvm_types_.void_;
	else
	{
		llvm_function_return_type= function_type->return_type.GetLLVMType();
		if( function_type->return_value_is_reference )
			llvm_function_return_type= llvm::PointerType::get( llvm_function_return_type, 0u );
	}

	function_type->llvm_function_type=
		llvm::FunctionType::get(
			llvm_function_return_type,
			llvm::ArrayRef<llvm::Type*>( args_llvm_types.data(), args_llvm_types.size() ),
			false );

	llvm::Function* llvm_function;
	if( func_variable.llvm_function == nullptr )
	{
		llvm_function=
			llvm::Function::Create(
				function_type->llvm_function_type,
				llvm::Function::LinkageTypes::ExternalLinkage, // TODO - select linkage
				MangleFunction( parent_names_scope, func_name, *function_type, func_variable.is_this_call ),
				module_.get() );

		// Merge functions with identical code.
		// We doesn`t need different addresses for different functions.
		llvm_function->setUnnamedAddr( true );

		// Mark reference-parameters as nonnull.
		// Mark fake-pointer parameters of struct type as "byvall".
		// TODO - maybe mark immutable reference-parameters as "noalias"?
		for( size_t i= 0u; i < function_type->args.size(); i++ )
		{
			const size_t arg_attr_index= i + 1u + (first_arg_is_sret ? 1u : 0u );
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

	func_variable.have_body= true;

	NamesScope function_names( ""_SpC, &parent_names_scope );
	FunctionContext function_context(
		function_type->return_type,
		function_type->return_value_is_mutable,
		function_type->return_value_is_reference,
		llvm_context_,
		llvm_function );
	const StackVariablesStorage args_storage( function_context );

	// push args
	Variable this_;
	Variable s_ret;
	unsigned int arg_number= 0u;

	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	const bool is_special_method= is_constructor || is_destructor;

	for( llvm::Argument& llvm_arg : llvm_function->args() )
	{
		// Skip "sret".
		if( first_arg_is_sret && &llvm_arg == &*llvm_function->arg_begin() )
		{
			s_ret.location= Variable::Location::Pointer;
			s_ret.value_type= ValueType::Reference;
			s_ret.type= function_type->return_type;
			s_ret.llvm_value= &llvm_arg;
			llvm_arg.setName( "_return_value" );
			function_context.s_ret_= &s_ret;
			continue;
		}

		const Function::Arg& arg= function_type->args[ arg_number ];

		if( is_special_method && arg_number == 0u )
		{
			this_.location= Variable::Location::Pointer;
			this_.value_type= ValueType::Reference;
			this_.type= arg.type;
			this_.llvm_value= &llvm_arg;
			llvm_arg.setName( KeywordAscii( Keywords::this_ ) );
			function_context.this_= &this_;

			const StoredVariablePtr this_storage= std::make_shared<StoredVariable>( this_ );
			this_.referenced_variables.emplace(this_storage);

			arg_number++;
			continue;
		}

		const Synt::FunctionArgument& declaration_arg= *args[ is_special_method ? ( arg_number - 1u ) : arg_number ];
		const ProgramString& arg_name= declaration_arg.name_;

		if( !arg.is_reference && arg.type.IsIncomplete() )
		{
			errors_.push_back( ReportUsingIncompleteType( declaration_arg.file_pos_, arg.type.ToString() ) );
			continue;
		}

		const bool is_this= arg_number == 0u && arg_name == Keywords::this_;
		U_ASSERT( !( is_this && !arg.is_reference ) );
		U_ASSERT( !( is_special_method && is_this ) );

		Variable var;
		var.location= Variable::Location::LLVMRegister;
		var.value_type= ValueType::Reference;
		var.type= arg.type;
		var.llvm_value= &llvm_arg;

		// TODO - make variables without explicit mutability modifiers immutable.
		if( declaration_arg.mutability_modifier_ == MutabilityModifier::Immutable )
			var.value_type= ValueType::ConstReference;

		if( arg.is_reference )
			var.location= Variable::Location::Pointer;
		else
		{
			if( arg.type.GetFundamentalType() != nullptr ||
				arg.type.GetTemplateDependentType() != nullptr )
			{
				// Move parameters to stack for assignment possibility.
				// TODO - do it, only if parameters are not constant.
				llvm::Value* address= function_context.alloca_ir_builder.CreateAlloca( var.type.GetLLVMType() );
				address->setName( ToStdString( arg_name ) );
				function_context.llvm_ir_builder.CreateStore( var.llvm_value, address );

				var.llvm_value= address;
				var.location= Variable::Location::Pointer;
			}
			else if( arg.type.GetClassType() != nullptr )
			{
				// Value classes parameters using llvm-pointers with "byval" attribute.
				var.location= Variable::Location::Pointer;
			}
			else
			{ U_ASSERT( false ); }
		}

		const StoredVariablePtr var_storage= std::make_shared<StoredVariable>( var, arg.is_reference );

		if( !var_storage->is_reference )
			var.referenced_variables.emplace(var_storage);

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

			function_context.stack_variables_stack.back()->RegisterVariable( var_storage );

			const NamesScope::InsertedName* const inserted_arg=
				function_names.AddName( arg_name, Value( var_storage, declaration_arg.file_pos_ ) );
			if( !inserted_arg )
			{
				errors_.push_back( ReportRedefinition( declaration_arg.file_pos_, arg_name ) );
				return;
			}
		}

		llvm_arg.setName( "_arg_" + ToStdString( arg_name ) );
		++arg_number;
	}

	if( is_constructor )
	{
		U_ASSERT( base_class != nullptr );
		U_ASSERT( function_context.this_ != nullptr );

		function_context.is_constructor_initializer_list_now= true;

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

		function_context.is_constructor_initializer_list_now= false;
	}

	if( is_destructor )
		function_context.destructor_end_block= llvm::BasicBlock::Create( llvm_context_ );

	const BlockBuildInfo block_build_info=
		BuildBlockCode( *block, function_names, function_context );

	U_ASSERT( function_context.stack_variables_stack.size() == 1u );

	// We need call destructors for arguments only if function returns "void".
	// In other case, we have "return" in all branches and destructors call before each "return".

	if(  function_type->return_type == void_type_ )
	{
		// Manually generate "return" for void-return functions.
		if( !block_build_info.have_unconditional_return_inside )
		{
			CallDestructors( *function_context.stack_variables_stack.back(), function_context, block->end_file_pos_ );

			if( function_context.destructor_end_block == nullptr )
				function_context.llvm_ir_builder.CreateRetVoid();
			else
			{
				// In explicit destructor, break to block with destructor calls for class members.
				function_context.llvm_ir_builder.CreateBr( function_context.destructor_end_block );
			}
		}
	}
	else
	{
		if( !block_build_info.have_unconditional_return_inside )
		{
			errors_.push_back( ReportNoReturnInFunctionReturningNonVoid( block->file_pos_ ) );
			return;
		}
	}

	llvm::Function::BasicBlockListType& bb_list = llvm_function->getBasicBlockList();

	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );

	if( is_destructor )
	{
		// Fill destructors block.
		U_ASSERT( function_context.destructor_end_block != nullptr );
		function_context.llvm_ir_builder.SetInsertPoint( function_context.destructor_end_block );
		bb_list.push_back( function_context.destructor_end_block );

		CallMembersDestructors( function_context );
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
	for( const Synt::StructNamedInitializer::MemberInitializer& field_initializer : constructor_initialization_list.members_initializers )
	{
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

			if( initialized_fields.find( member.first ) == initialized_fields.end() )
				uninitialized_fields.insert( member.first );
		} );

	// Initialize fields, missing in initializer list.
	for( const ProgramString& field_name : uninitialized_fields )
	{
		const NamesScope::InsertedName* const class_member=
			base_class.members.GetThisScopeName( field_name );
		U_ASSERT( class_member != nullptr );
		const ClassField* const field= class_member->second.GetClassField();
		U_ASSERT( field != nullptr );

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

	if( have_fields_errors )
		return;

	for( const Synt::StructNamedInitializer::MemberInitializer& field_initializer : constructor_initialization_list.members_initializers )
	{
		const NamesScope::InsertedName* const class_member=
			base_class.members.GetThisScopeName( field_initializer.name );
		U_ASSERT( class_member != nullptr );
		const ClassField* const field= class_member->second.GetClassField();
		U_ASSERT( field != nullptr );

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
		ApplyInitializer( field_variable, *field_initializer.initializer, names_scope, function_context );

		function_context.uninitialized_this_fields.erase( field );
	} // for fields initializers
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
			const unsigned int block_element_index= &block_element - block.elements_.data();
			if( block_element_index + 1u < block.elements_.size() )
				errors_.push_back( ReportUnreachableCode( block.elements_[ block_element_index + 1u ]->GetFilePos() ) );
		};

		if( const auto variables_declaration=
			dynamic_cast<const Synt::VariablesDeclaration*>( block_element_ptr ) )
		{
			BuildVariablesDeclarationCode( *variables_declaration, block_names, function_context );
		}
		else if( const auto auto_variable_declaration=
			dynamic_cast<const Synt::AutoVariableDeclaration*>( block_element_ptr ) )
		{
			BuildAutoVariableDeclarationCode( *auto_variable_declaration, block_names, function_context );
		}
		else if( const auto expression=
			dynamic_cast<const Synt::SingleExpressionOperator*>( block_element_ptr ) )
		{
			BuildExpressionCodeAndDestroyTemporaries(
				*expression->expression_,
				block_names,
				function_context );
		}
		else if( const auto assignment_operator=
			dynamic_cast<const Synt::AssignmentOperator*>( block_element_ptr ) )
		{
			BuildAssignmentOperatorCode( *assignment_operator, block_names, function_context );
		}
		else if( const auto additive_assignment_operator=
			dynamic_cast<const Synt::AdditiveAssignmentOperator*>( block_element_ptr ) )
		{
			BuildAdditiveAssignmentOperatorCode( *additive_assignment_operator, block_names, function_context );
		}
		else if( const auto increment_operator=
			dynamic_cast<const Synt::IncrementOperator*>( block_element_ptr ) )
		{
			BuildDeltaOneOperatorCode(
				*increment_operator->expression,
				increment_operator->file_pos_,
				true,
				block_names,
				function_context );
		}
		else if( const auto decrement_operator=
			dynamic_cast<const Synt::DecrementOperator*>( block_element_ptr ) )
		{
			BuildDeltaOneOperatorCode(
				*decrement_operator->expression,
				decrement_operator->file_pos_,
				false,
				block_names,
				function_context );
		}
		else if( const auto return_operator=
			dynamic_cast<const Synt::ReturnOperator*>( block_element_ptr ) )
		{
			BuildReturnOperatorCode(
				*return_operator,
				block_names,
				function_context );

			block_build_info.have_unconditional_return_inside= true;
			try_report_unreachable_code();
		}
		else if( const auto while_operator=
			dynamic_cast<const Synt::WhileOperator*>( block_element_ptr ) )
		{
			BuildWhileOperatorCode(
				*while_operator,
				block_names,
				function_context );
		}
		else if( const auto break_operator=
			dynamic_cast<const Synt::BreakOperator*>( block_element_ptr ) )
		{
			BuildBreakOperatorCode(
				*break_operator,
				function_context );

			block_build_info.have_uncodnitional_break_or_continue= true;
			try_report_unreachable_code();
		}
		else if( const auto continue_operator=
			dynamic_cast<const Synt::ContinueOperator*>( block_element_ptr ) )
		{
			BuildContinueOperatorCode(
				*continue_operator,
				function_context );

			block_build_info.have_uncodnitional_break_or_continue= true;
			try_report_unreachable_code();
		}
		else if(
			const auto if_operator=
			dynamic_cast<const Synt::IfOperator*>( block_element_ptr ) )
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
		else if( const auto static_assert_=
			dynamic_cast<const Synt::StaticAssert*>( block_element_ptr ) )
		{
			BuildStaticAssert( *static_assert_, block_names );
		}
		else if( const auto halt=
			dynamic_cast<const Synt::Halt*>( block_element_ptr ) )
		{
			BuildHalt( *halt, function_context );

			block_build_info.have_unconditional_return_inside= true;
			try_report_unreachable_code();
		}
		else if( const auto halt_if=
			dynamic_cast<const Synt::HaltIf*>( block_element_ptr ) )
		{
			BuildHaltIf( *halt_if, block_names, function_context );
		}
		else if( const auto block=
			dynamic_cast<const Synt::Block*>( block_element_ptr ) )
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
		else
			U_ASSERT(false);
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
	FunctionContext& function_context,
	const bool global )
{
	const Type type= PrepareType( variables_declaration.file_pos_, variables_declaration.type, block_names );
	if( type.IsIncomplete() )
	{
		errors_.push_back( ReportUsingIncompleteType( variables_declaration.file_pos_, type.ToString() ) );
		return;
	}

	for( const Synt::VariablesDeclaration::VariableEntry& variable_declaration : variables_declaration.variables )
	{
		if( IsKeyword( variable_declaration.name ) )
		{
			errors_.push_back( ReportUsingKeywordAsName( variables_declaration.file_pos_ ) );
			continue;
		}

		if( variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
			type.GetTemplateDependentType() == nullptr &&
			!type.CanBeConstexpr() )
		{
			errors_.push_back( ReportInvalidTypeForConstantExpressionVariable( variables_declaration.file_pos_ ) );
			continue;
		}

		// Destruction frame for temporary variables of initializer expression.
		const StackVariablesStorage temp_variables_storage( function_context );

		Variable variable;
		variable.type= type;
		variable.location= Variable::Location::Pointer;
		variable.value_type= ValueType::Reference;

		if( type.GetTemplateDependentType() != nullptr )
		{
			if( variable_declaration.initializer != nullptr )
				ApplyInitializer( variable, *variable_declaration.initializer, block_names, function_context );
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::None )
		{
			llvm::GlobalVariable* global_variable= nullptr;
			if( global && type.GetTemplateDependentType() == nullptr )
			{
				variable.llvm_value=
				global_variable=
					new llvm::GlobalVariable(
						*module_,
						type.GetLLVMType(),
						true,
						llvm::GlobalValue::LinkageTypes::LinkOnceODRLinkage,
						nullptr,
						MangleGlobalVariable( block_names, variable_declaration.name ) );
			}
			else
			{
				variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType() );
				variable.llvm_value->setName( ToStdString( variable_declaration.name ) );
			}

			if( variable_declaration.initializer != nullptr )
				variable.constexpr_value=
					ApplyInitializer( variable, *variable_declaration.initializer, block_names, function_context );
			else
				ApplyEmptyInitializer( variable_declaration.name, variables_declaration.file_pos_, variable, function_context );

			// Make immutable, if needed, only after initialization, because in initialization we need call constructors, which is mutable methods.
			// SPRACHE_TODO - make variables without explicit mutability modifiers immutable.
			if( variable_declaration.mutability_modifier == MutabilityModifier::Immutable ||
				variable_declaration.mutability_modifier == MutabilityModifier::Constexpr )
				variable.value_type= ValueType::ConstReference;

			if( global_variable != nullptr && variable.constexpr_value != nullptr )
				global_variable->setInitializer( variable.constexpr_value );
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			// Mark references immutable before initialization.
			// SPRACHE_TODO - make variables without explicit mutability modifiers immutable.
			if( variable_declaration.mutability_modifier == MutabilityModifier::Immutable ||
				variable_declaration.mutability_modifier == MutabilityModifier::Constexpr )
				variable.value_type= ValueType::ConstReference;

			if( variable_declaration.initializer == nullptr )
			{
				errors_.push_back( ReportExpectedInitializer( variables_declaration.file_pos_, variable_declaration.name ) );
				continue;
			}

			const Synt::IExpressionComponent* initializer_expression= nullptr;
			if( const auto expression_initializer=
				dynamic_cast<const Synt::ExpressionInitializer*>( variable_declaration.initializer.get() ) )
			{
				initializer_expression= expression_initializer->expression.get();
			}
			else if( const auto constructor_initializer=
				dynamic_cast<const Synt::ConstructorInitializer*>( variable_declaration.initializer.get() ) )
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

			const Value expression_result_value=
				BuildExpressionCode( *initializer_expression, block_names, function_context );

			if( expression_result_value.GetType() != variable.type &&
				expression_result_value.GetType().GetTemplateDependentType() == nullptr && variable.type.GetTemplateDependentType() == nullptr )
			{
				errors_.push_back( ReportTypesMismatch( variables_declaration.file_pos_, variable.type.ToString(), expression_result_value.GetType().ToString() ) );
				continue;
			}
			const Variable& expression_result= *expression_result_value.GetVariable();

			if( expression_result.value_type == ValueType::Value )
			{
				errors_.push_back( ReportExpectedReferenceValue( variables_declaration.file_pos_ ) );
				continue;
			}
			if( expression_result.value_type == ValueType::ConstReference &&
				variable.value_type == ValueType::Reference )
			{
				errors_.push_back( ReportBindingConstReferenceToNonconstReference( variables_declaration.file_pos_ ) );
				continue;
			}

			variable.referenced_variables= expression_result.referenced_variables;

			// TODO - maybe make copy of varaible address in new llvm register?
			variable.llvm_value= expression_result.llvm_value;
			variable.constexpr_value= expression_result.constexpr_value;
		}
		else
		{
			U_ASSERT(false);
		}

		if( type.GetTemplateDependentType() == nullptr &&
			variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
			variable.constexpr_value == nullptr )
		{
			errors_.push_back( ReportVariableInitializerIsNotConstantExpression( variables_declaration.file_pos_ ) );
			continue;
		}

		// Reset constexpr initial value for mutable variables.
		if( variable.value_type != ValueType::ConstReference )
			variable.constexpr_value= nullptr;

		if( variable.type.GetTemplateDependentType() == nullptr &&
			global && variable.constexpr_value == nullptr )
		{
			errors_.push_back( ReportGlobalVariableMustBeConstexpr( variables_declaration.file_pos_, variable_declaration.name ) );
			continue;
		}

		if( NameShadowsTemplateArgument( variable_declaration.name, block_names ) )
		{
			errors_.push_back( ReportDeclarationShadowsTemplateArgument( variables_declaration.file_pos_, variable_declaration.name ) );
			return;
		}

		const StoredVariablePtr stored_variable=
			std::make_shared<StoredVariable>(
				variable,
				variable_declaration.reference_modifier == ReferenceModifier::Reference );

		if( stored_variable->is_reference )
		{
			for( const StoredVariablePtr& referenced_variable : variable.referenced_variables )
			{
				stored_variable->locked_referenced_variables.push_back(
					variable.value_type == ValueType::ConstReference ? referenced_variable->imut_use_counter : referenced_variable->mut_use_counter );
			}
			CheckReferencedVariables( variable, variable_declaration.file_pos );
		}

		const NamesScope::InsertedName* const inserted_name=
			block_names.AddName( variable_declaration.name, Value( std::move(stored_variable), variable_declaration.file_pos ) );
		if( !inserted_name )
		{
			errors_.push_back( ReportRedefinition( variables_declaration.file_pos_, variable_declaration.name ) );
			continue;
		}

		// After lock of references we can call destructors.
		CallDestructors( *function_context.stack_variables_stack.back(), function_context, variable_declaration.file_pos );

		function_context.stack_variables_stack[ function_context.stack_variables_stack.size() - 2u ]->RegisterVariable( stored_variable );
	} // for variables
}

void CodeBuilder::BuildAutoVariableDeclarationCode(
	const Synt::AutoVariableDeclaration& auto_variable_declaration,
	NamesScope& block_names,
	FunctionContext& function_context,
	const bool global )
{
	// Destruction frame for temporary variables of initializer expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	const Value initializer_experrsion_value=
		BuildExpressionCode( *auto_variable_declaration.initializer_expression, block_names, function_context );

	if( initializer_experrsion_value.GetType() == NontypeStub::TemplateDependentValue )
	{
		// Stub, if initializer expression is something strange.

		Variable variable;
		// SPRACHE_TODO - make variables without explicit mutability modifiers immutable.
		if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Immutable||
			auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr )
			variable.value_type= ValueType::ConstReference;
		else
			variable.value_type= ValueType::Reference;
		variable.type= GetNextTemplateDependentType();

		if( NameShadowsTemplateArgument( auto_variable_declaration.name, block_names ) )
		{
			errors_.push_back( ReportDeclarationShadowsTemplateArgument( auto_variable_declaration.file_pos_, auto_variable_declaration.name ) );
			return;
		}
		const NamesScope::InsertedName* inserted_name= block_names.AddName( auto_variable_declaration.name, Value( variable, auto_variable_declaration.file_pos_ ) );
		if( inserted_name == nullptr )
		{
			errors_.push_back( ReportRedefinition( auto_variable_declaration.file_pos_, auto_variable_declaration.name ) );
			return;
		}
		return;
	}

	{ // Check expression type. Expression can have exotic types, such "Overloading functions set", "class name", etc.
		const Type& type= initializer_experrsion_value.GetType();
		const bool type_is_ok=
			type.GetFundamentalType() != nullptr ||
			type.GetArrayType() != nullptr ||
			type.GetClassType() != nullptr ||
			type.GetTemplateDependentType() != nullptr;
		if( !type_is_ok )
		{
			errors_.push_back( ReportInvalidTypeForAutoVariable( auto_variable_declaration.file_pos_, initializer_experrsion_value.GetType().ToString() ) );
			return;
		}
	}

	const Variable& initializer_experrsion= *initializer_experrsion_value.GetVariable();

	Variable variable;
	variable.location= Variable::Location::Pointer;

	// SPRACHE_TODO - make variables without explicit mutability modifiers immutable.
	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Immutable ||
		auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr )
		variable.value_type= ValueType::ConstReference;
	else
		variable.value_type= ValueType::Reference;

	variable.type= initializer_experrsion.type;

	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
		!variable.type.CanBeConstexpr() )
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
		if( initializer_experrsion.value_type == ValueType::ConstReference &&
			variable.value_type != ValueType::ConstReference )
		{
			errors_.push_back( ReportBindingConstReferenceToNonconstReference( auto_variable_declaration.file_pos_ ) );
			return;
		}

		variable.referenced_variables= initializer_experrsion.referenced_variables;

		variable.llvm_value= initializer_experrsion.llvm_value;
		variable.constexpr_value= initializer_experrsion.constexpr_value;
	}
	else if( auto_variable_declaration.reference_modifier == ReferenceModifier::None )
	{
		llvm::GlobalVariable* global_variable= nullptr;
		if( global && variable.type.GetTemplateDependentType() == nullptr )
		{
			variable.llvm_value=
			global_variable=
				new llvm::GlobalVariable(
					*module_,
					variable.type.GetLLVMType(),
					true,
					llvm::GlobalValue::LinkageTypes::LinkOnceODRLinkage,
					nullptr,
					MangleGlobalVariable( block_names, auto_variable_declaration.name ) );
		}
		else
		{
			variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType() );
			variable.llvm_value->setName( ToStdString( auto_variable_declaration.name ) );
		}

		if( const FundamentalType* const fundamental_type= variable.type.GetFundamentalType() )
		{
			U_UNUSED(fundamental_type);
			llvm::Value* const value_for_assignment= CreateMoveToLLVMRegisterInstruction( initializer_experrsion, function_context );
			function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );

			variable.constexpr_value= initializer_experrsion.constexpr_value;
		}
		else if( variable.type.GetTemplateDependentType() != nullptr )
		{}
		else
		{
			errors_.push_back( ReportNotImplemented( auto_variable_declaration.file_pos_, "expression initialization for nonfundamental types" ) );
			return;
		}

		if( global_variable != nullptr && variable.constexpr_value != nullptr )
			global_variable->setInitializer( variable.constexpr_value );
	}
	else
		U_ASSERT(false);

	if( variable.type.GetTemplateDependentType() == nullptr &&
		auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
		variable.constexpr_value == nullptr )
	{
		errors_.push_back( ReportVariableInitializerIsNotConstantExpression( auto_variable_declaration.file_pos_ ) );
		return;
	}

	// Reset constexpr initial value for mutable variables.
	if( variable.value_type != ValueType::ConstReference )
		variable.constexpr_value= nullptr;

	if( variable.type.GetTemplateDependentType() == nullptr &&
		global && variable.constexpr_value == nullptr )
	{
		errors_.push_back( ReportGlobalVariableMustBeConstexpr( auto_variable_declaration.file_pos_, auto_variable_declaration.name ) );
		return;
	}

	if( NameShadowsTemplateArgument( auto_variable_declaration.name, block_names ) )
	{
		errors_.push_back( ReportDeclarationShadowsTemplateArgument( auto_variable_declaration.file_pos_, auto_variable_declaration.name ) );
		return;
	}

	const StoredVariablePtr stored_variable=
		std::make_shared<StoredVariable>(
			variable,
			auto_variable_declaration.reference_modifier == ReferenceModifier::Reference );

	if( stored_variable->is_reference )
	{
		for( const StoredVariablePtr& referenced_variable : variable.referenced_variables )
		{
			stored_variable->locked_referenced_variables.push_back(
				variable.value_type == ValueType::ConstReference ? referenced_variable->imut_use_counter : referenced_variable->mut_use_counter );
		}
		CheckReferencedVariables( variable, auto_variable_declaration.file_pos_ );
	}

	const NamesScope::InsertedName* inserted_name=
		block_names.AddName( auto_variable_declaration.name, Value( stored_variable, auto_variable_declaration.file_pos_ ) );

	if( inserted_name == nullptr )
	{
		errors_.push_back( ReportRedefinition( auto_variable_declaration.file_pos_, auto_variable_declaration.name ) );
		return;
	}

	// After lock of references we can call destructors.
	CallDestructors( *function_context.stack_variables_stack.back(), function_context, auto_variable_declaration.file_pos_ );

	function_context.stack_variables_stack[ function_context.stack_variables_stack.size() - 2u ]->RegisterVariable( stored_variable );
}

void CodeBuilder::BuildAssignmentOperatorCode(
	const Synt::AssignmentOperator& assignment_operator,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expressions.
	const StackVariablesStorage temp_variables_storage( function_context );

	// Evalueate right part
	const Value r_var_value= BuildExpressionCode( *assignment_operator.r_value_, block_names, function_context );
	const Variable* const r_var= r_var_value.GetVariable();
	if( r_var == nullptr && r_var_value.GetType() != NontypeStub::TemplateDependentValue )
		errors_.push_back( ReportExpectedVariableInAssignment( assignment_operator.file_pos_, r_var_value.GetType().ToString() ) );

	// Lock r_var variables.
	std::vector<VariableStorageUseCounter> r_var_locks;
	if( r_var != nullptr )
	{
		for( const StoredVariablePtr& referenced_variable : r_var->referenced_variables )
			r_var_locks.push_back( r_var->value_type == ValueType::Reference ? referenced_variable->mut_use_counter : referenced_variable->imut_use_counter );
	}

	// Evaluate left part.
	const Value l_var_value= BuildExpressionCode( *assignment_operator.l_value_, block_names, function_context );
	const Variable* const l_var= l_var_value.GetVariable();
	if( l_var == nullptr && l_var_value.GetType() != NontypeStub::TemplateDependentValue )
		errors_.push_back( ReportExpectedVariableInAssignment( assignment_operator.file_pos_, l_var_value.GetType().ToString() ) );

	if( l_var == nullptr || r_var == nullptr )
		return;

	if( l_var->value_type != ValueType::Reference )
	{
		errors_.push_back( ReportExpectedReferenceValue( assignment_operator.file_pos_ ) );
		return;
	}
	if( l_var->type.GetTemplateDependentType() == nullptr && r_var->type.GetTemplateDependentType() == nullptr && l_var->type != r_var->type )
	{
		errors_.push_back( ReportTypesMismatch( assignment_operator.file_pos_, l_var->type.ToString(), r_var->type.ToString() ) );
		return;
	}

	// Check references of destination.
	for( const StoredVariablePtr& referenced_variable : l_var->referenced_variables )
	{
		if( referenced_variable->imut_use_counter.use_count() > 1u )
		{
			// Assign to variable, that have nonzero immutable references.
			errors_.push_back( ReportReferenceProtectionError( assignment_operator.file_pos_ ) );
		}
		if( referenced_variable->mut_use_counter.use_count() > 1u + 1u )
		{
			// Variable have mutable references except locked reference for this assignment.
			errors_.push_back( ReportReferenceProtectionError( assignment_operator.file_pos_ ) );
		}
	}

	const FundamentalType* const fundamental_type= l_var->type.GetFundamentalType();
	if( fundamental_type != nullptr )
	{
		if( l_var->location != Variable::Location::Pointer )
		{
			U_ASSERT(false);
			return;
		}
		llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( *r_var, function_context );
		function_context.llvm_ir_builder.CreateStore( value_for_assignment, l_var->llvm_value );
	}
	else if( l_var->type.GetTemplateDependentType() != nullptr || r_var->type.GetTemplateDependentType() != nullptr )
	{}
	else
	{
		// TODO - functions is not copyable.
		// TODO - arrays not copyable.
		// TODO - make classes copyable.
		errors_.push_back( ReportNotImplemented( assignment_operator.file_pos_, "nonfundamental types assignment." ) );
		return;
	}

	r_var_locks.clear();

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

	const Value r_var_value=
		BuildExpressionCode(
			*additive_assignment_operator.r_value_,
			block_names,
			function_context );
	const Variable* const r_var= r_var_value.GetVariable();

	// Lock r_var variables.
	std::vector<VariableStorageUseCounter> r_var_locks;
	if( r_var != nullptr )
	{
		for( const StoredVariablePtr& referenced_variable : r_var->referenced_variables )
			r_var_locks.push_back( r_var->value_type == ValueType::Reference ? referenced_variable->mut_use_counter : referenced_variable->imut_use_counter );
	}

	const Value l_var_value=
		BuildExpressionCode(
			*additive_assignment_operator.l_value_,
			block_names,
			function_context );
	const Variable* const l_var= l_var_value.GetVariable();

	if( l_var == nullptr && l_var_value.GetType() != NontypeStub::TemplateDependentValue )
		errors_.push_back( ReportExpectedVariableInAdditiveAssignment( additive_assignment_operator.file_pos_, l_var_value.GetType().ToString() ) );
	if( r_var == nullptr && r_var_value.GetType() != NontypeStub::TemplateDependentValue )
		errors_.push_back( ReportExpectedVariableInAdditiveAssignment( additive_assignment_operator.file_pos_, r_var_value.GetType().ToString() ) );

	if( l_var == nullptr || r_var == nullptr )
		return;

	// Check references of destination.
	for( const StoredVariablePtr& stored_variable : l_var->referenced_variables )
	{
		if( stored_variable->imut_use_counter.use_count() > 1u )
		{
			// Assign to variable, that have nonzero immutable references.
			errors_.push_back( ReportReferenceProtectionError( additive_assignment_operator.file_pos_ ) );
		}
		if( stored_variable->mut_use_counter.use_count() > 1u + 1u )
		{
			// Variable have mutable references except locked reference for this assignment.
			errors_.push_back( ReportReferenceProtectionError( additive_assignment_operator.file_pos_ ) );
		}
	}

	const FundamentalType* const l_var_fundamental_type= l_var->type.GetFundamentalType();
	const FundamentalType* const r_var_fundamental_type= r_var->type.GetFundamentalType();
	if( l_var_fundamental_type != nullptr && r_var_fundamental_type != nullptr )
	{
		// Generate binary operator and assignment for fundamental types.
		// SPRACHE_TODO - do not call "BuildBinaryOperator", when operators overloading will be implemented.
		const Value operation_result_value=
			BuildBinaryOperator(
				*l_var, *r_var,
				additive_assignment_operator.additive_operation_,
				additive_assignment_operator.file_pos_,
				function_context );
		if( operation_result_value.GetVariable() == nullptr ) // Not variable in case of error or if template-dependent stuff.
			return;
		const Variable& operation_result= *operation_result_value.GetVariable();

		if( l_var->value_type != ValueType::Reference )
		{
			errors_.push_back( ReportExpectedReferenceValue( additive_assignment_operator.file_pos_ ) );
			return;
		}

		if( operation_result.type.GetTemplateDependentType() == nullptr && l_var->type.GetTemplateDependentType() == nullptr )
		{
			if( operation_result.type != l_var->type )
			{
				errors_.push_back( ReportTypesMismatch( additive_assignment_operator.file_pos_, l_var->type.ToString(), operation_result.type.ToString() ) );
				return;
			}

			U_ASSERT( l_var->location == Variable::Location::Pointer );
			llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( operation_result, function_context );
			function_context.llvm_ir_builder.CreateStore( value_in_register, l_var->llvm_value );
		}
	}
	else if( l_var->type.GetTemplateDependentType() != nullptr || r_var->type.GetTemplateDependentType() != nullptr )
	{}
	else
	{
		// SPRACHE_TODO - search for overloaded operators.
		errors_.push_back( ReportNotImplemented( additive_assignment_operator.file_pos_, "additive operations for nonfundamental types" ) );
		return;
	}

	r_var_locks.clear();

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
	const Value value= BuildExpressionCodeAndDestroyTemporaries( expression, block_names, function_context );
	if( value.GetType() == NontypeStub::TemplateDependentValue )
		return;

	const Variable* const variable= value.GetVariable();
	if( variable == nullptr )
	{
		errors_.push_back( ReportExpectedVariableInIncrementOrDecrement( file_pos, value.GetType().ToString() ) );
		return;
	}

	if( const FundamentalType* const fundamental_type= variable->type.GetFundamentalType() )
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
				errors_.push_back( ReportReferenceProtectionError( file_pos ) );
			// If "mut_counter" is not 0 or 1, error must be generated previosly.
		}

		llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( *variable, function_context );
		llvm::Value* const one=
			llvm::Constant::getIntegerValue(
				fundamental_type->llvm_type,
				llvm::APInt( variable->type.SizeOf() * 8u, uint64_t(1u) ) );

		llvm::Value* const new_value=
			positive
				? function_context.llvm_ir_builder.CreateAdd( value_in_register, one )
				: function_context.llvm_ir_builder.CreateSub( value_in_register, one );

		U_ASSERT( variable->location == Variable::Location::Pointer );
		function_context.llvm_ir_builder.CreateStore( new_value, variable->llvm_value );
	}
	else if( variable->type.GetTemplateDependentType() != nullptr )
	{}
	else
	{
		// SPRACHE_TODO - search for overloaded operators.
		errors_.push_back( ReportNotImplemented( file_pos, "++ and -- for nonfundamental types" ) );
		return;
	}
}

void CodeBuilder::BuildReturnOperatorCode(
	const Synt::ReturnOperator& return_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( return_operator.expression_ == nullptr )
	{
		if( function_context.return_type != void_type_ )
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

	const Value expression_result_value=
		BuildExpressionCode(
			*return_operator.expression_,
			names,
			function_context );

	if( expression_result_value.GetType() == NontypeStub::TemplateDependentValue ||
		function_context.return_type == NontypeStub::TemplateDependentValue )
	{
		// Add "ret void", because we do not need to break llvm basic blocks structure.
		function_context.llvm_ir_builder.CreateRetVoid();
		return;
	}

	if( expression_result_value.GetType().GetTemplateDependentType() == nullptr && function_context.return_type.GetTemplateDependentType() == nullptr &&
		expression_result_value.GetType() != function_context.return_type )
	{
		errors_.push_back( ReportTypesMismatch( return_operator.file_pos_, function_context.return_type.ToString(), expression_result_value.GetType().ToString() ) );
		return;
	}
	const Variable& expression_result= *expression_result_value.GetVariable();

	if( function_context.return_value_is_reference )
	{
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
		std::vector<VariableStorageUseCounter> return_value_locks;
		for( const StoredVariablePtr& var : expression_result.referenced_variables )
			return_value_locks.push_back( function_context.return_value_is_mutable ? var->mut_use_counter : var->imut_use_counter );

		CallDestructorsBeforeReturn( function_context, return_operator.file_pos_ );
		return_value_locks.clear(); // Reset locks AFTER destructors call. We must get error in case of returning of reference to stack variable or value-argument.

		function_context.llvm_ir_builder.CreateRet( expression_result.llvm_value );
	}
	else
	{
		if( function_context.s_ret_ != nullptr )
		{
			const ClassProxyPtr class_= function_context.s_ret_->type.GetClassTypeProxy();
			TryCallCopyConstructor( return_operator.file_pos_, function_context.s_ret_->llvm_value, expression_result.llvm_value, class_, function_context );

			CallDestructorsBeforeReturn( function_context, return_operator.file_pos_ );
			function_context.llvm_ir_builder.CreateRetVoid();
		}
		else if( expression_result.type.GetTemplateDependentType() != nullptr )
			function_context.llvm_ir_builder.CreateRetVoid();
		else
		{
			// Now we can return by value only fundamentals.
			U_ASSERT( expression_result.type.GetFundamentalType() != nullptr );

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
	llvm::BasicBlock* test_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* while_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* block_after_while= llvm::BasicBlock::Create( llvm_context_ );

	// Break to test block. We must push terminal instruction at and of current block.
	function_context.llvm_ir_builder.CreateBr( test_block );

	// Test block code.
	function_context.function->getBasicBlockList().push_back( test_block );
	function_context.llvm_ir_builder.SetInsertPoint( test_block );

	const Value condition_expression= BuildExpressionCodeAndDestroyTemporaries( *while_operator.condition_, names, function_context );

	if( condition_expression.GetType() != NontypeStub::TemplateDependentValue &&
		condition_expression.GetType().GetTemplateDependentType() == nullptr )
	{
		if( condition_expression.GetType() != bool_type_ )
		{
			errors_.push_back(
				ReportTypesMismatch(
					while_operator.condition_->GetFilePos(),
					bool_type_.ToString(),
					condition_expression.GetType().ToString() ) );
			return;
		}

		llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( *condition_expression.GetVariable(), function_context );
		function_context.llvm_ir_builder.CreateCondBr( condition_in_register, while_block, block_after_while );
	}
	else
		function_context.llvm_ir_builder.CreateCondBr( llvm::UndefValue::get( fundamental_llvm_types_.bool_ ), while_block, block_after_while );

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

	llvm::BasicBlock* block_after_if= llvm::BasicBlock::Create( llvm_context_ );

	llvm::BasicBlock* next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
	// Break to first condition. We must push terminal instruction at end of current block.
	function_context.llvm_ir_builder.CreateBr( next_condition_block );

	for( unsigned int i= 0u; i < if_operator.branches_.size(); i++ )
	{
		const Synt::IfOperator::Branch& branch= if_operator.branches_[i];

		llvm::BasicBlock* body_block= llvm::BasicBlock::Create( llvm_context_ );
		llvm::BasicBlock* current_condition_block= next_condition_block;

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
			const Value condition_expression= BuildExpressionCodeAndDestroyTemporaries( *branch.condition, names, function_context );

			if( condition_expression.GetType() != NontypeStub::TemplateDependentValue &&
				condition_expression.GetType().GetTemplateDependentType() == nullptr )
			{
				if( condition_expression.GetType() != bool_type_ )
				{
					errors_.push_back(
						ReportTypesMismatch(
							branch.condition->GetFilePos(),
							bool_type_.ToString(),
							condition_expression.GetType().ToString() ) );
					return if_operator_blocks_build_info;
				}

				llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( *condition_expression.GetVariable(), function_context );
				function_context.llvm_ir_builder.CreateCondBr( condition_in_register, body_block, next_condition_block );
			}
			else
				function_context.llvm_ir_builder.CreateCondBr( llvm::UndefValue::get( fundamental_llvm_types_.bool_ ), body_block, next_condition_block );
		}

		// Make body block code.
		function_context.function->getBasicBlockList().push_back( body_block );
		function_context.llvm_ir_builder.SetInsertPoint( body_block );

		const BlockBuildInfo block_build_info=
			BuildBlockCode( *branch.block, names, function_context );

		have_return_in_all_branches= have_return_in_all_branches && block_build_info.have_unconditional_return_inside;
		have_break_or_continue_in_all_branches= have_break_or_continue_in_all_branches && block_build_info.have_uncodnitional_break_or_continue;

		function_context.llvm_ir_builder.CreateBr( block_after_if );
	}

	U_ASSERT( next_condition_block == block_after_if );

	if( if_operator.branches_.back().condition != nullptr )
	{
		have_return_in_all_branches= false;
		have_break_or_continue_in_all_branches= false;
	}

	// Block after if code.
	function_context.function->getBasicBlockList().push_back( block_after_if );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_if );

	if_operator_blocks_build_info.have_unconditional_return_inside= have_return_in_all_branches;
	if_operator_blocks_build_info.have_uncodnitional_break_or_continue= have_break_or_continue_in_all_branches;
	return if_operator_blocks_build_info;
}

void CodeBuilder::BuildStaticAssert(
	const Synt::StaticAssert& static_assert_,
	NamesScope& names )
{
	const Value expression_result= BuildExpressionCode( *static_assert_.expression, names, *dummy_function_context_ );
	if( expression_result.GetType() == NontypeStub::TemplateDependentValue ||
		expression_result.GetType().GetTemplateDependentType() != nullptr )
		return;

	if( expression_result.GetType() != bool_type_ )
	{
		errors_.push_back( ReportStaticAssertExpressionMustHaveBoolType( static_assert_.file_pos_ ) );
		return;
	}

	const Variable* const variable= expression_result.GetVariable();
	U_ASSERT( variable != nullptr );

	if( variable->constexpr_value == nullptr )
	{
		errors_.push_back( ReportStaticAssertExpressionIsNotConstant( static_assert_.file_pos_ ) );
		return;
	}
	if( llvm::dyn_cast<llvm::UndefValue>(variable->constexpr_value) != nullptr )
	{
		// Undef value means, that value is constexpr, but we are in template prepass, and exact value is unknown. Skip this static_assert
		return;
	}

	if( !variable->constexpr_value->isOneValue() )
	{
		errors_.push_back( ReportStaticAssertionFailed( static_assert_.file_pos_ ) );
		return;
	}
}

void CodeBuilder::BuildHalt( const Synt::Halt& halt, FunctionContext& function_context )
{
	U_UNUSED( halt );

	function_context.llvm_ir_builder.CreateCall( halt_func_ );

	// We needs return, because call to "halt" is not terminal instruction.
	function_context.llvm_ir_builder.CreateRetVoid();
}

void CodeBuilder::BuildHaltIf(const Synt::HaltIf& halt_if, NamesScope& names, FunctionContext& function_context )
{
	llvm::BasicBlock* const true_block = llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const false_block= llvm::BasicBlock::Create( llvm_context_ );

	const Value condition_expression= BuildExpressionCodeAndDestroyTemporaries( *halt_if.condition, names, function_context );

	if( condition_expression.GetType() != NontypeStub::TemplateDependentValue &&
		condition_expression.GetType().GetTemplateDependentType() == nullptr )
	{
		if( condition_expression.GetType() != bool_type_ )
		{
			errors_.push_back(
				ReportTypesMismatch(
					halt_if.condition->GetFilePos(),
					bool_type_.ToString(),
					condition_expression.GetType().ToString() ) );
			return;
		}

		llvm::Value* const condition_in_register= CreateMoveToLLVMRegisterInstruction( *condition_expression.GetVariable(), function_context );
		function_context.llvm_ir_builder.CreateCondBr( condition_in_register, true_block, false_block );
	}
	else
		function_context.llvm_ir_builder.CreateCondBr( llvm::UndefValue::get( fundamental_llvm_types_.bool_ ), true_block, false_block );

	// True branch
	function_context.function->getBasicBlockList().push_back( true_block );
	function_context.llvm_ir_builder.SetInsertPoint( true_block );

	function_context.llvm_ir_builder.CreateCall( halt_func_ );
	function_context.llvm_ir_builder.CreateRetVoid();

	// False branch
	function_context.function->getBasicBlockList().push_back( false_block );
	function_context.llvm_ir_builder.SetInsertPoint( false_block );
}

void CodeBuilder::BuildTypedef(
	const Synt::Typedef& typedef_,
	NamesScope& names )
{
	const Type type= PrepareType( typedef_.file_pos_, typedef_.value, names );
	if( type == invalid_type_ )
		return;

	if( NameShadowsTemplateArgument( typedef_.name, names ) )
		errors_.push_back( ReportDeclarationShadowsTemplateArgument( typedef_.file_pos_, typedef_.name ) );

	const NamesScope::InsertedName* const inserted_name= names.AddName( typedef_.name, Value( type, typedef_.file_pos_ ) );
	if( inserted_name == nullptr )
		errors_.push_back( ReportRedefinition( typedef_.file_pos_, typedef_.name ) );
}

FunctionVariable* CodeBuilder::GetFunctionWithExactSignature(
	const Function& function_type,
	OverloadedFunctionsSet& functions_set )
{
	for( FunctionVariable& function_varaible : functions_set )
	{
		const Function& set_function_type= *function_varaible.type.GetFunctionType();

		if( set_function_type.args == function_type.args )
			return &function_varaible;
	}

	return nullptr;
}

bool CodeBuilder::ApplyOverloadedFunction(
	OverloadedFunctionsSet& functions_set,
	const FunctionVariable& function,
	const FilePos& file_pos )
{
	if( functions_set.empty() )
	{
		functions_set.push_back(function);
		return true;
	}

	const Function* function_type= function.type.GetFunctionType();
	U_ASSERT(function_type);

	/*
	Algorithm for overloading applying:
	If parameter count differs - overload function.
	If "ArgOverloadingClass" of one or more arguments differs - overload function.
	*/
	for( const FunctionVariable& set_function : functions_set )
	{
		const Function& set_function_type= *set_function.type.GetFunctionType(); // Must be function type 100 %

		// If argument count differs - allow overloading.
		// SPRACHE_TODO - handle default arguments.
		if( function_type->args.size() != set_function_type.args.size() )
			continue;

		unsigned int arg_is_same_count= 0u;
		for( size_t i= 0u; i < function_type->args.size(); i++ )
		{
			const Function::Arg& arg= function_type->args[i];
			const Function::Arg& set_arg= set_function_type.args[i];

			if( arg.type != set_arg.type )
				continue;

			if( GetArgOverloadingClass( arg ) == GetArgOverloadingClass( set_arg ) )
				arg_is_same_count++;
		} // For args.

		if( arg_is_same_count == function_type->args.size() )
		{
			errors_.push_back( ReportCouldNotOverloadFunction(file_pos) );
			return false;
		}
	} // For functions in set.

	// No error - add function to set.
	functions_set.push_back(function);
	return true;
}

const FunctionVariable* CodeBuilder::GetOverloadedFunction(
	const OverloadedFunctionsSet& functions_set,
	const std::vector<Function::Arg>& actual_args,
	const bool first_actual_arg_is_this,
	const FilePos& file_pos )
{
	U_ASSERT( !functions_set.empty() );
	U_ASSERT( !( first_actual_arg_is_this && actual_args.empty() ) );

	// If we have only one function - return it.
	// Caller can generate error, if arguments does not match.
	if( functions_set.size() == 1u )
		return &functions_set.front();

	enum class MatchType
	{
		// Overloading class and type match for all parameters.
		Exact,
		// Exact types match for all parameters and mutable reference to immutable reference conversions for some parameters.
		MutToImutReferenceConversion,
		// Types conversion for some parameters ( including references conversion ) and possible mutable to immutable references conversion.
		TypeConversions,
		NoMatch,
	};

	// TODO - use here something like small vectors, or cache this vectors.
	std::vector<unsigned int> exact_match_functions;
	std::vector<unsigned int> match_with_mut_to_imut_cast_functions;
	std::vector<unsigned int> match_with_types_conversion_functions;

	for( const FunctionVariable& function : functions_set )
	{
		const Function& function_type= *function.type.GetFunctionType();

		size_t actial_arg_count;
		const Function::Arg* actual_args_begin;
		if( first_actual_arg_is_this && !function.is_this_call )
		{
			// In case of static function call via "this" compare actual args without "this".
			actual_args_begin= actual_args.data() + 1u;
			actial_arg_count= actual_args.size() - 1u;
		}
		else
		{
			actual_args_begin= actual_args.data();
			actial_arg_count= actual_args.size();
		}

		// SPRACHE_TODO - handle functions with default arguments.
		if( function_type.args.size() != actial_arg_count )
			continue;

		MatchType match_type= MatchType::Exact;
		for( unsigned int i= 0u; i < actial_arg_count; i++ )
		{
			// Function signature is template-dependent. Just return this function.
			// Something is template-dependent. In this case we can return any function with proper number of arguments.
			if( function_type.args[i].type.GetTemplateDependentType() != nullptr ||
				actual_args_begin[i].type.GetTemplateDependentType() != nullptr )
				return &function;

			// SPRACHE_TODO - support type-casting for function call.
			// SPRACHE_TODO - support references-casting.
			// Now - only exactly compare types.
			if( actual_args_begin[i].type != function_type.args[i].type )
			{
				match_type= MatchType::NoMatch;
				break;
			}

			const ArgOverloadingClass arg_overloading_class= GetArgOverloadingClass( actual_args_begin[i] );
			const ArgOverloadingClass parameter_overloading_class= GetArgOverloadingClass( function_type.args[i] );
			if( arg_overloading_class == parameter_overloading_class )
			{
				// All ok, exact match
			}
			else if( parameter_overloading_class == ArgOverloadingClass::MutalbeReference &&
				arg_overloading_class != ArgOverloadingClass::MutalbeReference )
			{
				// We can only bind nonconst-reference arg to nonconst-reference parameter.
				match_type= MatchType::NoMatch;
				break;
			}
			else if( parameter_overloading_class == ArgOverloadingClass::ImmutableReference &&
				arg_overloading_class == ArgOverloadingClass::MutalbeReference )
			{
				if( match_type == MatchType::Exact )
					match_type= MatchType::MutToImutReferenceConversion;
			}
			else
			{
				U_ASSERT(false);
			}
		} // for candidate function args.

		const unsigned int function_index= &function - functions_set.data();
		switch( match_type )
		{
		case MatchType::Exact:
			exact_match_functions.push_back( function_index );
			break;
		case MatchType::MutToImutReferenceConversion:
			match_with_mut_to_imut_cast_functions.push_back( function_index );
			break;
		case MatchType::TypeConversions:
			match_with_types_conversion_functions.push_back( function_index );
			break;
		case MatchType::NoMatch:
			break;
		};
	} // for functions

	if( !exact_match_functions.empty() )
	{
		if( exact_match_functions.size() == 1u )
			return &functions_set[ exact_match_functions.front() ];
		else
		{
			errors_.push_back( ReportTooManySuitableOverloadedFunctions( file_pos ) );
			return nullptr;
		}
	}
	else if( !match_with_mut_to_imut_cast_functions.empty() )
	{
		if( match_with_mut_to_imut_cast_functions.size() == 1u )
			return &functions_set[ match_with_mut_to_imut_cast_functions.front() ];
		else
		{
			errors_.push_back( ReportTooManySuitableOverloadedFunctions( file_pos ) );
			return nullptr;
		}
	}
	else if( !match_with_types_conversion_functions.empty() )
	{
		if( match_with_types_conversion_functions.size() == 1u )
			return &functions_set[ match_with_types_conversion_functions.front() ];
		else
		{
			errors_.push_back( ReportTooManySuitableOverloadedFunctions( file_pos ) );
			return nullptr;
		}
	}
	else
	{
		// Not found any function.
		errors_.push_back( ReportCouldNotSelectOverloadedFunction( file_pos ) );
		return nullptr;
	}
}

void CodeBuilder::CheckReferencedVariables( const Variable& reference, const FilePos& file_pos )
{
	for( const StoredVariablePtr& referenced_variable : reference.referenced_variables )
	{
		if(
			referenced_variable-> mut_use_counter.use_count() <= 2u &&
			referenced_variable->imut_use_counter.use_count() == 1u)
		{} // All ok - one mutable reference.
		else if( referenced_variable-> mut_use_counter.use_count() == 1u )
		{} // All ok - 0-infinity immutable references.
		else
			errors_.push_back( ReportReferenceProtectionError( file_pos ) );
	}
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

} // namespace CodeBuilderLLVMPrivate

} // namespace U
