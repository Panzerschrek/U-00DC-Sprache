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

std::vector<CodeBuilder::CompletionItem> CodeBuilder::Complete( const llvm::ArrayRef<CompletionRequestPrefixComponent> prefix, const Synt::ProgramElement& program_element )
{
	NamesScope* const names_scope= GetNamesScopeForCompletion( prefix );
	if( names_scope == nullptr )
		return {};

	BuildElementForCompletion( *names_scope, program_element );

	return CompletionResultFinalize();
}

std::vector<CodeBuilder::CompletionItem> CodeBuilder::Complete( const llvm::ArrayRef<CompletionRequestPrefixComponent> prefix, const Synt::ClassElement& class_element )
{
	NamesScope* const names_scope= GetNamesScopeForCompletion( prefix );
	if( names_scope == nullptr )
		return {};

	BuildElementForCompletion( *names_scope, class_element );

	return CompletionResultFinalize();
}

SrcLoc CodeBuilder::GetDefinitionFetchSrcLoc( const NamesScopeValue& value )
{
	// Process functions set specially.
	// TODO - maybe perform overloaidng resolution to fetch proper function?
	if( const auto functions_set= value.value.GetFunctionsSet() )
	{
		if( !functions_set->functions.empty() )
			return functions_set->functions.front().body_src_loc;
		if( !functions_set->template_functions.empty() )
			return functions_set->template_functions.front()->src_loc;
		if( !functions_set->syntax_elements.empty() )
			return functions_set->syntax_elements.front()->src_loc_;
		if( !functions_set->out_of_line_syntax_elements.empty() )
			return functions_set->out_of_line_syntax_elements.front()->src_loc_;
	}
	if( const auto type_templates_set= value.value.GetTypeTemplatesSet() )
	{
		if( !type_templates_set->type_templates.empty() )
			return type_templates_set->type_templates.front()->src_loc;
	}

	return value.src_loc;
}

void CodeBuilder::CollectDefinition( const NamesScopeValue& value, const SrcLoc& src_loc )
{
	if( !collect_definition_points_ )
		return;

	// For now enable saving definitions for non-main (imported) files, in order to implement occurences search.

	DefinitionPoint point;
	point.src_loc= GetDefinitionFetchSrcLoc( value );

	if( const auto variable_ptr= value.value.GetVariable() )
	{
		if( variable_ptr != nullptr )
			point.type= variable_ptr->type;
	}
	if( const auto class_field= value.value.GetClassField() )
	{
		if( class_field != nullptr )
			point.type= class_field->type;
	}

	definition_points_.insert( std::make_pair( src_loc, std::move(point) ) );
}

NamesScope* CodeBuilder::GetNamesScopeForCompletion( const llvm::ArrayRef<CompletionRequestPrefixComponent> prefix )
{
	NamesScope& root_names_scope= *compiled_sources_.front().names_map;
	return EvaluateCompletionRequestPrefix_r( root_names_scope, prefix );
}

NamesScope* CodeBuilder::EvaluateCompletionRequestPrefix_r( NamesScope& start_scope, const llvm::ArrayRef<CompletionRequestPrefixComponent> prefix )
{
	if( prefix.empty() )
		return &start_scope;

	const CompletionRequestPrefixComponent& prefix_head= prefix.front();
	const auto prefix_tail= prefix.drop_front();

	if( const auto namespace_= std::get_if<const Synt::Namespace*>( &prefix_head ) )
	{
		if( const NamesScopeValue* const value= start_scope.GetThisScopeValue( (*namespace_)->name_ ) )
		{
			if( const auto names_scope= value->value.GetNamespace() )
				return EvaluateCompletionRequestPrefix_r( *names_scope, prefix_tail );
		}
	}
	else if( const auto class_= std::get_if<const Synt::Class*>( &prefix_head ) )
	{
		if( const NamesScopeValue* const value= start_scope.GetThisScopeValue( (*class_)->name_ ) )
		{
			if( const auto type_name= value->value.GetTypeName() )
			{
				if( const auto class_type= type_name->GetClassType() )
					return EvaluateCompletionRequestPrefix_r( *class_type->members, prefix_tail );
			}
		}
	}
	else if( const auto type_template= std::get_if<const Synt::TypeTemplate*>( &prefix_head ) )
	{
		if( const NamesScopeValue* const value= start_scope.GetThisScopeValue( (*type_template)->name_ ) )
		{
			if( const auto type_templates_set= value->value.GetTypeTemplatesSet() )
			{
				for( const TypeTemplatePtr& type_template_ptr : type_templates_set->type_templates )
				{
					if( type_template_ptr->syntax_element == *type_template )
					{
						// Found this type template.
						// TODO - support completion inside templates.
						return nullptr;
					}
				}
			}
		}
	}
	else U_ASSERT(false);

	return nullptr;
}

std::vector<CodeBuilder::CompletionItem> CodeBuilder::CompletionResultFinalize()
{
	std::vector<CompletionItem> result;
	result.swap( completion_items_ );	
	// Ideally we should filter-out shadowed names, but it is for now too complicated.
	return result;
}

