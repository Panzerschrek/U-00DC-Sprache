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
		if( const auto names_scope= name_value_pair.second.value.GetNamespace() )
			DeleteFunctionsBodies_r( *names_scope );

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

	enum_type->underlaying_type= FundamentalType( U_FundamentalType::u8_, fundamental_llvm_types_.u8_ );
	enum_type->element_count= 1;

	const std::string_view member_name= "Member0";
	const auto constexpr_value= llvm::Constant::getNullValue( enum_type->underlaying_type.llvm_type );

	const VariablePtr member_variable=
		std::make_shared<Variable>(
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
						if( const auto type_template_space= BuildTypeTemplateForCompletion( *start_scope, existing_type_template ) )
							return EvaluateCompletionRequestPrefix_r( type_template_space, prefix_tail );
					}
				}
			}
		}

		if( const auto type_template_space= BuildTypeTemplateForCompletion( *start_scope, prepared_type_template ) )
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
	FunctionContext& function_context= *global_function_context_;

	// Complete type name.
	const Type type= PrepareType( variables_declaration.type, names_scope, function_context );

	// Complete names in initializers.
	for( const Synt::VariablesDeclaration::VariableEntry& variable_entry : variables_declaration.variables )
	{
		if( variable_entry.initializer == nullptr )
			continue;

		const VariableMutPtr variable=
			std::make_shared<Variable>(
				type,
				ValueType::Value,
				Variable::Location::Pointer,
				variable_entry.name + " variable itself" );

		function_context.variables_state.AddNode( variable );

		const VariableMutPtr variable_for_initialization=
			std::make_shared<Variable>(
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
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::AutoVariableDeclaration& auto_variable_declaration )
{
	// Complete names in auto-variable expression initializer.
	BuildExpressionCode( auto_variable_declaration.initializer_expression, names_scope, *global_function_context_ );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::StaticAssert& static_assert_ )
{
	// Complete names in static assert expression.
	BuildExpressionCode( static_assert_.expression, names_scope, *global_function_context_ );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::TypeAlias& type_alias )
{
	// Complete names in aliased type name.
	PrepareType( type_alias.value, names_scope, *global_function_context_ );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Enum& enum_ )
{
	// Nothing to complete in enum.
	(void)names_scope;
	(void)enum_;
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Function& function )
{
	NamesScope* actual_nams_scope= nullptr;
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

			actual_nams_scope= nullptr;
			if( const auto namespace_= value->value.GetNamespace() )
				actual_nams_scope= namespace_.get();
			else if( const auto type= value->value.GetTypeName() )
			{
				if( const auto class_= type->GetClassType() )
				{
					actual_nams_scope= class_->members.get();

					if( i + 2 == name.size() )
						base_class= class_;
				}
			}

			if( actual_nams_scope == nullptr )
				return;

			if( name[i + 1].completion_requested )
			{
				NamesScopeFetchComleteForNamesScope( *actual_nams_scope, name[i + 1].name );
				return;
			}

			value= actual_nams_scope->GetThisScopeValue( name[i + 1].name );
		}
	}
	else
	{
		if( name.front().completion_requested )
		{
			NameLookupCompleteImpl( names_scope, name.front().name );
			return;
		}

		// Declaration/definition in current scope.
		actual_nams_scope= &names_scope;

		base_class= names_scope.GetClass();
	}

	OverloadedFunctionsSet functions_set;

	// Consider this not an out-of line definition.
	// There is no reason to set this flag to true, since it affects only some consistency checks.
	const bool out_of_line_flag= false;

	// Prepare function - complete names in types of params and return value.
	const size_t function_index= PrepareFunction( *actual_nams_scope, base_class, functions_set, function, out_of_line_flag );

	if( function_index >= functions_set.functions.size() )
	{
		// Something went wrong.
		return;
	}

	if( function.block == nullptr )
		return; // This is only prototype.

	FunctionVariable& function_variable= functions_set.functions[ function_index ];

	// Build function code - complete names inside its body.
	BuildFuncCode(
		function_variable,
		base_class,
		*actual_nams_scope,
		name.back().name,
		function.type.params,
		*function.block,
		function.constructor_initialization_list.get() );

	// Clear garbage - remove created llvm function.
	if( function_variable.llvm_function->function != nullptr )
		function_variable.llvm_function->function->eraseFromParent();
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Class& class_ )
{
	// Complete names in parent names.
	for( const Synt::ComplexName& parent_name : class_.parents )
		ResolveValue( names_scope, *global_function_context_, parent_name );

	// Complete names in non-sync tag.
	if( const auto non_sync_expression= std::get_if< std::unique_ptr<const Synt::Expression> >( &class_.non_sync_tag ) )
	{
		if( *non_sync_expression != nullptr )
			BuildExpressionCode( **non_sync_expression, names_scope, *global_function_context_ );
	}

	// Do not complete class members, since completion for class member should be triggered instead.
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::TypeTemplate& type_template )
{
	TypeTemplatesSet temp_type_templates_set;
	PrepareTypeTemplate( type_template, temp_type_templates_set, names_scope );
	if( !temp_type_templates_set.type_templates.empty() )
		BuildTypeTemplateForCompletion( names_scope, temp_type_templates_set.type_templates.front() );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::FunctionTemplate& function_template_syntax_element )
{
	// Prepare function template and instatiate it with dummy template args in order to produce some sort of usefull completion result.

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

	const auto template_args_scope= std::make_shared<NamesScope>( "", &names_scope );
	for( const TemplateBase::TemplateParameter& param : result_function_template->template_params )
	{
		template_args_scope->AddName( param.name, NamesScopeValue( Value( GetStubTemplateArgType() ), param.src_loc ) );
		// TODO - support value params.
	}

	const auto errors_container= std::make_shared<CodeBuilderErrorsContainer>();
	template_args_scope->SetErrors( errors_container );

	const auto prev_skip_building_generated_functions= skip_building_generated_functions_;
	skip_building_generated_functions_= false;

	FinishTemplateFunctionGeneration(
		*errors_container,
		function_template_syntax_element.src_loc,
		TemplateFunctionPreparationResult{ result_function_template, template_args_scope } );

	skip_building_generated_functions_= prev_skip_building_generated_functions;
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Namespace& namespace_ )
{
	// Nothing to do here, since completion for namespace has no sense and completion for namespace member will be trigered otherwise.
	(void)names_scope;
	(void)namespace_;
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::ClassField& class_field )
{
	// Complete type name of class field.
	PrepareType( class_field.type, names_scope, *global_function_context_ );
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::ClassVisibilityLabel& class_visibility_label )
{
	// Nothing to complete in class visibility label.
	(void)names_scope;
	(void)class_visibility_label;
}

