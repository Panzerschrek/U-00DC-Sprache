#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
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
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), namespace_->src_loc_, namespace_->name_ );
	}
	else
	{
		if( IsKeyword( namespace_->name_ ) )
			REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), namespace_->src_loc_ );

		const auto new_names_scope= std::make_shared<NamesScope>( namespace_->name_, &names_scope );
		names_scope.AddName( namespace_->name_, Value( new_names_scope, namespace_->src_loc_ ) );
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
			REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), variable_declaration.src_loc );

		IncompleteGlobalVariable incomplete_global_variable;
		incomplete_global_variable.variables_declaration= &variables_declaration;
		incomplete_global_variable.element_index= size_t( &variable_declaration - variables_declaration.variables.data() );
		incomplete_global_variable.name= variable_declaration.name;

		if( names_scope.AddName( variable_declaration.name, Value( incomplete_global_variable, variable_declaration.src_loc ) ) == nullptr )
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), variable_declaration.src_loc, variable_declaration.name );
	}
}

void CodeBuilder::NamesScopeFill(
	const Synt::AutoVariableDeclaration& variable_declaration,
	NamesScope& names_scope )
{
	if( IsKeyword( variable_declaration.name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), variable_declaration.src_loc_ );

	IncompleteGlobalVariable incomplete_global_variable;
	incomplete_global_variable.auto_variable_declaration= &variable_declaration;
	incomplete_global_variable.name= variable_declaration.name;

	if( names_scope.AddName( variable_declaration.name, Value( incomplete_global_variable, variable_declaration.src_loc_ ) ) == nullptr )
		REPORT_ERROR( Redefinition, names_scope.GetErrors(), variable_declaration.src_loc_, variable_declaration.name );
}

void CodeBuilder::NamesScopeFill(
	const Synt::FunctionPtr& function_declaration_ptr,
	NamesScope& names_scope,
	const ClassPtr& base_class,
	const ClassMemberVisibility visibility )
{
	const auto& function_declaration= *function_declaration_ptr;
	
	if( function_declaration.name_.size() != 1u )
		return; // process out of line functions later.

	const std::string& func_name= function_declaration.name_.back();
	if( IsKeyword( func_name ) && func_name != Keywords::constructor_ && func_name != Keywords::destructor_ )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), function_declaration.src_loc_ );

	if( visibility != ClassMemberVisibility::Public )
	{
		if( func_name == Keywords::constructor_ ||
			func_name == Keywords::destructor_ ||
			func_name == OverloadedOperatorToString( OverloadedOperator::Assign ) ||
			func_name == OverloadedOperatorToString( OverloadedOperator::CompareEqual ) ||
			func_name == OverloadedOperatorToString( OverloadedOperator::CompareOrder ) )
			REPORT_ERROR( ThisMethodMustBePublic, names_scope.GetErrors(), function_declaration.src_loc_, func_name );
	}

	if( Value* const prev_value= names_scope.GetThisScopeValue( func_name ) )
	{
		if( OverloadedFunctionsSet* const functions_set= prev_value->GetFunctionsSet() )
		{
			if( base_class != nullptr && base_class->GetMemberVisibility( func_name ) != visibility )
				REPORT_ERROR( FunctionsVisibilityMismatch, names_scope.GetErrors(), function_declaration.src_loc_, func_name );

			U_ASSERT( functions_set->base_class == base_class );
			functions_set->syntax_elements.push_back( &function_declaration );
		}
		else
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), function_declaration.src_loc_, func_name );
	}
	else
	{
		if( base_class != nullptr )
			base_class->SetMemberVisibility( func_name, visibility );

		OverloadedFunctionsSet functions_set;
		functions_set.base_class= base_class;
		functions_set.syntax_elements.push_back( &function_declaration );

		names_scope.AddName( func_name, Value( std::move(functions_set) ) );
	}
}

