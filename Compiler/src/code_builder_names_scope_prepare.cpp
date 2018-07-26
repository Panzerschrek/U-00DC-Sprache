#include "assert.hpp"
#include "keywords.hpp"
#include "mangling.hpp"
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
			NamesScopeFill( names_scope, *func, nullptr );
		}
		else if( const auto class_= dynamic_cast<const Synt::Class*>( program_element.get() ) )
		{
			NamesScopeFill( names_scope, *class_ );
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
			NamesScopeFill( names_scope, *variables_declaration );
		else if( const auto auto_variable_declaration= dynamic_cast<const Synt::AutoVariableDeclaration*>( program_element.get() ) )
			NamesScopeFill( names_scope, *auto_variable_declaration );
		else if( const auto static_assert_= dynamic_cast<const Synt::StaticAssert*>( program_element.get() ) )
			NamesScopeFill( names_scope, *static_assert_ );
		else if( const auto enum_= dynamic_cast<const Synt::Enum*>( program_element.get() ) )
			NamesScopeFill( names_scope, *enum_ );
		else if( const auto typedef_= dynamic_cast<const Synt::Typedef*>( program_element.get() ) )
		{
			U_UNUSED(typedef_); U_ASSERT(false); // TODO
		}
		else if( const auto type_template= dynamic_cast<const Synt::TypeTemplateBase*>( program_element.get() ) )
			NamesScopeFill( names_scope, *type_template );
		else if( const auto function_template= 	dynamic_cast<const Synt::FunctionTemplate*>( program_element.get() ) )
		{
			NamesScopeFill( names_scope, *function_template, nullptr );
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

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::Function& function_declaration, const ClassProxyPtr base_class, ClassMemberVisibility visibility )
{
	if( function_declaration.name_.components.size() != 1u )
	{
		return; // TODO - process body functions later.
	}

	const ProgramString& func_name= function_declaration.name_.components.back().name;

	if( NamesScope::InsertedName* const prev_name= names_scope.GetThisScopeName( func_name ) )
	{
		if( OverloadedFunctionsSet* const functions_set= prev_name->second.GetFunctionsSet() )
		{
			if( base_class != nullptr && base_class->class_->GetMemberVisibility( func_name ) != visibility )
				errors_.push_back( ReportFunctionsVisibilityMismatch( function_declaration.file_pos_, func_name ) );

			U_ASSERT( functions_set->base_class == base_class );
			functions_set->syntax_elements.push_back( &function_declaration );
		}
		else
			errors_.push_back( ReportRedefinition( function_declaration.file_pos_, func_name ) );
	}
	else
	{
		if( base_class != nullptr )
			base_class->class_->SetMemberVisibility( func_name, visibility );

		OverloadedFunctionsSet functions_set;
		functions_set.base_class= base_class;
		functions_set.syntax_elements.push_back( &function_declaration );

		names_scope.AddName( func_name, Value( std::move(functions_set) ) );
	}
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::FunctionTemplate& function_template_declaration, const ClassProxyPtr base_class )
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
		{
			U_ASSERT( functions_set->base_class == base_class );
			functions_set->template_syntax_elements.push_back( &function_template_declaration );
		}
		else
			errors_.push_back( ReportRedefinition( function_template_declaration.file_pos_, function_template_name ) );
	}
	else
	{
		OverloadedFunctionsSet functions_set;
		functions_set.base_class= base_class;
		functions_set.template_syntax_elements.push_back( &function_template_declaration );

		names_scope.AddName( function_template_name, Value( std::move(functions_set) ) );
	}
}

