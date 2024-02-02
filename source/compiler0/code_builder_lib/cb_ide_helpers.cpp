#include <sstream>
#include "keywords.hpp"
#include "../../code_builder_lib_common/string_ref.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "../lex_synt_lib/program_writer.hpp"
#include "code_builder.hpp"

namespace U
{

std::optional<SrcLoc> CodeBuilder::GetDefinition( const SrcLoc& src_loc )
{
	const auto it= definition_points_.find( src_loc );
	if( it == definition_points_.end() )
		return std::nullopt;

	return it->second.src_loc;
}

std::vector<SrcLoc> CodeBuilder::GetAllOccurrences( const SrcLoc& src_loc )
{
	std::vector<SrcLoc> result;

	if( const auto definition_point= GetDefinition( src_loc ) )
	{
		// Found a definition point. Find all usages of it.
		result.push_back( *definition_point );
		for( const auto& definition_point_pair : definition_points_ )
		{
			if( definition_point == definition_point_pair.second.src_loc )
				result.push_back( definition_point_pair.first );
		}
	}
	else
	{
		// Assume, that this is definition itself. Find all usages.
		result.push_back( src_loc );
		for( const auto& definition_point_pair : definition_points_ )
		{
			if( src_loc == definition_point_pair.second.src_loc )
				result.push_back( definition_point_pair.first );
		}
	}

	std::sort( result.begin(), result.end() );
	result.erase( std::unique( result.begin(), result.end() ), result.end() );
	return result;
}

void CodeBuilder::DeleteFunctionsBodies()
{
	// Delete bodies of in code.
	DeleteFunctionsBodies_r( *compiled_sources_.front().names_map );

	// Delete bodies of template functions / functions inside templates.
	for( auto& name_value_pair : generated_template_things_storage_ )
		DeleteFunctionsBodies_r( *name_value_pair.second );

	// Delete destructors of typeinfo classes.
	for( const auto& typeinfo_class : typeinfo_class_table_ )
		DeleteFunctionsBodies_r( *typeinfo_class->members );
}

void CodeBuilder::CollectDefinition( const NamesScopeValue& value, const SrcLoc& src_loc )
{
	if( !collect_definition_points_ )
		return;

	// For now enable saving definitions for non-main (imported) files, in order to implement occurences search.

	DefinitionPoint point;
	point.src_loc= value.src_loc;

	if( value.value.GetFunctionsSet() != nullptr )
		return; // Collect function definitions via separate method.
	if( const auto type_templates_set= value.value.GetTypeTemplatesSet() )
	{
		// TODO - collect type templates definitions specially.
		if( !type_templates_set->type_templates.empty() )
			point.src_loc= type_templates_set->type_templates.front()->src_loc;
	}

	if( point.src_loc.GetFileIndex() >= compiled_sources_.size() )
		return; // Ignore names with generated location (like built-in types).

	// Reset macro expansion contexts.
	// This fixes search of definitions/usages inside macro expansions.
	// This breaks search within macro definitions itself, but it is anyway irrelevant.
	SrcLoc src_loc_corrected= src_loc;
	src_loc_corrected.SetMacroExpansionIndex( SrcLoc::c_max_macro_expanison_index );
	point.src_loc.SetMacroExpansionIndex( SrcLoc::c_max_macro_expanison_index );

	definition_points_.insert( std::make_pair( src_loc_corrected, std::move(point) ) );
}

void CodeBuilder::CollectFunctionDefinition( const FunctionVariable& function_variable, const SrcLoc& src_loc )
{
	if( !collect_definition_points_ )
		return;

	DefinitionPoint point;
	point.src_loc= function_variable.body_src_loc;

	if( point.src_loc.GetFileIndex() >= compiled_sources_.size() )
		return; // Ignore names with generated location (like built-in types).

	// Reset macro expansion contexts.
	// This fixes search of definitions/usages inside macro expansions.
	// This breaks search within macro definitions itself, but it is anyway irrelevant.
	SrcLoc src_loc_corrected= src_loc;
	src_loc_corrected.SetMacroExpansionIndex( SrcLoc::c_max_macro_expanison_index );
	point.src_loc.SetMacroExpansionIndex( SrcLoc::c_max_macro_expanison_index );

	definition_points_.insert( std::make_pair( src_loc_corrected, std::move(point) ) );
}

NamesScopePtr CodeBuilder::GetNamesScopeForCompletion( const llvm::ArrayRef<CompletionRequestPrefixComponent> prefix )
{
	const NamesScopePtr& root_names_scope= compiled_sources_.front().names_map;
	return EvaluateCompletionRequestPrefix_r( root_names_scope, prefix );
}

NamesScopePtr CodeBuilder::EvaluateCompletionRequestPrefix_r( const NamesScopePtr& start_scope, const llvm::ArrayRef<CompletionRequestPrefixComponent> prefix )
{
	if( prefix.empty() || start_scope == nullptr )
		return start_scope;

	const CompletionRequestPrefixComponent& prefix_head= prefix.front();
	const auto prefix_tail= prefix.drop_front();

	if( const auto namespace_= std::get_if<const Synt::Namespace*>( &prefix_head ) )
	{
		if( const NamesScopeValue* const value= start_scope->GetThisScopeValue( (*namespace_)->name ) )
		{
			if( const auto names_scope= value->value.GetNamespace() )
				return EvaluateCompletionRequestPrefix_r( names_scope, prefix_tail );
		}
	}
	else if( const auto class_= std::get_if<const Synt::Class*>( &prefix_head ) )
	{
		if( const NamesScopeValue* const value= start_scope->GetThisScopeValue( (*class_)->name ) )
		{
			if( const auto type_name= value->value.GetTypeName() )
			{
				if( const auto class_type= type_name->GetClassType() )
					return EvaluateCompletionRequestPrefix_r( class_type->members, prefix_tail );
			}
		}
	}
	else if( const auto type_template= std::get_if<const Synt::TypeTemplate*>( &prefix_head ) )
	{
		TypeTemplatesSet temp_type_templates_set;
		PrepareTypeTemplate( **type_template, temp_type_templates_set, *start_scope );
		if( temp_type_templates_set.type_templates.empty() )
			return nullptr;
		const TypeTemplatePtr& prepared_type_template= temp_type_templates_set.type_templates.front();

		// Try to found existing type template with same signature and provess completion inside it.
		// It is faster to existing type template, rather than creating new one for each completion request.
		if( const NamesScopeValue* const value= start_scope->GetThisScopeValue( (*type_template)->name ) )
		{
			if( const auto type_templates_set= value->value.GetTypeTemplatesSet() )
			{
				for( const TypeTemplatePtr& existing_type_template : type_templates_set->type_templates )
				{
					if( prepared_type_template->signature_params == existing_type_template->signature_params )
					{
						// Found this type template.
						if( const auto type_template_space= InstantiateTypeTemplateWithDummyArgs( existing_type_template ) )
							return EvaluateCompletionRequestPrefix_r( type_template_space, prefix_tail );
					}
				}
			}
		}

		if( const auto type_template_space= InstantiateTypeTemplateWithDummyArgs( prepared_type_template ) )
			return EvaluateCompletionRequestPrefix_r( type_template_space, prefix_tail );
	}
	else U_ASSERT(false);

	return nullptr;
}

std::vector<CodeBuilder::CompletionItem> CodeBuilder::CompletionResultFinalize()
{
	std::vector<CompletionItem> result;
	result.swap( completion_items_ );

	// Ideally we should filter-out shadowed names, but it is for now too complicated.

	// Sort result list and remove duplicates.
	// Duplicates are possible, since name lookup may be performed multiple types, like for epression preevaluation.
	std::sort(
		result.begin(), result.end(),
		[]( const CompletionItem& l, const CompletionItem & r )
		{
			if( l.sort_text != r.sort_text ) // Compare sort text first.
				return l.sort_text < r.sort_text;
			if( l.name != r.name )
				return l.name < r.name;
			if( l.detail != r.detail )
				return l.detail < r.detail;
			return l.kind < r.kind;
		} );

	const auto new_end=
		std::unique(
			result.begin(),
			result.end(),
			[]( const CompletionItem& l, const CompletionItem& r )
			{
				return l.name == r.name && l.sort_text == r.sort_text && l.detail == r.detail && l.kind == r.kind;
			} );

	result.erase( new_end, result.end() );

	return result;
}

std::vector<CodeBuilder::SignatureHelpItem> CodeBuilder::SignatureHelpResultFinalize()
{
	std::vector<SignatureHelpItem> result;
	result.swap( signature_help_items_ );

	// Sort by label in order to have stable result.
	std::sort(
		result.begin(), result.end(),
		[]( const SignatureHelpItem& l, const SignatureHelpItem& r ) { return l.label < r.label; });

	return result;
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::VariablesDeclaration& variables_declaration )
{
	WithGlobalFunctionContext(
		[&]( FunctionContext& function_context )
		{
			// Complete type name.
			const Type type= PrepareType( variables_declaration.type, names_scope, function_context );

			// Complete names in initializers.
			for( const Synt::VariablesDeclaration::VariableEntry& variable_entry : variables_declaration.variables )
			{
				if( variable_entry.initializer == nullptr )
					continue;

				const VariablePtr variable=
					Variable::Create(
						type,
						ValueType::Value,
						Variable::Location::Pointer,
						variable_entry.name + " variable itself" );

				function_context.variables_state.AddNode( variable );

				const VariablePtr variable_for_initialization=
					Variable::Create(
						type,
						ValueType::ReferenceMut,
						Variable::Location::Pointer,
						variable_entry.name,
						variable->llvm_value );

				function_context.variables_state.AddNode( variable_for_initialization );
				function_context.variables_state.AddLink( variable, variable_for_initialization );

				ApplyInitializer( variable_for_initialization, names_scope, function_context, *variable_entry.initializer );

				function_context.variables_state.RemoveNode( variable_for_initialization );
				function_context.variables_state.RemoveNode( variable );
			}
		} );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::AutoVariableDeclaration& auto_variable_declaration )
{
	// Complete names in auto-variable expression initializer.
	WithGlobalFunctionContext(
		[&]( FunctionContext& function_context ) { BuildExpressionCode( auto_variable_declaration.initializer_expression, names_scope, function_context ); } );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::StaticAssert& static_assert_ )
{
	// Complete names in static assert expression.
	WithGlobalFunctionContext(
		[&]( FunctionContext& function_context ) { BuildExpressionCode( static_assert_.expression, names_scope, function_context ); } );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::TypeAlias& type_alias )
{
	// Complete names in aliased type name.
	WithGlobalFunctionContext(
		[&]( FunctionContext& function_context ) { PrepareType( type_alias.value, names_scope, function_context ); } );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Enum& enum_ )
{
	// Nothing to complete in enum.
	(void)names_scope;
	(void)enum_;
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Function& function )
{
	NamesScope* actual_names_scope= nullptr;
	ClassPtr base_class= nullptr;

	const auto& name= function.name;

	if( name.size() > 1 )
	{
		// Out of line definition - fetch proper namespace.
		NamesScopeValue* value= nullptr;
		size_t component_index= 0u;
		if( name.front().name.empty() )
		{
			if( name[0].completion_requested || name[1].completion_requested )
			{
				RootNamespaseLookupCompleteImpl( names_scope, name[1].name );
				return;
			}

			// Perform function name lookup starting from root.
			U_ASSERT( name.size() >= 2u );
			value= names_scope.GetRoot()->GetThisScopeValue( name[1].name );
			++component_index;
			if( value != nullptr )
				CollectDefinition( *value, name[1].src_loc );
		}
		else
		{
			if( name[0].completion_requested )
			{
				NameLookupCompleteImpl( names_scope, name[0].name );
				return;
			}

			// Perform regular name lookup.
			value= LookupName( names_scope, name[0].name, function.src_loc ).value;
			if( value != nullptr )
				CollectDefinition( *value, name[0].src_loc );
		}

		// Iterate over name components except last and fetch scopes.
		// Last component is not a scope name, but function name, so, ignore it.
		for( size_t i= component_index; i + 1 < name.size(); ++i )
		{
			if( value == nullptr )
				return;

			actual_names_scope= nullptr;
			if( const auto namespace_= value->value.GetNamespace() )
				actual_names_scope= namespace_.get();
			else if( const auto type= value->value.GetTypeName() )
			{
				if( const auto class_= type->GetClassType() )
				{
					actual_names_scope= class_->members.get();

					if( i + 2 == name.size() )
						base_class= class_;
				}
			}

			if( actual_names_scope == nullptr )
				return;

			if( name[i + 1].completion_requested )
			{
				NamesScopeFetchComleteForNamesScope( *actual_names_scope, name[i + 1].name );
				return;
			}

			value= actual_names_scope->GetThisScopeValue( name[i + 1].name );
		}

		if( actual_names_scope == nullptr )
			return;
	}
	else
	{
		if( name.front().completion_requested )
		{
			NameLookupCompleteImpl( names_scope, name.front().name );
			return;
		}

		// Declaration/definition in current scope.
		actual_names_scope= &names_scope;

		base_class= names_scope.GetClass();
	}

	OverloadedFunctionsSet functions_set;

	// Consider this not an out-of line definition.
	// There is no reason to set this flag to true, since it affects only some consistency checks.
	const bool out_of_line_flag= false;

	// Prepare function - complete names in types of params and return value.
	const size_t function_index= PrepareFunction( *actual_names_scope, base_class, functions_set, function, out_of_line_flag );

	if( function_index >= functions_set.functions.size() )
	{
		// Something went wrong.
		return;
	}

	if( function.block == nullptr )
		return; // This is only prototype.

	FunctionVariable& function_variable= functions_set.functions[ function_index ];

	// Build function code - complete names inside its body.
	BuildFuncCode( function_variable, base_class, *actual_names_scope, name.back().name );

	// Clear garbage - remove created llvm function.
	if( function_variable.llvm_function->function != nullptr )
		function_variable.llvm_function->function->eraseFromParent();
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Class& class_ )
{
	WithGlobalFunctionContext(
		[&]( FunctionContext& function_context )
		{
			// Complete names in parent names.
			for( const Synt::ComplexName& parent_name : class_.parents )
				ResolveValue( names_scope, function_context, parent_name );

			// Complete names in non-sync tag.
			if( const auto non_sync_expression= std::get_if< std::unique_ptr<const Synt::Expression> >( &class_.non_sync_tag ) )
			{
				if( *non_sync_expression != nullptr )
					BuildExpressionCode( **non_sync_expression, names_scope, function_context );
			}

			// Do not complete class members, since completion for class member should be triggered instead.
		} );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::TypeTemplate& type_template )
{
	TypeTemplatesSet temp_type_templates_set;
	PrepareTypeTemplate( type_template, temp_type_templates_set, names_scope );
	if( !temp_type_templates_set.type_templates.empty() )
		InstantiateTypeTemplateWithDummyArgs( temp_type_templates_set.type_templates.front() );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::FunctionTemplate& function_template_syntax_element )
{
	// Prepare function template and instantiate it with dummy template args in order to produce some sort of usefull completion result.

	OverloadedFunctionsSet functions_set;
	PrepareFunctionTemplate( function_template_syntax_element, functions_set, names_scope, names_scope.GetClass() );

	FunctionTemplatePtr result_function_template;
	for( const FunctionTemplatePtr& function_template : functions_set.template_functions )
	{
		if( function_template != nullptr && function_template->syntax_element == &function_template_syntax_element )
		{
			result_function_template= function_template;
			break;
		}
	}

	if( result_function_template == nullptr )
		return;

	InstantiateFunctionTemplateWithDummyArgs( result_function_template );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Namespace& namespace_ )
{
	// Nothing to do here, since completion for namespace has no sense and completion for namespace member will be trigered otherwise.
	(void)names_scope;
	(void)namespace_;
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::ClassField& class_field )
{
	WithGlobalFunctionContext(
		[&]( FunctionContext& function_context )
		{
			// Complete type name of class field.
			PrepareType( class_field.type, names_scope, function_context );

			// Complete inside reference notation expressions.
			if( !std::holds_alternative< Synt::EmptyVariant >( class_field.reference_tag_expression ) )
				BuildExpressionCode( class_field.reference_tag_expression, names_scope, function_context );
			if( !std::holds_alternative< Synt::EmptyVariant >( class_field.inner_reference_tags_expression ) )
				BuildExpressionCode( class_field.inner_reference_tags_expression, names_scope, function_context );
		} );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::ClassVisibilityLabel& class_visibility_label )
{
	// Nothing to complete in class visibility label.
	(void)names_scope;
	(void)class_visibility_label;
}

NamesScopePtr CodeBuilder::InstantiateTypeTemplateWithDummyArgs( const TypeTemplatePtr& type_template )
{
	const auto template_args_scope= std::make_shared<NamesScope>( std::string( NamesScope::c_template_args_namespace_name ), type_template->parent_namespace );

	// Since (normally) template args should be evaluated during matching of signature params,
	// pefrorm dummy signature args creation.
	// This will fill template args properly.
	TemplateArgs signature_args;
	signature_args.reserve( type_template->signature_params.size() );
	for( const TemplateSignatureParam& signature_param : type_template->signature_params )
		signature_args.push_back( CreateDummyTemplateSignatureArg( *type_template, *template_args_scope, signature_param ) );

	{
		const TemplateKey template_key{ type_template, signature_args };
		if( const auto it= generated_template_things_storage_.find( template_key ); it != generated_template_things_storage_.end() )
		{
			// If this is not first instantiation, return previous namespace, where inserted type is really located.
			const NamesScopePtr template_parameters_space= it->second;
			U_ASSERT( template_parameters_space != nullptr );
			return template_parameters_space;
		}
	}

	const auto prev_skip_building_generated_functions= skip_building_generated_functions_;
	skip_building_generated_functions_= false;

	const std::optional<Type> type=
		FinishTemplateTypeGeneration(
			SrcLoc(),
			*EnsureDummyTemplateInstantiationArgsScopeCreated(),
			TemplateTypePreparationResult{ type_template, template_args_scope, signature_args } );

	if( type != std::nullopt )
	{
		if( const auto class_type= type->GetClassType() )
		{
			GlobalThingBuildClass( class_type );
			GlobalThingBuildNamespace( *class_type->members );
		}
	}

	skip_building_generated_functions_= prev_skip_building_generated_functions;

	return template_args_scope;
}

void CodeBuilder::InstantiateFunctionTemplateWithDummyArgs( const FunctionTemplatePtr& function_template )
{
	const auto template_args_scope= std::make_shared<NamesScope>( std::string( NamesScope::c_template_args_namespace_name ), function_template->parent_namespace );

	// Since it is not always possible to calculate template args from signature args for function template,
	// perform direct args filling.
	TemplateArgs template_args;
	template_args.reserve( function_template->template_params.size() );
	for( const TemplateBase::TemplateParameter& param : function_template->template_params )
		template_args.push_back( CreateDummyTemplateSignatureArgForTemplateParam( *function_template, *template_args_scope, param ) );

	// Do not care here about signature params filling.
	// For function templates they are used mostly for overloaded resolition.
	// But we do not need it for completion.

	const auto prev_skip_building_generated_functions= skip_building_generated_functions_;
	skip_building_generated_functions_= false;

	FinishTemplateFunctionGeneration(
		EnsureDummyTemplateInstantiationArgsScopeCreated()->GetErrors(),
		SrcLoc(),
		TemplateFunctionPreparationResult{ function_template, template_args_scope } );

	skip_building_generated_functions_= prev_skip_building_generated_functions;
}

TemplateArg CodeBuilder::CreateDummyTemplateSignatureArg( const TemplateBase& template_, NamesScope& args_names_scope, const TemplateSignatureParam& signature_param )
{
	return signature_param.Visit( [&]( const auto& el ) { return CreateDummyTemplateSignatureArgImpl( template_, args_names_scope, el ); } );
}

TemplateArg CodeBuilder::CreateDummyTemplateSignatureArgImpl( const TemplateBase& template_, NamesScope& args_names_scope, const TemplateSignatureParam::TypeParam& type_param )
{
	(void)template_;
	(void)args_names_scope;

	return type_param.t;
}

TemplateArg CodeBuilder::CreateDummyTemplateSignatureArgImpl( const TemplateBase& template_, NamesScope& args_names_scope, const TemplateSignatureParam::VariableParam& variable_param )
{
	(void)template_;
	(void)args_names_scope;

	TemplateVariableArg arg;
	arg.type= variable_param.type;
	arg.constexpr_value= llvm::Constant::getNullValue( arg.type.GetLLVMType() );

	return std::move(arg);
}

TemplateArg CodeBuilder::CreateDummyTemplateSignatureArgImpl( const TemplateBase& template_, NamesScope& args_names_scope, const TemplateSignatureParam::TemplateParam& template_param )
{
	if( template_param.index < template_.template_params.size() )
		return CreateDummyTemplateSignatureArgForTemplateParam( template_, args_names_scope, template_.template_params[ template_param.index ] );

	return invalid_type_;
}

TemplateArg CodeBuilder::CreateDummyTemplateSignatureArgImpl( const TemplateBase& template_, NamesScope& args_names_scope, const TemplateSignatureParam::ArrayParam& array_param )
{
	const TemplateArg element_type= CreateDummyTemplateSignatureArg( template_, args_names_scope, *array_param.element_type );
	const TemplateArg element_count= CreateDummyTemplateSignatureArg( template_, args_names_scope, *array_param.element_count );

	if( const auto t= std::get_if<Type>( &element_type ) )
	{
		if( const auto v= std::get_if<TemplateVariableArg>( &element_count ) )
		{
			ArrayType array_type;
			array_type.element_type= *t;
			array_type.element_count= v->constexpr_value->getUniqueInteger().getLimitedValue();
			array_type.llvm_type= llvm::ArrayType::get( array_type.element_type.GetLLVMType(), array_type.element_count );
			return Type( std::move(array_type) );
		}
	}

	return invalid_type_;
}

TemplateArg CodeBuilder::CreateDummyTemplateSignatureArgImpl( const TemplateBase& template_, NamesScope& args_names_scope, const TemplateSignatureParam::TupleParam& tuple_param )
{
	TupleType tuple_type;
	tuple_type.element_types.reserve( tuple_param.element_types.size() );
	llvm::SmallVector<llvm::Type*, 16> elements_llvm_types;
	elements_llvm_types.reserve( tuple_param.element_types.size() );

	for( const TemplateSignatureParam& element_param : tuple_param.element_types )
	{
		const TemplateArg element_type= CreateDummyTemplateSignatureArg( template_, args_names_scope, element_param );
		if( const auto t= std::get_if<Type>( &element_type ) )
		{
			tuple_type.element_types.push_back( *t );
			elements_llvm_types.push_back( t->GetLLVMType() );
		}
		else
			return invalid_type_;
	}

	tuple_type.llvm_type= llvm::StructType::get( llvm_context_, elements_llvm_types );

	return Type( std::move(tuple_type) );
}

TemplateArg CodeBuilder::CreateDummyTemplateSignatureArgImpl( const TemplateBase& template_, NamesScope& args_names_scope, const TemplateSignatureParam::RawPointerParam& raw_pointer_param )
{
	const TemplateArg element_type= CreateDummyTemplateSignatureArg( template_, args_names_scope, *raw_pointer_param.element_type );
	if( const auto t= std::get_if<Type>( &element_type ) )
	{
		RawPointerType raw_pointer;
		raw_pointer.element_type= *t;
		raw_pointer.llvm_type= raw_pointer.element_type.GetLLVMType()->getPointerTo();

		return Type( std::move(raw_pointer) );
	}

	return invalid_type_;
}

TemplateArg CodeBuilder::CreateDummyTemplateSignatureArgImpl( const TemplateBase& template_, NamesScope& args_names_scope, const TemplateSignatureParam::FunctionParam& function_param )
{
	FunctionType function_type;

	for( const TemplateSignatureParam::FunctionParam::Param& param : function_param.params )
	{
		FunctionType::Param out_param;

		const TemplateArg param_type= CreateDummyTemplateSignatureArg( template_, args_names_scope, *param.type );
		if( const auto t= std::get_if<Type>( &param_type ) )
			out_param.type= *t;
		else
			return invalid_type_;

		out_param.value_type= param.value_type;
		function_type.params.push_back( std::move(out_param) );
	}

	const TemplateArg return_type= CreateDummyTemplateSignatureArg( template_, args_names_scope, *function_param.return_type );
	if( const auto t= std::get_if<Type>( &return_type ) )
		function_type.return_type= *t;
	else
		return invalid_type_;

	function_type.return_value_type= function_param.return_value_type;
	function_type.unsafe= function_param.is_unsafe;
	function_type.calling_convention= function_param.calling_convention;

	return Type( FunctionPointerType{ std::move(function_type), llvm::PointerType::get( llvm_context_, 0 ) } );
}

TemplateArg CodeBuilder::CreateDummyTemplateSignatureArgImpl( const TemplateBase& template_, NamesScope& args_names_scope, const TemplateSignatureParam::CoroutineParam& coroutine_param )
{
	const TemplateArg return_type_type= CreateDummyTemplateSignatureArg( template_, args_names_scope, *coroutine_param.return_type );
	if( const auto t= std::get_if<Type>( &return_type_type ) )
	{
		CoroutineTypeDescription coroutine_type_description;
		coroutine_type_description.kind= coroutine_param.kind;
		coroutine_type_description.return_type= *t;
		coroutine_type_description.return_value_type= coroutine_param.return_value_type;
		coroutine_type_description.inner_references= coroutine_param.inner_references;
		coroutine_type_description.non_sync= coroutine_param.non_sync;

		return Type( GetCoroutineType( *args_names_scope.GetRoot(), coroutine_type_description ) );
	}

	return invalid_type_;
}

TemplateArg CodeBuilder::CreateDummyTemplateSignatureArgImpl( const TemplateBase& template_, NamesScope& args_names_scope, const TemplateSignatureParam::SpecializedTemplateParam& specialized_template_param )
{
	std::vector<TemplateArg> args;
	args.reserve( specialized_template_param.params.size() );
	for( const TemplateSignatureParam& param : specialized_template_param.params )
		args.push_back( CreateDummyTemplateSignatureArg( template_, args_names_scope, param ) );

	// TODO - instantiate some type template here.
	return invalid_type_;
}

TemplateArg CodeBuilder::CreateDummyTemplateSignatureArgForTemplateParam( const TemplateBase& template_, NamesScope& args_names_scope, const TemplateBase::TemplateParameter& param )
{
	if( const auto inserted_arg= args_names_scope.GetThisScopeValue( param.name ) )
	{
		// Template arg was already created. Just fetch it.
		if( const auto type= inserted_arg->value.GetTypeName() )
			return *type;
		else if( const auto variable= inserted_arg->value.GetVariable() )
			return TemplateVariableArg( *variable );
	}
	else
	{
		if( param.type != std::nullopt )
		{
			// Create variable arg.
			const TemplateArg type_arg= CreateDummyTemplateSignatureArg( template_, args_names_scope, *param.type );
			if( const auto t= std::get_if<Type>( &type_arg ) )
			{
				TemplateVariableArg arg;
				arg.type= *t;
				arg.constexpr_value= llvm::Constant::getNullValue( arg.type.GetLLVMType() );

				const VariablePtr variable_for_insertion=
					Variable::Create(
						arg.type,
						ValueType::ReferenceImut,
						Variable::Location::Pointer,
						param.name,
						CreateGlobalConstantVariable( arg.type, param.name, arg.constexpr_value ),
						arg.constexpr_value );

				args_names_scope.AddName( param.name, NamesScopeValue( variable_for_insertion, param.src_loc ) );
				return std::move(arg);
			}
		}
		else
		{
			// Create type arg. Use stub type for this.

			const Type t= GetStubTemplateArgType();
			args_names_scope.AddName( param.name, NamesScopeValue( t, param.src_loc ) );
			return t;
		}
	}

	return invalid_type_;
}

Type CodeBuilder::GetStubTemplateArgType()
{
	if( stub_template_param_type_ != std::nullopt )
		return *stub_template_param_type_; // Already created.

	// Generate some enum. It is equivalent of something like
	//	struct TemplateParam { Member0 }
	// Use enum since enums can also be used as template value params.

	NamesScope& root_namespace= *compiled_sources_.front().names_map;

	auto stub_enum= std::make_unique<Enum>( "TemplateParam", &root_namespace );
	const EnumPtr enum_type= stub_enum.get();
	enums_table_.push_back( std::move(stub_enum) );

	enum_type->underlying_type= FundamentalType( U_FundamentalType::u8_, fundamental_llvm_types_.u8_ );
	enum_type->element_count= 1;

	const std::string_view member_name= "Member0";
	const auto constexpr_value= llvm::Constant::getNullValue( enum_type->underlying_type.llvm_type );

	const VariablePtr member_variable=
		Variable::Create(
			enum_type,
			ValueType::ReferenceImut,
			Variable::Location::Pointer,
			std::string( member_name ),
			CreateGlobalConstantVariable(
				enum_type,
				mangler_->MangleGlobalVariable( enum_type->members, member_name, enum_type, true ),
				constexpr_value ),
			constexpr_value );

	enum_type->members.AddName( member_name, NamesScopeValue( member_variable, SrcLoc() ) );

	stub_template_param_type_= Type( enum_type );
	return *stub_template_param_type_;
}

NamesScopePtr CodeBuilder::EnsureDummyTemplateInstantiationArgsScopeCreated()
{
	if( dummy_template_instantiation_args_scope_ != nullptr )
		return dummy_template_instantiation_args_scope_;

	dummy_template_instantiation_args_scope_= std::make_unique<NamesScope>( "_dummy_instantiation_point_scope", compiled_sources_.front().names_map.get() );

	// Create errors container, not linked with root errors container.
	// Doing so we ignore all errors in dummy template instantiations.
	const auto errors_container= std::make_shared<CodeBuilderErrorsContainer>();
	dummy_template_instantiation_args_scope_->SetErrors( errors_container );

	return dummy_template_instantiation_args_scope_;
}

void CodeBuilder::DummyInstantiateTemplates()
{
	const auto prev_skip_building_generated_functions= skip_building_generated_functions_;
	skip_building_generated_functions_= false;

	DummyInstantiateTemplates_r( *compiled_sources_.front().names_map );

	// Instantiate also contents of templates.
	// use index-based for because this array may be modified during iteration.
	for( size_t i= 0; i < generated_template_things_sequence_.size(); ++i )
	{
		const NamesScopePtr namespace_= generated_template_things_storage_[generated_template_things_sequence_[i]];
		DummyInstantiateTemplates_r( *namespace_ );
	}

	skip_building_generated_functions_= prev_skip_building_generated_functions;
}

void CodeBuilder::DummyInstantiateTemplates_r( NamesScope& names_scope )
{
	// Instantiate templates with dummy params.
	// Doing so we allow to collect definition points for template code, even if it was not instantiated.
	// Process only templates, defined in root namespace.
	names_scope.ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const auto type_templates_set= value.GetTypeTemplatesSet() )
			{
				for( const TypeTemplatePtr& type_template : type_templates_set->type_templates )
				{
					if( type_template->src_loc.GetFileIndex() == 0 )
						InstantiateTypeTemplateWithDummyArgs( type_template );
				}
			}
			else if( const auto functions_set= value.GetFunctionsSet() )
			{
				for( const FunctionTemplatePtr& function_template : functions_set->template_functions )
				{
					if( function_template->src_loc.GetFileIndex() == 0 )
						InstantiateFunctionTemplateWithDummyArgs( function_template );
				}
			}
			else if( const NamesScopePtr inner_namespace= value.GetNamespace() )
				DummyInstantiateTemplates_r( *inner_namespace );
			else if( const Type* const type= value.GetTypeName() )
			{
				if( const ClassPtr class_type= type->GetClassType() )
				{
					// Process classes only from parent namespace.
					// Otherwise we can get loop, using type alias.
					if( class_type->members->GetParent() == &names_scope )
						DummyInstantiateTemplates_r( *class_type->members );
				}
			}
		});
}