void CodeBuilder::NamesScopeFill(
	const Synt::FunctionTemplate& function_template_declaration,
	NamesScope& names_scope,
	const ClassPtr& base_class,
	const ClassMemberVisibility visibility )
{
	const auto& full_name = function_template_declaration.function_->name_;
	const std::string& function_template_name= full_name.front();

	if( full_name.size() > 1u )
		REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), function_template_declaration.src_loc_ );
	if( IsKeyword( function_template_name ) && function_template_name != Keywords::constructor_ && function_template_name != Keywords::destructor_ )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), function_template_declaration.src_loc_ );

	if( Value* const prev_value= names_scope.GetThisScopeValue( function_template_name ) )
	{
		if( OverloadedFunctionsSet* const functions_set= prev_value->GetFunctionsSet() )
		{
			if( base_class != nullptr && base_class->GetMemberVisibility( function_template_name ) != visibility )
				REPORT_ERROR( FunctionsVisibilityMismatch, names_scope.GetErrors(), function_template_declaration.src_loc_, function_template_name );

			U_ASSERT( functions_set->base_class == base_class );
			functions_set->template_syntax_elements.push_back( &function_template_declaration );
		}
		else
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), function_template_declaration.src_loc_, function_template_name );
	}
	else
	{
		if( base_class != nullptr )
			base_class->SetMemberVisibility( function_template_name, visibility );

		OverloadedFunctionsSet functions_set;
		functions_set.base_class= base_class;
		functions_set.template_syntax_elements.push_back( &function_template_declaration );

		names_scope.AddName( function_template_name, Value( std::move(functions_set) ) );
	}
}

