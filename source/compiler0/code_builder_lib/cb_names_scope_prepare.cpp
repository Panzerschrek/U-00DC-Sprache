#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::ProgramElementsList& namespace_elements )
{
	namespace_elements.Iter( [&]( const auto& el ) { NamesScopeFill( names_scope, el ); } );
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::Namespace& namespace_  )
{
	NamesScope* result_scope= &names_scope;
	if( const NamesScopeValue* const same_value= names_scope.GetThisScopeValue( namespace_.name ) )
	{
		if( const NamesScopePtr same_namespace= same_value->value.GetNamespace() )
			result_scope= same_namespace.get(); // Extend existend namespace.
		else
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), namespace_.src_loc, namespace_.name );
	}
	else
	{
		if( IsKeyword( namespace_.name ) )
			REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), namespace_.src_loc );

		const auto new_names_scope= std::make_shared<NamesScope>( namespace_.name, &names_scope );
		names_scope.AddName( namespace_.name, NamesScopeValue( new_names_scope, namespace_.src_loc ) );
		result_scope= new_names_scope.get();
	}

	NamesScopeFill( *result_scope, namespace_.elements );
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::VariablesDeclaration& variables_declaration )
{
	for( const Synt::VariablesDeclaration::VariableEntry& variable_declaration : variables_declaration.variables )
	{
		if( IsKeyword( variable_declaration.name ) )
			REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), variable_declaration.src_loc );

		IncompleteGlobalVariable incomplete_global_variable;
		incomplete_global_variable.variables_declaration= &variables_declaration;
		incomplete_global_variable.element_index= size_t( &variable_declaration - variables_declaration.variables.data() );

		if( names_scope.AddName( variable_declaration.name, NamesScopeValue( incomplete_global_variable, variable_declaration.src_loc ) ) == nullptr )
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), variable_declaration.src_loc, variable_declaration.name );
	}
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::AutoVariableDeclaration& variable_declaration )
{
	if( IsKeyword( variable_declaration.name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), variable_declaration.src_loc );

	IncompleteGlobalVariable incomplete_global_variable;
	incomplete_global_variable.auto_variable_declaration= &variable_declaration;

	if( names_scope.AddName( variable_declaration.name, NamesScopeValue( incomplete_global_variable, variable_declaration.src_loc ) ) == nullptr )
		REPORT_ERROR( Redefinition, names_scope.GetErrors(), variable_declaration.src_loc, variable_declaration.name );
}

void CodeBuilder::NamesScopeFill(
	NamesScope& names_scope,
	const Synt::Function& function_declaration,
	const ClassPtr base_class,
	const ClassMemberVisibility visibility )
{
	if( function_declaration.name.size() != 1u )
		return; // process out of line functions later.

	const std::string& func_name= function_declaration.name.back().name;
	if( IsKeyword( func_name ) && func_name != Keywords::constructor_ && func_name != Keywords::destructor_ )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), function_declaration.src_loc );

	if( visibility != ClassMemberVisibility::Public )
	{
		if( func_name == Keywords::constructor_ ||
			func_name == Keywords::destructor_ ||
			func_name == OverloadedOperatorToString( OverloadedOperator::Assign ) ||
			func_name == OverloadedOperatorToString( OverloadedOperator::CompareEqual ) ||
			func_name == OverloadedOperatorToString( OverloadedOperator::CompareOrder ) )
			REPORT_ERROR( ThisMethodMustBePublic, names_scope.GetErrors(), function_declaration.src_loc, func_name );
	}

	if( NamesScopeValue* const prev_value= names_scope.GetThisScopeValue( func_name ) )
	{
		if( const OverloadedFunctionsSetPtr functions_set= prev_value->value.GetFunctionsSet() )
		{
			if( base_class != nullptr && base_class->GetMemberVisibility( func_name ) != visibility )
				REPORT_ERROR( FunctionsVisibilityMismatch, names_scope.GetErrors(), function_declaration.src_loc, func_name );

			U_ASSERT( functions_set->base_class == base_class );
			functions_set->syntax_elements.push_back( &function_declaration );
		}
		else
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), function_declaration.src_loc, func_name );
	}
	else
	{
		if( base_class != nullptr )
			base_class->SetMemberVisibility( func_name, visibility );

		OverloadedFunctionsSet functions_set;
		functions_set.base_class= base_class;
		functions_set.syntax_elements.push_back( &function_declaration );

		names_scope.AddName( func_name, NamesScopeValue( std::make_shared<OverloadedFunctionsSet>( std::move(functions_set) ), SrcLoc() ) );
	}
}

