#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "mangling.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void CodeBuilder::NamesScopeFill(
	const Synt::ProgramElements& namespace_elements,
	NamesScope& names_scope )
{
	for( const Synt::ProgramElement& program_element : namespace_elements )
	{
		std::visit(
			[&]( const auto& t )
			{
				NamesScopeFill( t, names_scope );
			},
			program_element );
	}
}

void CodeBuilder::NamesScopeFill(
	const Synt::NamespacePtr& namespace_,
	NamesScope& names_scope )
{
	NamesScope* result_scope= &names_scope;
	if( const Value* const same_value= names_scope.GetThisScopeValue( namespace_->name_ ) )
	{
		if( const NamesScopePtr same_namespace= same_value->GetNamespace() )
			result_scope= same_namespace.get(); // Extend existend namespace.
		else
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), namespace_->file_pos_, namespace_->name_ );
	}
	else
	{
		if( IsKeyword( namespace_->name_ ) )
			REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), namespace_->file_pos_ );
		U_ASSERT( !NameShadowsTemplateArgument( namespace_->name_, names_scope ) ); // There are no templates abowe namespace. Namespaces inside classes does not exists.

		const auto new_names_scope= std::make_shared<NamesScope>( namespace_->name_, &names_scope );
		names_scope.AddName( namespace_->name_, Value( new_names_scope, namespace_->file_pos_ ) );
		result_scope= new_names_scope.get();
	}

	NamesScopeFill( namespace_->elements_, *result_scope );
}

void CodeBuilder::NamesScopeFill(
	const Synt::VariablesDeclaration& variables_declaration,
	NamesScope& names_scope )
{
	for( const Synt::VariablesDeclaration::VariableEntry& variable_declaration : variables_declaration.variables )
	{
		if( IsKeyword( variable_declaration.name ) )
			REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), variable_declaration.file_pos );
		if( NameShadowsTemplateArgument( variable_declaration.name, names_scope ) )
			REPORT_ERROR( DeclarationShadowsTemplateArgument, names_scope.GetErrors(), variable_declaration.file_pos, variable_declaration.name );

		IncompleteGlobalVariable incomplete_global_variable;
		incomplete_global_variable.variables_declaration= &variables_declaration;
		incomplete_global_variable.element_index= size_t( &variable_declaration - variables_declaration.variables.data() );
		incomplete_global_variable.name= variable_declaration.name;

		if( names_scope.AddName( variable_declaration.name, Value( incomplete_global_variable, variable_declaration.file_pos ) ) == nullptr )
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), variable_declaration.file_pos, variable_declaration.name );
	}
}

void CodeBuilder::NamesScopeFill(
	const Synt::AutoVariableDeclaration& variable_declaration,
	NamesScope& names_scope )
{
	if( IsKeyword( variable_declaration.name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), variable_declaration.file_pos_ );
	if( NameShadowsTemplateArgument( variable_declaration.name, names_scope ) )
		REPORT_ERROR( DeclarationShadowsTemplateArgument, names_scope.GetErrors(), variable_declaration.file_pos_, variable_declaration.name );

	IncompleteGlobalVariable incomplete_global_variable;
	incomplete_global_variable.auto_variable_declaration= &variable_declaration;
	incomplete_global_variable.name= variable_declaration.name;

	if( names_scope.AddName( variable_declaration.name, Value( incomplete_global_variable, variable_declaration.file_pos_ ) ) == nullptr )
		REPORT_ERROR( Redefinition, names_scope.GetErrors(), variable_declaration.file_pos_, variable_declaration.name );
}

void CodeBuilder::NamesScopeFill(
	const Synt::FunctionPtr& function_declaration_ptr,
	NamesScope& names_scope,
	const ClassProxyPtr& base_class,
	const ClassMemberVisibility visibility )
{
	const auto& function_declaration= *function_declaration_ptr;
	
	if( function_declaration.name_.components.size() != 1u )
		return; // process out of line functions later.

	const ProgramString& func_name= function_declaration.name_.components.back().name;
	if( IsKeyword( func_name ) && func_name != Keywords::constructor_ && func_name != Keywords::destructor_ )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), function_declaration.file_pos_ );
	if( NameShadowsTemplateArgument( func_name, names_scope ) )
		REPORT_ERROR( DeclarationShadowsTemplateArgument, names_scope.GetErrors(), function_declaration.file_pos_, func_name );

	if( Value* const prev_value= names_scope.GetThisScopeValue( func_name ) )
	{
		if( OverloadedFunctionsSet* const functions_set= prev_value->GetFunctionsSet() )
		{
			if( base_class != nullptr && base_class->class_->GetMemberVisibility( func_name ) != visibility )
				REPORT_ERROR( FunctionsVisibilityMismatch, names_scope.GetErrors(), function_declaration.file_pos_, func_name );

			U_ASSERT( functions_set->base_class == base_class );
			functions_set->syntax_elements.push_back( &function_declaration );
		}
		else
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), function_declaration.file_pos_, func_name );
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