ClassPtr CodeBuilder::NamesScopeFill( const Synt::ClassPtr& class_declaration_ptr, NamesScope& names_scope )
{
	const auto& class_declaration= *class_declaration_ptr;

	const std::string& class_name= class_declaration.name_;
	if( IsKeyword( class_name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), class_declaration.src_loc_ );

	ClassPtr class_type= nullptr;
	if( const Value* const prev_value= names_scope.GetThisScopeValue( class_name ) )
	{
		if( const Type* const type= prev_value->GetTypeName() )
		{
			if( const ClassPtr prev_class_type= type->GetClassType() )
			{
				class_type= prev_class_type;
				if(  class_type->syntax_element->is_forward_declaration_ &&  class_declaration.is_forward_declaration_ )
					REPORT_ERROR( Redefinition, names_scope.GetErrors(), class_declaration.src_loc_, class_name );
				if( !class_type->syntax_element->is_forward_declaration_ && !class_declaration.is_forward_declaration_ )
					REPORT_ERROR( ClassBodyDuplication, names_scope.GetErrors(), class_declaration.src_loc_ );
			}
			else
			{
				REPORT_ERROR( Redefinition, names_scope.GetErrors(), class_declaration.src_loc_, class_name );
				return nullptr;
			}
		}
		else
		{
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), class_declaration.src_loc_, class_name );
			return nullptr;
		}
	}
	else
	{
		const auto class_type_ptr= std::make_shared<Class>( class_name, &names_scope  );
		current_class_table_.push_back(class_type_ptr);
		class_type= class_type_ptr.get();

		names_scope.AddName( class_name, Value( Type( class_type ), class_declaration.src_loc_ ) );
		class_type->syntax_element= &class_declaration;
		class_type->body_src_loc= class_type->forward_declaration_src_loc= class_declaration.src_loc_;
		class_type->llvm_type= llvm::StructType::create( llvm_context_, mangler_->MangleType( class_type ) );

		class_type->members->AddAccessRightsFor( class_type, ClassMemberVisibility::Private );
		class_type->members->SetClass( class_type );
	}

	Class& the_class= *class_type;

	if( class_declaration.is_forward_declaration_ )
		the_class.forward_declaration_src_loc= class_declaration.src_loc_;
	else
	{
		the_class.body_src_loc= class_declaration.src_loc_;
		the_class.syntax_element= &class_declaration;
	}

	if( !class_declaration.is_forward_declaration_ )
	{
		struct Visitor final
		{
			CodeBuilder& this_;
			const Synt::Class& class_declaration;
			ClassPtr& class_type;
			Class& the_class;
			const std::string& class_name;
			ClassMemberVisibility current_visibility= ClassMemberVisibility::Public;
			unsigned int field_number= 0u;

			Visitor( CodeBuilder& in_this, const Synt::Class& in_class_declaration, ClassPtr& in_class_type, Class& in_the_class, const std::string& in_class_name )
				: this_(in_this), class_declaration(in_class_declaration), class_type(in_class_type), the_class(in_the_class), class_name(in_class_name)
			{}

			void operator()( const Synt::ClassField& in_class_field )
			{
				ClassField class_field;
				class_field.syntax_element= &in_class_field;
				class_field.original_index= field_number;

				if( IsKeyword( in_class_field.name ) )
					REPORT_ERROR( UsingKeywordAsName, the_class.members->GetErrors(), in_class_field.src_loc_ );
				if( the_class.members->AddName( in_class_field.name, Value( class_field, in_class_field.src_loc_ ) ) == nullptr )
					REPORT_ERROR( Redefinition, the_class.members->GetErrors(), in_class_field.src_loc_, in_class_field.name );

				++field_number;
				the_class.SetMemberVisibility( in_class_field.name, current_visibility );
			}
			void operator()( const Synt::FunctionPtr& func )
			{
				this_.NamesScopeFill( func, *the_class.members, class_type, current_visibility );
			}
			void operator()( const Synt::FunctionTemplate& func_template )
			{
				this_.NamesScopeFill( func_template, *the_class.members, class_type, current_visibility );
			}
			void operator()( const Synt::ClassVisibilityLabel& visibility_label )
			{
				if( class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Struct )
					REPORT_ERROR( VisibilityForStruct, the_class.members->GetErrors(), visibility_label.src_loc_, class_name );
				current_visibility= visibility_label.visibility_;
			}
			void operator()( const Synt::TypeTemplate& type_template )
			{
				this_.NamesScopeFill( type_template, *the_class.members, class_type, current_visibility );
			}
			void operator()( const Synt::Enum& enum_ )
			{
				this_.NamesScopeFill( enum_, *the_class.members );
				the_class.SetMemberVisibility( enum_.name, current_visibility );
			}
			void operator()( const Synt::StaticAssert& static_assert_ )
			{
				this_.NamesScopeFill( static_assert_, *the_class.members );
			}
			void operator()( const Synt::TypeAlias& type_alias )
			{
				this_.NamesScopeFill( type_alias, *the_class.members );
				the_class.SetMemberVisibility( type_alias.name, current_visibility );
			}
			void operator()( const Synt::VariablesDeclaration& variables_declaration )
			{
				this_.NamesScopeFill( variables_declaration, *the_class.members );
				for( const auto& variable_declaration : variables_declaration.variables )
					the_class.SetMemberVisibility( variable_declaration.name, current_visibility );
			}
			void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
			{
				this_.NamesScopeFill( auto_variable_declaration, *the_class.members );
				the_class.SetMemberVisibility( auto_variable_declaration.name, current_visibility );
			}
			void operator()( const Synt::ClassPtr& inner_class )
			{
				this_.NamesScopeFill( inner_class, *the_class.members );
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
	const Synt::TypeTemplate& type_template_declaration,
	NamesScope& names_scope,
	const ClassPtr& base_class,
	const ClassMemberVisibility visibility )
{
	const std::string type_template_name= type_template_declaration.name_;
	if( IsKeyword( type_template_name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), type_template_declaration.src_loc_ );

	if( Value* const prev_value= names_scope.GetThisScopeValue( type_template_name ) )
	{
		if( base_class != nullptr && base_class->GetMemberVisibility( type_template_name ) != visibility )
			REPORT_ERROR( TypeTemplatesVisibilityMismatch, names_scope.GetErrors(), type_template_declaration.src_loc_, type_template_name ); // TODO - use separate error code

		if( TypeTemplatesSet* const type_templates_set= prev_value->GetTypeTemplatesSet() )
			type_templates_set->syntax_elements.push_back( &type_template_declaration );
		else
			REPORT_ERROR( Redefinition, names_scope.GetErrors(), type_template_declaration.src_loc_, type_template_name );
	}
	else
	{
		if( base_class != nullptr )
			base_class->SetMemberVisibility( type_template_name, visibility );

		TypeTemplatesSet type_templates_set;
		type_templates_set.syntax_elements.push_back( &type_template_declaration );
		names_scope.AddName( type_template_name, Value( std::move(type_templates_set), type_template_declaration.src_loc_ ) );
	}
}

void CodeBuilder::NamesScopeFill(
	const Synt::Enum& enum_declaration,
	NamesScope& names_scope )
{
	if( IsKeyword( enum_declaration.name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), enum_declaration.src_loc_ );

	enums_table_.push_back( std::make_unique<Enum>( enum_declaration.name, &names_scope ) );
	const EnumPtr enum_= enums_table_.back().get();

	enum_->syntax_element= &enum_declaration;

	if( names_scope.AddName( enum_declaration.name, Value( Type( enum_ ), enum_declaration.src_loc_ ) ) == nullptr )
		REPORT_ERROR( Redefinition, names_scope.GetErrors(), enum_declaration.src_loc_, enum_declaration.name );
}

void CodeBuilder::NamesScopeFill(
	const Synt::TypeAlias& typedef_declaration,
	NamesScope& names_scope )
{
	if( IsKeyword( typedef_declaration.name ) )
		REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), typedef_declaration.src_loc_ );

	Typedef typedef_;
	typedef_.syntax_element= &typedef_declaration;

	if( names_scope.AddName( typedef_declaration.name, Value( typedef_, typedef_declaration.src_loc_ ) ) == nullptr )
		REPORT_ERROR( Redefinition, names_scope.GetErrors(), typedef_declaration.src_loc_, typedef_declaration.name );
}