void CodeBuilder::NameLookupCompleteImpl( const NamesScope& names_scope, const std::string_view name )
{
	// Complete for this names_scope and all parents until root.
	const NamesScope* current_scope= &names_scope;
	do
	{
		NamesScopeFetchComleteForNamesScope( *current_scope, name );
		current_scope= current_scope->GetParent();
	} while( current_scope != nullptr );
}

void CodeBuilder::RootNamespaseLookupCompleteImpl( const NamesScope& names_scope, const std::string_view name )
{
	const auto root= names_scope.GetRoot();
	U_ASSERT( root != nullptr );
	NamesScopeFetchComleteForNamesScope( *root, name );
}

void CodeBuilder::NamesScopeFetchComleteImpl( const Value& base, const std::string_view name )
{
	if( const NamesScopePtr inner_namespace= base.GetNamespace() )
		NamesScopeFetchComleteForNamesScope( *inner_namespace, name );
	else if( const Type* const type= base.GetTypeName() )
	{
		if( const ClassPtr class_= type->GetClassType() )
			NamesScopeFetchComleteForClass( class_, name );
		else if( const EnumPtr enum_= type->GetEnumType() )
			NamesScopeFetchComleteForNamesScope( enum_->members, name );
	}
}

void CodeBuilder::MemberAccessCompleteImpl( const VariablePtr& variable, const std::string_view name )
{
	if( variable == nullptr )
		return;

	if( Class* const class_= variable->type.GetClassType() )
		NamesScopeFetchComleteForClass( class_, name );
}