void CodeBuilder::NamesScopeFill(
	const Synt::FunctionTemplate& function_template_declaration,
	NamesScope& names_scope,
	const ClassProxyPtr& base_class,
	const ClassMemberVisibility visibility )
{
	const Synt::ComplexName& complex_name = function_template_declaration.function_->name_;
	const ProgramString& function_template_name= complex_name.components.front().name;

	if( complex_name.components.size() > 1u )
		REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), function_template_declaration.file_pos_ );
	if( complex_name.components.front().have_template_parameters )
		REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), function_template_declaration.file_pos_ );
	if( IsKeyword( function_template_name ) && function_template_name != Keywords::constructor_ && function_template_name != Keywords::destructor_ )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), function_template_declaration.file_pos_ );
	if( NameShadowsTemplateArgument( function_template_name, names_scope ) )
		REPORT_ERROR( DeclarationShadowsTemplateArgument, names_scope.GetErrors(), function_template_declaration.file_pos_, function_template_name );

	if( Value* const prev_value= names_scope.GetThisScopeValue( function_template_name ) )
	{
		if( OverloadedFunctionsSet* const functions_set= prev_value->GetFunctionsSet() )
		{
			if( base_class != nullptr && base_class->class_->GetMemberVisibility( function_template_name ) != visibility )
				REPORT_ERROR( FunctionsVisibilityMismatch, names_scope.GetErrors(), function_template_declaration.file_pos_, function_template_name );

			U_ASSERT( functions_set->base_class == base_class );
			functions_set->template_syntax_elements.push_back( &function_template_declaration );
		}
		else
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), function_template_declaration.file_pos_, function_template_name );
	}
	else
	{
		if( base_class != nullptr )
			base_class->class_->SetMemberVisibility( function_template_name, visibility );

		OverloadedFunctionsSet functions_set;
		functions_set.base_class= base_class;
		functions_set.template_syntax_elements.push_back( &function_template_declaration );

		names_scope.AddName( function_template_name, Value( std::move(functions_set) ) );
	}
}

