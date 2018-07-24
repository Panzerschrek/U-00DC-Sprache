#include "assert.hpp"
#include "keywords.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::ProgramElements& namespace_elements )
{
	for (const Synt::IProgramElementPtr& program_element : namespace_elements )
	{
		if( const auto func= dynamic_cast<const Synt::Function*>( program_element.get() ) )
		{
			NamesScopeFill( names_scope, *func );
		}
		else if( const auto class_= dynamic_cast<const Synt::Class*>( program_element.get() ) )
		{
			U_UNUSED(class_); U_ASSERT(false); // TODO
		}
		else if( const auto namespace_= dynamic_cast<const Synt::Namespace*>( program_element.get() ) )
		{
			NamesScope* result_scope= &names_scope;
			if( const NamesScope::InsertedName* const same_name= names_scope.GetThisScopeName( namespace_->name_ ) )
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

				const auto new_names_scope= std::make_shared<NamesScope>( namespace_->name_, &names_scope );
				names_scope.AddName( namespace_->name_, Value( new_names_scope, namespace_->file_pos_ ) );
				result_scope= new_names_scope.get();
			}

			NamesScopeFill( *result_scope, namespace_->elements_ );
		}
		else if( const auto variables_declaration= dynamic_cast<const Synt::VariablesDeclaration*>( program_element.get() ) )
		{
			NamesScopeFill( names_scope, *variables_declaration );
		}
		else if( const auto auto_variable_declaration= dynamic_cast<const Synt::AutoVariableDeclaration*>( program_element.get() ) )
		{
			NamesScopeFill( names_scope, *auto_variable_declaration );
		}
		else if( const auto static_assert_= dynamic_cast<const Synt::StaticAssert*>( program_element.get() ) )
		{
			U_UNUSED(static_assert_); U_ASSERT(false); // TODO
		}
		else if( const auto enum_= dynamic_cast<const Synt::Enum*>( program_element.get() ) )
		{
			U_UNUSED(enum_); U_ASSERT(false); // TODO
		}
		else if( const auto typedef_= dynamic_cast<const Synt::Typedef*>( program_element.get() ) )
		{
			U_UNUSED(typedef_); U_ASSERT(false); // TODO
		}
		else if( const auto type_template= dynamic_cast<const Synt::TypeTemplateBase*>( program_element.get() ) )
		{
			U_UNUSED(type_template); U_ASSERT(false); // TODO
		}
		else if( const auto function_template= 	dynamic_cast<const Synt::FunctionTemplate*>( program_element.get() ) )
		{
			NamesScopeFill( names_scope, *function_template );
		}
		else U_ASSERT(false);
	}
}

void  CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::VariablesDeclaration& variables_declaration )
{
	for( const Synt::VariablesDeclaration::VariableEntry& variable_declaration : variables_declaration.variables )
	{
		if( IsKeyword( variable_declaration.name ) )
			errors_.push_back( ReportUsingKeywordAsName( variable_declaration.file_pos ) );

		const auto stored_variable= std::make_shared<StoredVariable>( variable_declaration.name );
		const NamesScope::InsertedName* const inserted_variable= names_scope.AddName( variable_declaration.name, Value( stored_variable, variable_declaration.file_pos ) );
		if( inserted_variable == nullptr )
			errors_.push_back( ReportRedefinition( variable_declaration.file_pos, variable_declaration.name ) );

		// TODO - add syntax elements for later variable initialization.
	}
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::AutoVariableDeclaration& variable_declaration )
{
	if( IsKeyword( variable_declaration.name ) )
		errors_.push_back( ReportUsingKeywordAsName( variable_declaration.file_pos_ ) );

	const auto stored_variable= std::make_shared<StoredVariable>( variable_declaration.name );
	const NamesScope::InsertedName* const inserted_variable= names_scope.AddName( variable_declaration.name, Value( stored_variable, variable_declaration.file_pos_ ) );
	if( inserted_variable == nullptr )
		errors_.push_back( ReportRedefinition( variable_declaration.file_pos_, variable_declaration.name ) );

	// TODO - add syntax elements for later variable initialization.
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::Function& function_declaration )
{
	if( function_declaration.name_.components.size() != 1u )
	{
		return; // TODO - process body functions later.
	}

	const ProgramString& func_name= function_declaration.name_.components.back().name;

	if( NamesScope::InsertedName* const prev_name= names_scope.GetThisScopeName( func_name ) )
	{
		if( OverloadedFunctionsSet* const functions_set= prev_name->second.GetFunctionsSet() )
			functions_set->syntax_elements.push_back( &function_declaration );
		else
			errors_.push_back( ReportRedefinition( function_declaration.file_pos_, func_name ) );
	}
	else
	{
		OverloadedFunctionsSet functions_set;
		functions_set.syntax_elements.push_back( &function_declaration );
		names_scope.AddName( func_name, Value( std::move(functions_set) ) );
	}
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::FunctionTemplate& function_template_declaration )
{
	const Synt::ComplexName& complex_name = function_template_declaration.function_->name_;
	const ProgramString& function_template_name= complex_name.components.front().name;

	if( complex_name.components.size() > 1u )
		errors_.push_back( ReportFunctionDeclarationOutsideItsScope( function_template_declaration.file_pos_ ) );
	if( complex_name.components.front().have_template_parameters )
		errors_.push_back( ReportValueIsNotTemplate( function_template_declaration.file_pos_ ) );

	if( NamesScope::InsertedName* const prev_name= names_scope.GetThisScopeName( function_template_name ) )
	{
		if( OverloadedFunctionsSet* const functions_set= prev_name->second.GetFunctionsSet() )
			functions_set->template_syntax_elements.push_back( &function_template_declaration );
		else
			errors_.push_back( ReportRedefinition( function_template_declaration.file_pos_, function_template_name ) );
	}
	else
	{
		OverloadedFunctionsSet functions_set;
		functions_set.template_syntax_elements.push_back( &function_template_declaration );
		names_scope.AddName( function_template_name, Value( std::move(functions_set) ) );
	}
}

} // namespace CodeBuilderPrivate

} // namespace U