void CodeBuilder::NamesScopeFetchComleteForNamesScope( const NamesScope& names_scope, const std::string_view name )
{
	names_scope.ForEachInThisScope(
		[&]( const std::string_view value_name, const NamesScopeValue& names_scope_value )
		{
			CompleteProcessValue( name, value_name, names_scope_value );
		});
}

void CodeBuilder::NamesScopeFetchComleteForClass( const Class* const class_, const std::string_view name )
{
	// Complete for class members.
	NamesScopeFetchComleteForNamesScope( *class_->members, name );

	// Complete for all parent classes, since parent class membes are accessible via child namespace.
	for( const auto& parent : class_->parents )
		NamesScopeFetchComleteForClass( parent.class_, name );
}

void CodeBuilder::ComleteClassOwnFields( const Class* class_, const std::string_view name )
{
	class_->members->ForEachInThisScope(
		[&]( const std::string_view value_name, const NamesScopeValue& names_scope_value )
		{
			if( names_scope_value.value.GetClassField() == nullptr )
				return;

			CompleteProcessValue( name, value_name, names_scope_value );
		});
}

void CodeBuilder::CompleteProcessValue( const std::string_view completion_name, const std::string_view value_name, const NamesScopeValue& names_scope_value )
{
	const llvm::StringRef completion_name_ref= StringViewToStringRef(completion_name);
	const llvm::StringRef value_name_ref= StringViewToStringRef( value_name );

	// Use this name if given name is substring (ignoring case) of provided name.
	// TODO - support Unicode names.
	const auto pos=value_name_ref.find_insensitive( completion_name_ref );
	if( completion_name_ref.empty() || pos != llvm::StringRef::npos )
	{
		if( value_name_ref.startswith( StringViewToStringRef( Keyword(Keywords::static_assert_) ) ) || // static_assert name may exist inside namespace, but we should ignore it.
			StringToOverloadedOperator( value_name ) != std::nullopt // Ignore all overloaded operators. There is no reason and no possibility to access overloaded operator by name.
			// Still allow to access constructors and destructors, even if it is needed very rarely.
			)
		{
			return;
		}

		const Value& value= names_scope_value.value;

		if( const auto functions_set= value.GetFunctionsSet() )
		{
			// Ignore functions set with no functions (for example, if they are disabled via "enable_if").
			if( functions_set->functions.empty() && functions_set->template_functions.empty() )
				return;
		}

		CompletionItem item;
		item.name= std::string(value_name);

		// Perform prioritization by prefixing name in sort text.
		// All values names, starting with given text have more priority, than values with name matching given in the middle/at end.
		if( completion_name_ref.empty() || pos == 0 )
			item.sort_text= "0_" + item.name;
		else
			item.sort_text= "1_" + item.name;

		// TODO - fill detail for other kinds of values.

		if( const auto variable= value.GetVariable() )
		{
			item.kind= CompletionItemKind::Variable;
			item.detail= variable->type.ToString();
		}
		else if( value.GetFunctionsSet() != nullptr || value.GetThisOverloadedMethodsSet() != nullptr )
			item.kind= CompletionItemKind::FunctionsSet;
		else if( const auto type= value.GetTypeName() )
		{
			item.kind= CompletionItemKind::Type;

			if( type->GetFundamentalType() == nullptr )
			{
				// Fill detail for types except fundamentals.

				// Prefix structs/classes and enums with specific keyword.
				if( const auto class_type= type->GetClassType() )
				{
					item.detail+= Keyword( class_type->kind == Class::Kind::Struct ? Keywords::struct_ : Keywords::class_ );
					item.detail+= " ";
				}
				else if( type->GetEnumType() != nullptr )
				{
					item.detail+= Keyword( Keywords::enum_ );
					item.detail+= " ";
				}

				item.detail+= type->ToString();
			}
		}
		else if( value.GetTypeAlias() != nullptr )
			item.kind= CompletionItemKind::Type;
		else if( const auto class_field= value.GetClassField() )
		{
			item.kind= CompletionItemKind::ClassField;

			if( class_field->is_reference )
			{
				item.detail+= "&";
				item.detail+= Keyword( class_field->is_mutable ? Keywords::mut_ : Keywords::imut_ );
				item.detail+= " ";
			}
			else if( !class_field->is_mutable )
			{
				item.detail+= Keyword( Keywords::imut_ );
				item.detail+= " ";
			}

			item.detail+= class_field->type.ToString();
		}
		else if( value.GetNamespace() != nullptr )
			item.kind= CompletionItemKind::NamesScope;
		else if( value.GetTypeTemplatesSet() != nullptr )
			item.kind= CompletionItemKind::TypeTemplatesSet;

		completion_items_.push_back( std::move(item) );
	}
}