ClassProxyPtr CodeBuilder::NamesScopeFill(
	const Synt::ClassPtr& class_declaration_ptr,
	NamesScope& names_scope,
	const ProgramString& override_name )
{
	const auto& class_declaration= *class_declaration_ptr;

	const ProgramString& class_name= override_name.empty() ? class_declaration.name_: override_name;
	if( IsKeyword( class_name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), class_declaration.file_pos_ );
	if( NameShadowsTemplateArgument( class_name, names_scope ) )
		REPORT_ERROR( DeclarationShadowsTemplateArgument, names_scope.GetErrors(), class_declaration.file_pos_, class_name );

	ClassProxyPtr class_type;
	if( const Value* const prev_value= names_scope.GetThisScopeValue( class_name ) )
	{
		if( const Type* const type= prev_value->GetTypeName() )
		{
			if( const ClassProxyPtr prev_class_type= type->GetClassTypeProxy() )
			{
				class_type= prev_class_type;
				if(  class_type->class_->syntax_element->is_forward_declaration_ &&  class_declaration.is_forward_declaration_ )
					REPORT_ERROR( Redefinition, names_scope.GetErrors(), class_declaration.file_pos_, class_name );
				if( !class_type->class_->syntax_element->is_forward_declaration_ && !class_declaration.is_forward_declaration_ )
					REPORT_ERROR( ClassBodyDuplication, names_scope.GetErrors(), class_declaration.file_pos_ );
			}
			else
			{
				REPORT_ERROR( Redefinition, names_scope.GetErrors(), class_declaration.file_pos_, class_name );
				return nullptr;
			}
		}
		else
		{
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), class_declaration.file_pos_, class_name );
			return nullptr;
		}
	}
	else
	{
		class_type= std::make_shared<ClassProxy>();
		(*current_class_table_)[ class_type ].reset( new Class( class_name, &names_scope ) );
		class_type->class_= (*current_class_table_)[ class_type ].get();

		names_scope.AddName( class_name, Value( Type( class_type ), class_declaration.file_pos_ ) );
		class_type->class_->syntax_element= &class_declaration;
		class_type->class_->body_file_pos= class_type->class_->forward_declaration_file_pos= class_declaration.file_pos_;
		class_type->class_->llvm_type= llvm::StructType::create( llvm_context_, MangleType( class_type ) );

		class_type->class_->members.AddAccessRightsFor( class_type, ClassMemberVisibility::Private );
	}

	Class& the_class= *class_type->class_;

	if( class_declaration.is_forward_declaration_ )
		the_class.forward_declaration_file_pos= class_declaration.file_pos_;
	else
	{
		the_class.body_file_pos= class_declaration.file_pos_;
		the_class.syntax_element= &class_declaration;
	}

	if( !class_declaration.is_forward_declaration_ )
	{
		struct Visitor final
		{
			CodeBuilder& this_;
			const Synt::Class& class_declaration;
			ClassProxyPtr& class_type;
			Class& the_class;
			const ProgramString& class_name;
			ClassMemberVisibility current_visibility= ClassMemberVisibility::Public;
			unsigned int field_number= 0u;

			Visitor( CodeBuilder& in_this, const Synt::Class& in_class_declaration, ClassProxyPtr& in_class_type, Class& in_the_class, const ProgramString& in_class_name )
				: this_(in_this), class_declaration(in_class_declaration), class_type(in_class_type), the_class(in_the_class), class_name(in_class_name)
			{}

			void operator()( const Synt::ClassField& in_class_field )
			{
				ClassField class_field;
				class_field.syntax_element= &in_class_field;
				class_field.original_index= field_number;

				if( IsKeyword( in_class_field.name ) )
					REPORT_ERROR( UsingKeywordAsName, the_class.members.GetErrors(), class_declaration.file_pos_ );
				if( this_.NameShadowsTemplateArgument( in_class_field.name, the_class.members ) )
					REPORT_ERROR( DeclarationShadowsTemplateArgument, the_class.members.GetErrors(), in_class_field.file_pos_, in_class_field.name );
				if( the_class.members.AddName( in_class_field.name, Value( class_field, in_class_field.file_pos_ ) ) == nullptr )
					REPORT_ERROR( Redefinition, the_class.members.GetErrors(), in_class_field.file_pos_, in_class_field.name );

				++field_number;
				the_class.SetMemberVisibility( in_class_field.name, current_visibility );
			}
			void operator()( const Synt::FunctionPtr& func )
			{
				this_.NamesScopeFill( func, the_class.members, class_type, current_visibility );
			}
			void operator()( const Synt::FunctionTemplate& func_template )
			{
				this_.NamesScopeFill( func_template, the_class.members, class_type, current_visibility );
			}
			void operator()( const Synt::ClassVisibilityLabel& visibility_label )
			{
				if( class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Struct )
					REPORT_ERROR( VisibilityForStruct, the_class.members.GetErrors(), visibility_label.file_pos_, class_name );
				current_visibility= visibility_label.visibility_;
			}
			void operator()( const Synt::TypeTemplateBase& type_template )
			{
				this_.NamesScopeFill( type_template, the_class.members, class_type, current_visibility );
			}
			void operator()( const Synt::Enum& enum_ )
			{
				this_.NamesScopeFill( enum_, the_class.members );
				the_class.SetMemberVisibility( enum_.name, current_visibility );
			}
			void operator()( const Synt::StaticAssert& static_assert_ )
			{
				this_.NamesScopeFill( static_assert_, the_class.members );
			}
			void operator()( const Synt::Typedef& typedef_ )
			{
				this_.NamesScopeFill( typedef_, the_class.members );
				the_class.SetMemberVisibility( typedef_.name, current_visibility );
			}
			void operator()( const Synt::VariablesDeclaration& variables_declaration )
			{
				this_.NamesScopeFill( variables_declaration, the_class.members );
				for( const auto& variable_declaration : variables_declaration.variables )
					the_class.SetMemberVisibility( variable_declaration.name, current_visibility );
			}
			void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
			{
				this_.NamesScopeFill( auto_variable_declaration, the_class.members );
				the_class.SetMemberVisibility( auto_variable_declaration.name, current_visibility );
			}
			void operator()( const Synt::ClassPtr& inner_class )
			{
				this_.NamesScopeFill( inner_class, the_class.members );
				the_class.SetMemberVisibility( inner_class->name_, current_visibility );
			}
		};

		Visitor visitor( *this, class_declaration, class_type, the_class, class_name );
		for( const Synt::ClassElement& class_element : class_declaration.elements_ )
			std::visit( visitor, class_element );
	}

	return class_type;
}