void CodeBuilder::BuildElementForCompletion( NamesScope& names_scope, const Synt::ProgramElement& program_element )
{
	return std::visit( [&](const auto &el){ return BuildElementForCompletionImpl( names_scope, el ); }, program_element );
}

void CodeBuilder::BuildElementForCompletion( NamesScope& names_scope, const Synt::ClassElement& class_element )
{
	return std::visit( [&](const auto &el){ return BuildElementForCompletionImpl( names_scope, el ); }, class_element );
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

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::FunctionPtr& function_ptr )
{
	if( function_ptr == nullptr || function_ptr->name_.empty() )
		return;

	NamesScope* actual_nams_scope= nullptr;
	ClassPtr base_class= nullptr;

	const auto& name= function_ptr->name_;

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
			value= LookupName( names_scope, name[0].name, function_ptr->src_loc_ ).value;
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
	const size_t function_index= PrepareFunction( *actual_nams_scope, base_class, functions_set, *function_ptr, out_of_line_flag );

	if( function_index >= functions_set.functions.size() )
	{
		// Something went wrong.
		return;
	}

	if( function_ptr->block_ == nullptr )
		return; // This is only prototype.

	FunctionVariable& function_variable= functions_set.functions[ function_index ];

	// Build function code - complete names inside its body.
	BuildFuncCode(
		function_variable,
		base_class,
		*actual_nams_scope,
		name.back().name,
		function_ptr->type_.params_,
		*function_ptr->block_,
		function_ptr->constructor_initialization_list_.get() );

	// Clear garbage - remove created llvm function.
	if( function_variable.llvm_function->function != nullptr )
		function_variable.llvm_function->function->eraseFromParent();
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::ClassPtr& class_ptr )
{
	if( class_ptr == nullptr )
		return;

	// Complete names in parent names.
	for( const Synt::ComplexName& parent_name : class_ptr->parents_ )
		PrepareTypeImpl( names_scope, *global_function_context_, parent_name );

	// Complete names in non-sync tag.
	if( const auto non_sync_expression= std::get_if<Synt::ExpressionPtr>( &class_ptr->non_sync_tag_ ) )
	{
		if( *non_sync_expression != nullptr )
			BuildExpressionCode( **non_sync_expression, names_scope, *global_function_context_ );
	}

	// Do not complete class members, since completion for class member should be triggered instead.
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::TypeTemplate& type_template )
{
	// TODO - support completion inside templates.
	(void)names_scope;
	(void)type_template;
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::FunctionTemplate& function_template )
{
	// TODO - support completion inside templates.
	(void)names_scope;
	(void)function_template;
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::NamespacePtr& namespace_ptr )
{
	// Nothing to do here, since completion for namespace has no sense and completion for namespace member will be trigered otherwise.
	(void)names_scope;
	(void)namespace_ptr;
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
		if( value_name_ref.startswith( StringViewToStringRef( Keyword(Keywords::static_assert_) ) ) || // static_assert name may exist isnide namespace, but we should ignore it.
			StringToOverloadedOperator( value_name ) != std::nullopt // Ignore all overloaded operators. There is no reason and no possibility to access overloaded operator by name.
			// Still allow to access constructors and destructors, even if it is needed very rarely.
			)
		{
			return;
		}

		CompletionItem item;
		item.name= std::string(value_name);

		// Perform prioritization by prefixing name in sort text.
		// All values names, starting with given text have more priority, that values with name matching given in the middle/at end.
		if( completion_name_ref.empty() || pos == 0 )
			item.sort_text= "0_" + item.name;
		else
			item.sort_text= "1_" + item.name;

		// TODO - fill detail for other kinds of values.

		const Value& value= names_scope_value.value;
		if( const auto variable= value.GetVariable() )
		{
			item.kind= CompletionItemKind::Variable;

			item.detail= item.name;
			item.detail+= " : ";
			item.detail+= variable->type.ToString();
		}
		else if( value.GetFunctionsSet() != nullptr || value.GetThisOverloadedMethodsSet() != nullptr )
			item.kind= CompletionItemKind::FunctionsSet;
		else if( const auto type= value.GetTypeName() )
		{
			item.kind= CompletionItemKind::Type;

			if( type->GetFundamentalType() == nullptr )
			{
				// Fill detail for types except fundamentals.
				item.detail= item.name;
				item.detail+= " : ";

				// Prefix structs/classes with specific keyword.
				if( const auto class_type= type->GetClassType() )
				{
					item.detail+= Keyword( class_type->kind == Class::Kind::Struct ? Keywords::struct_ : Keywords::class_ );
					item.detail+= " ";
				}

				item.detail+= type->ToString();
			}
		}
		else if( value.GetTypedef() != nullptr )
			item.kind= CompletionItemKind::Type;
		else if( const auto class_field= value.GetClassField() )
		{
			item.kind= CompletionItemKind::ClassField;

			item.detail= item.name;
			item.detail+= " : ";

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

} // namespace U