void CodeBuilder::NamesScopeFill(
	NamesScope& names_scope,
	const Synt::FunctionTemplate& function_template_declaration,
	const ClassPtr base_class,
	const ClassMemberVisibility visibility )
{
	const auto& full_name = function_template_declaration.function->name;
	const std::string& function_template_name= full_name.front().name;

	if( full_name.size() > 1u )
		REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), function_template_declaration.src_loc );
	if( IsKeyword( function_template_name ) && function_template_name != Keywords::constructor_ && function_template_name != Keywords::destructor_ )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), function_template_declaration.src_loc );

	if( NamesScopeValue* const prev_value= names_scope.GetThisScopeValue( function_template_name ) )
	{
		if( const OverloadedFunctionsSetPtr functions_set= prev_value->value.GetFunctionsSet() )
		{
			if( base_class != nullptr && base_class->GetMemberVisibility( function_template_name ) != visibility )
				REPORT_ERROR( FunctionsVisibilityMismatch, names_scope.GetErrors(), function_template_declaration.src_loc, function_template_name );

			U_ASSERT( functions_set->base_class == base_class );
			functions_set->template_syntax_elements.push_back( &function_template_declaration );
		}
		else
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), function_template_declaration.src_loc, function_template_name );
	}
	else
	{
		if( base_class != nullptr )
			base_class->SetMemberVisibility( function_template_name, visibility );

		OverloadedFunctionsSet functions_set;
		functions_set.base_class= base_class;
		functions_set.template_syntax_elements.push_back( &function_template_declaration );

		names_scope.AddName( function_template_name, NamesScopeValue( std::make_shared<OverloadedFunctionsSet>( std::move(functions_set) ), SrcLoc() ) );
	}
}