ClassProxyPtr CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::Class& class_declaration )
{
	const ProgramString& class_name= class_declaration.name_.components.back().name;
	if( IsKeyword( class_name ) )
		errors_.push_back( ReportUsingKeywordAsName( class_declaration.file_pos_ ) );

	if( class_declaration.name_.components.size() != 1u )
	{
		return nullptr; // TODO - disable out-of-line possibility for classes.
	}

	const ClassProxyPtr class_type= std::make_shared<ClassProxy>( new Class( class_name, &names_scope ) );
	Class& the_class= *class_type->class_;
	the_class.body_file_pos= the_class.forward_declaration_file_pos= class_declaration.file_pos_;

	if( names_scope.AddName( class_name, Value( Type( class_type ), class_declaration.file_pos_ ) ) == nullptr )
		errors_.push_back( ReportRedefinition( class_declaration.file_pos_, class_name ) );

	if( class_declaration.is_forward_declaration_ )
		return class_type;

	the_class.syntax_element= &class_declaration;

	the_class.llvm_type= llvm::StructType::create( llvm_context_, MangleType( class_type ) );

	ClassMemberVisibility current_visibility= ClassMemberVisibility::Public;
	for( const Synt::IClassElementPtr& member : class_declaration.elements_ )
	{
		// TODO - process visibility

		if( dynamic_cast<const Synt::ClassField*>( member.get() ) != nullptr )
		{} // Do not place class fields. Any access to class fields must be accesible only for complete classes.
		else if( const auto func= dynamic_cast<const Synt::Function*>( member.get() ) )
			NamesScopeFill( the_class.members, *func, class_type, current_visibility );
		else if( const auto func_template= dynamic_cast<const Synt::FunctionTemplate*>( member.get() ) )
			NamesScopeFill( the_class.members, *func_template, class_type );
		else if( const auto visibility_label=
			dynamic_cast<const Synt::ClassVisibilityLabel*>( member.get() ) )
		{
			if( class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Struct )
				errors_.push_back( ReportVisibilityForStruct( visibility_label->file_pos_, class_name ) );
			current_visibility= visibility_label->visibility_;
		}
		else if( const auto type_template= dynamic_cast<const Synt::TypeTemplateBase*>( member.get() ) )
			NamesScopeFill( the_class.members, *type_template );
		else if( const auto enum_= dynamic_cast<const Synt::Enum*>( member.get() ) )
			NamesScopeFill( the_class.members, *enum_ );
		else if( const auto static_assert_= dynamic_cast<const Synt::StaticAssert*>( member.get() ) )
			NamesScopeFill( the_class.members, *static_assert_ );
		else U_ASSERT(false); // TODO - process another members.
	} // for class elements

	return class_type;
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::TypeTemplateBase& type_template_declaration )
{
	const ProgramString type_template_name= type_template_declaration.name_;
	if( NamesScope::InsertedName* const prev_name= names_scope.GetThisScopeName( type_template_name ) )
	{
		if( TypeTemplatesSet* const type_templates_set= prev_name->second.GetTypeTemplatesSet() )
			type_templates_set->syntax_elements.push_back( &type_template_declaration );
		else
			errors_.push_back( ReportRedefinition( type_template_declaration.file_pos_, type_template_name ) );
	}
	else
	{
		TypeTemplatesSet type_templates_set;
		type_templates_set.syntax_elements.push_back( &type_template_declaration );
		names_scope.AddName( type_template_name, Value( std::move(type_templates_set), type_template_declaration.file_pos_ ) );
	}
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::Enum& enum_declaration )
{
	const EnumPtr enum_= std::make_shared<Enum>( enum_declaration.name, &names_scope );
	enum_->syntax_element= &enum_declaration;

	if( names_scope.AddName( enum_declaration.name, Value( Type( enum_ ), enum_declaration.file_pos_ ) ) == nullptr )
		errors_.push_back( ReportRedefinition( enum_declaration.file_pos_, enum_declaration.name ) );
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::StaticAssert& static_assert_declaration )
{
	StaticAssert static_assert_;
	static_assert_.syntax_element= &static_assert_declaration;

	names_scope.AddName(
		"_sa_"_SpC + ToProgramString( std::to_string(reinterpret_cast<uintptr_t>(&static_assert_declaration)).c_str() ),
		Value( static_assert_, static_assert_declaration.file_pos_ ) );
}

void CodeBuilder::NamesScopeFillOutOfLineElements( NamesScope& names_scope, const Synt::ProgramElements& namespace_elements )
{
	for (const Synt::IProgramElementPtr& program_element : namespace_elements )
	{
		if( const auto func= dynamic_cast<const Synt::Function*>( program_element.get() ) )
		{
			if( func->name_.components.size() != 1u )
			{
				const NamesScope::InsertedName* const func_name= ResolveName( func->file_pos_, names_scope, func->name_, true );
				if( func_name == nullptr || func_name->second.GetFunctionsSet() == nullptr )
				{
					errors_.push_back( ReportFunctionDeclarationOutsideItsScope( func->file_pos_ ) );
					continue;
				}
				const_cast<OverloadedFunctionsSet*>(func_name->second.GetFunctionsSet())->syntax_elements.push_back(func); // TODO - remove const_cast
			}
		}
		else if( const auto namespace_= dynamic_cast<const Synt::Namespace*>( program_element.get() ) )
		{
			if( const NamesScope::InsertedName* const inner_namespace_name= names_scope.GetThisScopeName( namespace_->name_ ) )
			{
				if( const NamesScopePtr inner_namespace= inner_namespace_name->second.GetNamespace() )
					NamesScopeFillOutOfLineElements( *inner_namespace, namespace_->elements_ );
			}
		}
		else if( dynamic_cast<const Synt::Class*>( program_element.get() ) != nullptr )
		{}
		else if( dynamic_cast<const Synt::VariablesDeclaration*>( program_element.get() ) != nullptr )
		{}
		else if( dynamic_cast<const Synt::AutoVariableDeclaration*>( program_element.get() ) != nullptr )
		{}
		else if( dynamic_cast<const Synt::StaticAssert*>( program_element.get() ) != nullptr )
		{}
		else if( dynamic_cast<const Synt::Enum*>( program_element.get() ) != nullptr )
		{
			// SPRACHE_TODO - enable out-of-line enums
		}
		else if( dynamic_cast<const Synt::Typedef*>( program_element.get() ) != nullptr )
		{}
		else if( dynamic_cast<const Synt::TypeTemplateBase*>( program_element.get() ) != nullptr )
		{}
		else if( dynamic_cast<const Synt::FunctionTemplate*>( program_element.get() ) != nullptr )
		{}
		else U_ASSERT(false);
	}
}

} // namespace CodeBuilderPrivate

} // namespace U