void CodeBuilder::NamesScopeFill(
	const Synt::TypeTemplateBase& type_template_declaration,
	NamesScope& names_scope,
	const ClassProxyPtr& base_class,
	const ClassMemberVisibility visibility )
{
	const ProgramString type_template_name= type_template_declaration.name_;
	if( IsKeyword( type_template_name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), type_template_declaration.file_pos_ );
	if( NameShadowsTemplateArgument( type_template_name, names_scope ) )
		REPORT_ERROR( DeclarationShadowsTemplateArgument, names_scope.GetErrors(), type_template_declaration.file_pos_, type_template_name );

	if( Value* const prev_value= names_scope.GetThisScopeValue( type_template_name ) )
	{
		if( base_class != nullptr && base_class->class_->GetMemberVisibility( type_template_name ) != visibility )
			REPORT_ERROR( TypeTemplatesVisibilityMismatch, names_scope.GetErrors(), type_template_declaration.file_pos_, type_template_name ); // TODO - use separate error code

		if( TypeTemplatesSet* const type_templates_set= prev_value->GetTypeTemplatesSet() )
			type_templates_set->syntax_elements.push_back( &type_template_declaration );
		else
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), type_template_declaration.file_pos_, type_template_name );
	}
	else
	{
		if( base_class != nullptr )
			base_class->class_->SetMemberVisibility( type_template_name, visibility );

		TypeTemplatesSet type_templates_set;
		type_templates_set.syntax_elements.push_back( &type_template_declaration );
		names_scope.AddName( type_template_name, Value( std::move(type_templates_set), type_template_declaration.file_pos_ ) );
	}
}

void CodeBuilder::NamesScopeFill(
	const Synt::Enum& enum_declaration,
	NamesScope& names_scope )
{
	if( IsKeyword( enum_declaration.name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), enum_declaration.file_pos_ );
	if( NameShadowsTemplateArgument( enum_declaration.name, names_scope ) )
		REPORT_ERROR( DeclarationShadowsTemplateArgument, names_scope.GetErrors(), enum_declaration.file_pos_, enum_declaration.name );

	enums_table_.emplace_back( new Enum( enum_declaration.name, &names_scope ) );
	const EnumPtr enum_= enums_table_.back().get();

	enum_->syntax_element= &enum_declaration;

	if( names_scope.AddName( enum_declaration.name, Value( Type( enum_ ), enum_declaration.file_pos_ ) ) == nullptr )
		REPORT_ERROR( Redefinition, names_scope.GetErrors(), enum_declaration.file_pos_, enum_declaration.name );
}

void CodeBuilder::NamesScopeFill(
	const Synt::Typedef& typedef_declaration,
	NamesScope& names_scope )
{
	if( IsKeyword( typedef_declaration.name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), typedef_declaration.file_pos_ );
	if( NameShadowsTemplateArgument( typedef_declaration.name, names_scope ) )
		REPORT_ERROR( DeclarationShadowsTemplateArgument, names_scope.GetErrors(), typedef_declaration.file_pos_, typedef_declaration.name );

	Typedef typedef_;
	typedef_.syntax_element= &typedef_declaration;

	if( names_scope.AddName( typedef_declaration.name, Value( typedef_, typedef_declaration.file_pos_ ) ) == nullptr )
		REPORT_ERROR( Redefinition, names_scope.GetErrors(), typedef_declaration.file_pos_, typedef_declaration.name );
}

void CodeBuilder::NamesScopeFill(
	const Synt::StaticAssert& static_assert_declaration,
	NamesScope& names_scope )
{
	StaticAssert static_assert_;
	static_assert_.syntax_element= &static_assert_declaration;

	names_scope.AddName(
		"_sa_"_SpC + ToProgramString( std::to_string(reinterpret_cast<uintptr_t>(&static_assert_declaration)) ),
		Value( static_assert_, static_assert_declaration.file_pos_ ) );
}

void CodeBuilder::NamesScopeFillOutOfLineElements(
	const Synt::ProgramElements& namespace_elements,
	NamesScope& names_scope )
{
	for (const Synt::ProgramElement& program_element : namespace_elements )
	{
		if( const auto func_ptr= std::get_if<Synt::FunctionPtr>( &program_element ) )
		{
			const Synt::Function& func= **func_ptr;
			if( func.name_.components.size() != 1u )
			{
				Value* const func_value= ResolveValue( func.file_pos_, names_scope, func.name_, ResolveMode::ForDeclaration );
				if( func_value == nullptr || func_value->GetFunctionsSet() == nullptr )
				{
					REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), func.file_pos_ );
					continue;
				}
				func_value->GetFunctionsSet()->out_of_line_syntax_elements.push_back(&func);
			}
		}
		else if( const auto namespace_ptr= std::get_if<Synt::NamespacePtr>( &program_element ) )
		{
			const Synt::Namespace& namespace_= **namespace_ptr;
			if( const Value* const inner_namespace_value= names_scope.GetThisScopeValue( namespace_.name_ ) )
			{
				if( const NamesScopePtr inner_namespace= inner_namespace_value->GetNamespace() )
					NamesScopeFillOutOfLineElements( namespace_.elements_, *inner_namespace );
			}
		}
	}
}

} // namespace CodeBuilderPrivate

} // namespace U