void CodeBuilder::PerformSignatureHelp( const Value& value )
{
	OverloadedFunctionsSetConstPtr functions_set;

	if( const auto value_functions_set= value.GetFunctionsSet() )
		functions_set= value_functions_set;
	else if( const auto overloaded_methods_set= value.GetThisOverloadedMethodsSet() )
		functions_set= overloaded_methods_set->overloaded_methods_set;
	else if( const auto type= value.GetTypeName() )
	{
		// This is temp variable construction. Try to extract constructors for given type.
		if( const auto class_type= type->GetClassType() )
		{
			if( const auto constructors= class_type->members->GetThisScopeValue( Keyword( Keywords::constructor_ ) ) )
			{
				if( const auto constructors_functions_set= constructors->value.GetFunctionsSet() )
					functions_set= constructors_functions_set;
			}
		}
	}
	else if( const auto variable= value.GetVariable() )
	{
		if( variable->type.GetFunctionPointerType() != nullptr )
		{
			// Suggest call to function pointer.
			SignatureHelpItem item;
			item.label= variable->type.ToString();
			signature_help_items_.push_back( std::move(item) );
		}
		else if( const auto class_type= variable->type.GetClassType() )
		{
			// Try to call overloaded () operator.
			if( const NamesScopeValue* const call_operator_value= ResolveClassValue( class_type, OverloadedOperatorToString( OverloadedOperator::Call ) ).first )
			{
				if( const auto operator_functions_set= call_operator_value->value.GetFunctionsSet() )
					functions_set= operator_functions_set;
			}
		}
	}

	if( functions_set == nullptr )
		return;

	for( const FunctionVariable& function : functions_set->functions )
	{
		if( function.is_deleted )
			continue;

		std::stringstream ss;

		if( function.syntax_element != nullptr )
		{
			if( !function.syntax_element->name.empty() )
				ss << function.syntax_element->name.back().name;

			Synt::WriteFunctionParamsList( function.syntax_element->type, ss );
			Synt::WriteFunctionTypeEnding( function.syntax_element->type, ss );
		}
		else
		{
			// Some generated method.
			if( functions_set->base_class != nullptr )
			{
				// Try to find value for this name in the class and extract proper name.
				std::string_view name;
				functions_set->base_class->members->ForEachInThisScope(
					[&]( const std::string_view member_name, const NamesScopeValue& value )
				{
					if( value.value.GetFunctionsSet() == functions_set )
						name= member_name;
				} );

				ss << name;

				// Stringify params list, based on function type.
				ss << "( ";
				for( const FunctionType::Param& param : function.type.params )
				{
					if( function.is_this_call && &param == &function.type.params.front() )
					{
						if( param.value_type == ValueType::Value )
							ss << Keyword( Keywords::byval_ ) << " " << Keyword( Keywords::this_ );
						else
							ss << Keyword( param.value_type == ValueType::ReferenceMut ? Keywords::mut_ : Keywords::imut_ ) << " " << Keyword( Keywords::this_ );
					}
					else
					{
						ss << param.type.ToString() << " ";
						if( param.value_type == ValueType::Value )
						{}
						else if( param.value_type == ValueType::ReferenceMut )
							ss << "&" << Keyword( Keywords::mut_ ) << " ";
						else if( param.value_type == ValueType::ReferenceImut )
							ss << "&" << Keyword( Keywords::imut_ ) << " ";

						ss << "other"; // Give some dummy name to this param.
					}

					if( &param != &function.type.params.back() )
						ss << ", ";
				}
				ss << " ) ";

				if( function.type.unsafe )
					ss << Keyword( Keywords::unsafe_ ) << " ";
				ss << ": " << function.type.return_type.ToString();

				if( function.type.return_value_type == ValueType::Value )
				{}
				else if( function.type.return_value_type == ValueType::ReferenceMut )
					ss << " &" << Keyword( Keywords::mut_ );
				else if( function.type.return_value_type == ValueType::ReferenceImut )
					ss << " &" << Keyword( Keywords::imut_ );
			}
		}

		SignatureHelpItem item;
		item.label= ss.str();
		signature_help_items_.push_back( std::move(item) );
	}

	for( const FunctionTemplatePtr& function_template : functions_set->template_functions )
	{
		if( function_template->syntax_element != nullptr )
		{
			std::stringstream ss;
			Synt::WriteFunctionTemplate( *function_template->syntax_element, ss );

			SignatureHelpItem item;
			item.label= ss.str();
			signature_help_items_.push_back( std::move(item) );
		}
	}
}

void CodeBuilder::DeleteFunctionsBodies_r( NamesScope& names_scope )
{
	names_scope.ForEachValueInThisScope(
		[&]( Value& value )
		{
			if( const OverloadedFunctionsSetPtr functions_set= value.GetFunctionsSet() )
			{
				for( FunctionVariable& function_variable : functions_set->functions )
				{
					if( function_variable.constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete || function_variable.constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete )
						continue; // Preserve bodies of constexpr functions.

					if( function_variable.llvm_function->function != nullptr )
						function_variable.llvm_function->function->deleteBody();
				}
			}
			else if( const NamesScopePtr inner_namespace= value.GetNamespace() )
				DeleteFunctionsBodies_r( *inner_namespace );
			else if( const Type* const type= value.GetTypeName() )
			{
				if( const ClassPtr class_type= type->GetClassType() )
				{
					// Process classes only from parent namespace.
					// Otherwise we can get loop, using type alias.
					if( class_type->members->GetParent() == &names_scope )
						DeleteFunctionsBodies_r( *class_type->members );
				}
			}
		});
}

} // namespace U
