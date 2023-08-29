#include <sstream>
#include "keywords.hpp"
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

std::vector<std::string> CodeBuilder::Complete( const llvm::ArrayRef<CompletionRequestPrefixComponent> prefix, const Synt::ProgramElement& program_element )
{
	NamesScope& root_names_scope= *compiled_sources_.front().names_map;

	NamesScope* const names_scope= EvaluateCompletionRequestPrefix( root_names_scope, prefix );
	if( names_scope == nullptr )
		return {};

	completion_items_.clear();

	BuildElementForCompletion( *names_scope, program_element );

	std::vector<std::string> result;
	result.swap( completion_items_ );
	std::sort( result.begin(), result.end() );
	result.erase( std::unique( result.begin(), result.end() ), result.end() );
	return result;
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

NamesScope* CodeBuilder::EvaluateCompletionRequestPrefix( NamesScope& start_scope, const llvm::ArrayRef<CompletionRequestPrefixComponent> prefix )
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
				return EvaluateCompletionRequestPrefix( *names_scope, prefix_tail );
		}
	}
	else if( const auto class_= std::get_if<const Synt::Class*>( &prefix_head ) )
	{
		if( const NamesScopeValue* const value= start_scope.GetThisScopeValue( (*class_)->name_ ) )
		{
			if( const auto type_name= value->value.GetTypeName() )
			{
				if( const auto class_type= type_name->GetClassType() )
					return EvaluateCompletionRequestPrefix( *class_type->members, prefix_tail );
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
						// TODO - instantiate type template with dummy ags and search something inside it.
						return nullptr;
					}
				}
			}
		}
	}
	else U_ASSERT(false);

	return nullptr;
}

void CodeBuilder::BuildElementForCompletion( NamesScope& names_scope, const Synt::ProgramElement& program_element )
{
	return std::visit( [&](const auto &el){ return BuildElementForCompletionImpl( names_scope, el ); }, program_element );
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

	OverloadedFunctionsSet functions_set;

	const ClassPtr base_class= nullptr; // TODO - pass it.
	const bool is_out_of_line_function= false; // TODO - set it.

	// Prepare function - complete names in types of params and return value.
	const size_t function_index= PrepareFunction( names_scope, base_class, functions_set, *function_ptr, is_out_of_line_function );

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
		names_scope,
		function_ptr->name_.back().name,
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

	// Complete names in parent mames.
	for( const Synt::ComplexName& parent_name : class_ptr->parents_ )
		PrepareTypeImpl( names_scope, *global_function_context_, parent_name );

	// Do not complete class members, since completion for class member should be triggered instead.
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::TypeTemplate& type_template )
{
	// TODO
	(void)names_scope;
	(void)type_template;
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::FunctionTemplate& function_template )
{
	// TODO
	(void)names_scope;
	(void)function_template;
}

void CodeBuilder::BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::NamespacePtr& namespace_ptr )
{
	// Nothing to do here, since completion for namespace has no sense and completion for namespace member will be trigered otherwise.
	(void)names_scope;
	(void)namespace_ptr;
}

void CodeBuilder::NameLookupCompleteImpl( const NamesScope& names_scope, const std::string_view name )
{
	const NamesScope* current_scope= &names_scope;
	do
	{
		current_scope->ForEachInThisScope(
			[&]( const std::string_view namespace_name, const NamesScopeValue& value )
			{
				(void)value;
				if( namespace_name.size() >= name.size() && namespace_name.substr(0, name.size() ) == name )
					completion_items_.push_back( std::string(namespace_name) );
			});

		current_scope= current_scope->GetParent();
	} while( current_scope != nullptr );
}

} // namespace U