ClassPtr CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::Class& class_declaration, std::optional<Class::BaseTemplate> base_template )
{
	const std::string& class_name= class_declaration.name;
	if( IsKeyword( class_name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), class_declaration.src_loc );

	if( names_scope.GetThisScopeValue( class_name ) != nullptr )
	{
		REPORT_ERROR( Redefinition, names_scope.GetErrors(), class_declaration.src_loc, class_name );
		return nullptr;
	}

	auto class_type_ptr= std::make_unique<Class>( class_name, &names_scope );
	const ClassPtr class_type= class_type_ptr.get();
	classes_table_.push_back( std::move(class_type_ptr) );

	names_scope.AddName( class_name, NamesScopeValue( Type( class_type ), class_declaration.src_loc ) );

	class_type->syntax_element= &class_declaration;
	class_type->src_loc= class_declaration.src_loc;
	if( base_template != std::nullopt )
		class_type->generated_class_data= std::move(*base_template);

	class_type->llvm_type= llvm::StructType::create( llvm_context_, mangler_->MangleType( class_type ) );

	class_type->members->AddAccessRightsFor( class_type, ClassMemberVisibility::Private );
	class_type->members->SetClass( class_type );

	struct Visitor final
	{
		CodeBuilder& this_;
		const Synt::Class& class_declaration;
		ClassPtr class_type;
		const std::string_view class_name;
		ClassMemberVisibility current_visibility= ClassMemberVisibility::Public;
		uint32_t field_number= 0u;

		Visitor( CodeBuilder& in_this, const Synt::Class& in_class_declaration, ClassPtr in_class_type, const std::string_view in_class_name )
			: this_(in_this), class_declaration(in_class_declaration), class_type(in_class_type), class_name(in_class_name)
		{}

		void operator()( const Synt::ClassField& in_class_field )
		{
			ClassField class_field;
			class_field.syntax_element= &in_class_field;
			class_field.original_index= field_number;
			class_field.name= in_class_field.name;

			if( IsKeyword( in_class_field.name ) )
				REPORT_ERROR( UsingKeywordAsName, class_type->members->GetErrors(), in_class_field.src_loc );
			if( class_type->members->AddName( in_class_field.name, NamesScopeValue( std::make_shared<ClassField>(std::move(class_field)), in_class_field.src_loc ) ) == nullptr )
				REPORT_ERROR( Redefinition, class_type->members->GetErrors(), in_class_field.src_loc, in_class_field.name );

			++field_number;
			class_type->SetMemberVisibility( in_class_field.name, current_visibility );
		}
		void operator()( const Synt::Function& func )
		{
			this_.NamesScopeFill( *class_type->members, func, class_type, current_visibility );
		}
		void operator()( const Synt::FunctionTemplate& func_template )
		{
			this_.NamesScopeFill( *class_type->members, func_template, class_type, current_visibility );
		}
		void operator()( const Synt::ClassVisibilityLabel& visibility_label )
		{
			if( class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Struct )
				REPORT_ERROR( VisibilityForStruct, class_type->members->GetErrors(), visibility_label.src_loc, class_name );
			current_visibility= visibility_label.visibility;
		}
		void operator()( const Synt::TypeTemplate& type_template )
		{
			this_.NamesScopeFill( *class_type->members, type_template, class_type, current_visibility );
		}
		void operator()( const Synt::Enum& enum_ )
		{
			this_.NamesScopeFill( *class_type->members, enum_ );
			class_type->SetMemberVisibility( enum_.name, current_visibility );
		}
		void operator()( const Synt::StaticAssert& static_assert_ )
		{
			this_.NamesScopeFill( *class_type->members, static_assert_ );
		}
		void operator()( const Synt::TypeAlias& type_alias )
		{
			this_.NamesScopeFill( *class_type->members, type_alias );
			class_type->SetMemberVisibility( type_alias.name, current_visibility );
		}
		void operator()( const Synt::VariablesDeclaration& variables_declaration )
		{
			this_.NamesScopeFill( *class_type->members, variables_declaration );
			for( const auto& variable_declaration : variables_declaration.variables )
				class_type->SetMemberVisibility( variable_declaration.name, current_visibility );
		}
		void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
		{
			this_.NamesScopeFill( *class_type->members, auto_variable_declaration );
			class_type->SetMemberVisibility( auto_variable_declaration.name, current_visibility );
		}
		void operator()( const Synt::Class& inner_class )
		{
			this_.NamesScopeFill( *class_type->members, inner_class );
			class_type->SetMemberVisibility( inner_class.name, current_visibility );
		}
	};

	class_declaration.elements.Iter( Visitor( *this, class_declaration, class_type, class_name ) );

	return class_type;
}

void CodeBuilder::NamesScopeFill(
	NamesScope& names_scope,
	const Synt::TypeTemplate& type_template_declaration,
	const ClassPtr base_class,
	const ClassMemberVisibility visibility )
{
	const std::string type_template_name= type_template_declaration.name;
	if( IsKeyword( type_template_name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), type_template_declaration.src_loc );

	if( NamesScopeValue* const prev_value= names_scope.GetThisScopeValue( type_template_name ) )
	{
		if( base_class != nullptr && base_class->GetMemberVisibility( type_template_name ) != visibility )
			REPORT_ERROR( TypeTemplatesVisibilityMismatch, names_scope.GetErrors(), type_template_declaration.src_loc, type_template_name ); // TODO - use separate error code

		if( TypeTemplatesSet* const type_templates_set= prev_value->value.GetTypeTemplatesSet() )
			type_templates_set->syntax_elements.push_back( &type_template_declaration );
		else
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), type_template_declaration.src_loc, type_template_name );
	}
	else
	{
		if( base_class != nullptr )
			base_class->SetMemberVisibility( type_template_name, visibility );

		TypeTemplatesSet type_templates_set;
		type_templates_set.syntax_elements.push_back( &type_template_declaration );
		names_scope.AddName( type_template_name, NamesScopeValue( std::move(type_templates_set), type_template_declaration.src_loc  ) );
	}
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::Enum& enum_declaration )
{
	if( IsKeyword( enum_declaration.name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), enum_declaration.src_loc );

	enums_table_.push_back( std::make_unique<Enum>( enum_declaration.name, &names_scope ) );
	const EnumPtr enum_= enums_table_.back().get();

	enum_->syntax_element= &enum_declaration;

	if( names_scope.AddName( enum_declaration.name, NamesScopeValue( Type( enum_ ), enum_declaration.src_loc ) ) == nullptr )
		REPORT_ERROR( Redefinition, names_scope.GetErrors(), enum_declaration.src_loc, enum_declaration.name );
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::TypeAlias& type_alias_declaration )
{
	if( IsKeyword( type_alias_declaration.name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), type_alias_declaration.src_loc );

	TypeAlias type_alias;
	type_alias.syntax_element= &type_alias_declaration;

	if( names_scope.AddName( type_alias_declaration.name, NamesScopeValue( type_alias, type_alias_declaration.src_loc ) ) == nullptr )
		REPORT_ERROR( Redefinition, names_scope.GetErrors(), type_alias_declaration.src_loc, type_alias_declaration.name );
}