NamesScopePtr CodeBuilder::BuildTypeTemplateForCompletion( NamesScope& names_scope, const TypeTemplatePtr& type_template )
{
	const auto template_args_scope= std::make_shared<NamesScope>( "", &names_scope );

	TemplateArgs signature_args;

	for( const TemplateBase::TemplateParameter& param : type_template->template_params )
	{
		// TODO - support value params.
		const NamesScopeValue dummy_arg( Value( GetStubTemplateArgType() ), param.src_loc );
		template_args_scope->AddName( param.name, dummy_arg );

		// TODO - prepare signature args properly.
		signature_args.push_back( GetStubTemplateArgType() );
	}

	const auto errors_container= std::make_shared<CodeBuilderErrorsContainer>();
	template_args_scope->SetErrors( errors_container );

	const auto prev_skip_building_generated_functions= skip_building_generated_functions_;
	skip_building_generated_functions_= false;

	const NamesScopeValue* names_scope_value=
		FinishTemplateTypeGeneration( SrcLoc(), names_scope, TemplateTypePreparationResult{ type_template, template_args_scope, signature_args } );

	skip_building_generated_functions_= prev_skip_building_generated_functions;

	if( names_scope_value == nullptr )
		return nullptr;

	if( const auto type= names_scope_value->value.GetTypeName() )
	{
		if( const auto class_type= type->GetClassType() )
		{
			GlobalThingBuildClass( class_type );
			GlobalThingBuildNamespace( *class_type->members );
		}
	}

	return template_args_scope;
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

		CompletionItem item;
		item.name= std::string(value_name);

		// Perform prioritization by prefixing name in sort text.
		// All values names, starting with given text have more priority, than values with name matching given in the middle/at end.
		if( completion_name_ref.empty() || pos == 0 )
			item.sort_text= "0_" + item.name;
		else
			item.sort_text= "1_" + item.name;

		// TODO - fill detail for other kinds of values.

		const Value& value= names_scope_value.value;
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
						ss << Keyword( param.value_type == ValueType::ReferenceMut ? Keywords::mut_ : Keywords::imut_ ) << " " << Keyword( Keywords::this_ );
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