void CodeBuilder::NamesScopeFill(
	const Synt::StaticAssert& static_assert_declaration,
	NamesScope& names_scope )
{
	StaticAssert static_assert_;
	static_assert_.syntax_element= &static_assert_declaration;

	names_scope.AddName(
		"_sa_" + std::to_string(reinterpret_cast<uintptr_t>(&static_assert_declaration)),
		Value( static_assert_, static_assert_declaration.src_loc_ ) );
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
			if( func.name_.size() <= 1u )
				continue;

			NamesScope* src_space= &names_scope;
			size_t component_index= 0u;
			if( func.name_.front().empty() )
			{
				U_ASSERT( func.name_.size() >= 2u );
				src_space= src_space->GetRoot();
				++component_index;
			}

			Value* prev_value = nullptr;
			while( prev_value == nullptr && src_space != nullptr )
			{
				prev_value= src_space->GetThisScopeValue( func.name_[component_index] );
				src_space= src_space->GetParent();
			}

			if( prev_value == nullptr )
			{
				REPORT_ERROR( NameNotFound, names_scope.GetErrors(), func.src_loc_, func.name_.front() );
				continue;
			}

			for( size_t i= component_index + 1u; prev_value != nullptr && i < func.name_.size(); ++i )
			{
				if( const auto namespace_= prev_value->GetNamespace() )
					prev_value= namespace_->GetThisScopeValue( func.name_[i] );
				else if( const auto type= prev_value->GetTypeName() )
				{
					if( const auto class_= type->GetClassType() )
						prev_value= class_->members->GetThisScopeValue( func.name_[i] );
				}

				if( prev_value == nullptr )
					REPORT_ERROR( NameNotFound, names_scope.GetErrors(), func.src_loc_, func.name_[i] );
			}

			if( prev_value == nullptr || prev_value->GetFunctionsSet() == nullptr )
			{
				REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), func.src_loc_ );
				continue;
			}
			prev_value->GetFunctionsSet()->out_of_line_syntax_elements.push_back(&func);
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

} // namespace U