void CodeBuilder::NamesScopeFill( NamesScope& names_scope, const Synt::StaticAssert& static_assert_declaration )
{
	StaticAssert static_assert_;
	static_assert_.syntax_element= &static_assert_declaration;

	names_scope.AddName(
		"static_assert_" + std::to_string(reinterpret_cast<uintptr_t>(&static_assert_declaration)),
		NamesScopeValue( static_assert_, static_assert_declaration.src_loc ) );
}

void CodeBuilder::NamesScopeFillOutOfLineElements(
	NamesScope& names_scope,
	const Synt::ProgramElementsList& namespace_elements )
{
	namespace_elements.Iter( [&]( const auto& el ) { NamesScopeFillOutOfLineElement( names_scope, el ); } );
}

void CodeBuilder::NamesScopeFillOutOfLineElement( NamesScope& names_scope, const Synt::Function& function )
{
	if( function.name.size() <= 1u )
		return;

	NamesScopeValue* value= nullptr;
	size_t component_index= 0u;
	if( function.name.front().name.empty() )
	{
		// Perform function name lookup starting from root.
		U_ASSERT( function.name.size() >= 2u );
		value= names_scope.GetRoot()->GetThisScopeValue( function.name[1].name );
		++component_index;
		if( value != nullptr )
			CollectDefinition( *value, function.name[1].src_loc );
	}
	else
	{
		// Perform regular name lookup.
		value= LookupName( names_scope, function.name[0].name, function.src_loc ).value;
		if( value != nullptr )
			CollectDefinition( *value, function.name[0].src_loc );
	}

	if( value == nullptr )
	{
		REPORT_ERROR( NameNotFound, names_scope.GetErrors(), function.src_loc, function.name.front().name );
		return;
	}

	for( size_t i= component_index + 1u; value != nullptr && i < function.name.size(); ++i )
	{
		if( const auto namespace_= value->value.GetNamespace() )
			value= namespace_->GetThisScopeValue( function.name[i].name );
		else if( const auto type= value->value.GetTypeName() )
		{
			if( const auto class_= type->GetClassType() )
				value= class_->members->GetThisScopeValue( function.name[i].name );
		}

		if( value == nullptr )
		{
			REPORT_ERROR( NameNotFound, names_scope.GetErrors(), function.src_loc, function.name[i].name );
			return;
		}
		if( i + 1 < function.name.size() )
			CollectDefinition( *value, function.name[i].src_loc );
	}

	if( value == nullptr || value->value.GetFunctionsSet() == nullptr )
	{
		REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), function.src_loc );
		return;
	}
	value->value.GetFunctionsSet()->out_of_line_syntax_elements.push_back(&function);
}

void CodeBuilder::NamesScopeFillOutOfLineElement( NamesScope& names_scope, const Synt::Namespace& namespace_ )
{
	if( const NamesScopeValue* const inner_namespace_value= names_scope.GetThisScopeValue( namespace_.name ) )
	{
		if( const NamesScopePtr inner_namespace= inner_namespace_value->value.GetNamespace() )
			NamesScopeFillOutOfLineElements( *inner_namespace, namespace_.elements );
	}
}

} // namespace U
