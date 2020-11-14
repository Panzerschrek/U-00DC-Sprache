﻿#include <algorithm>

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/GlobalVariable.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "mangling.hpp"

#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

namespace
{

void CreateTemplateErrorsContext(
	CodeBuilderErrorsContainer& errors_container,
	const FilePos& file_pos,
	const NamesScopePtr& template_parameters_namespace,
	const TemplateBase& template_,
	const std::string& template_name,
	const TemplateArgs& template_args )
{
	REPORT_ERROR( TemplateContext, errors_container, file_pos );
	const auto template_error_context= std::make_shared<TemplateErrorsContext>();
	template_error_context->context_declaration_file_pos= template_.file_pos;
	errors_container.back().template_context= template_error_context;
	template_parameters_namespace->SetErrors( template_error_context->errors );

	{
		std::string args_description;
		args_description+= "[ with ";

		for( size_t i= 0u; i < template_args.size() ; ++i )
		{
			const TemplateArg& arg= template_args[i];

			args_description+= template_.template_params[i].name + " = ";
			if( const Type* const type= std::get_if<Type>( &arg ) )
				args_description+= type->ToString();
			else if( const Variable* const variable= std::get_if<Variable>( &arg ) )
				args_description+= ConstantVariableToString( *variable );
			else U_ASSERT(false);

			if( i + 1u < template_args.size() )
				args_description+= ", ";
		}

		args_description+= " ]";
		template_error_context->parameters_description= std::move(args_description);
	}
	{
		std::string name= template_.parent_namespace->ToString();
		if( !name.empty() )
			name+= "::";
		name+= template_name;

		template_error_context->context_name= std::move(name);
	}
}

} // namesapce

void CodeBuilder::PrepareTypeTemplate(
	const Synt::TypeTemplateBase& type_template_declaration,
	TypeTemplatesSet& type_templates_set,
	NamesScope& names_scope )
{
	/* SPRACHE_TODO:
	 *) Support default template arguments for short form.
	 *) Convert signature and template arguments to "default form" for equality comparison.
	 *) Add "enable_if".
	 *) Support template-dependent types for value parameters, such template</ type T, U</ T /> ut />.
	*/

	const auto type_template= std::make_shared<TypeTemplate>();
	type_templates_set.type_templates.push_back( type_template );

	type_template->parent_namespace= &names_scope;
	type_template->syntax_element= &type_template_declaration;
	type_template->file_pos= type_template_declaration.file_pos_;

	std::vector<TypeTemplate::TemplateParameter>& template_parameters= type_template->template_params;
	template_parameters.reserve( type_template_declaration.args_.size() );
	std::vector<bool> template_parameters_usage_flags;

	ProcessTemplateArgs(
		type_template_declaration.args_,
		names_scope,
		type_template_declaration.file_pos_,
		template_parameters,
		template_parameters_usage_flags );

	if( type_template_declaration.is_short_form_ )
	{
		U_ASSERT( type_template_declaration.signature_args_.empty() );
		// Assign template arguments to signature arguments.
		for( const Synt::TypeTemplateBase::Arg& arg : type_template_declaration.args_ )
		{
			type_template->signature_params_new.push_back(
				CheckTemplateSignatureParameter( type_template_declaration.file_pos_, *arg.name, names_scope, *global_function_context_, template_parameters, template_parameters_usage_flags ) );
			type_template->signature_params.push_back(arg.name_expr.get());
			type_template->default_signature_params.push_back(nullptr);
		}
		type_template->first_optional_signature_param= type_template->signature_params.size();
	}
	else
	{
		// Check and fill signature args.
		type_template->first_optional_signature_param= 0u;
		for( const Synt::TypeTemplateBase::SignatureArg& signature_arg : type_template_declaration.signature_args_ )
		{
			type_template->signature_params_new.push_back(
				CheckTemplateSignatureParameter( signature_arg.name, names_scope, *global_function_context_, template_parameters, template_parameters_usage_flags ) );
			type_template->signature_params.push_back(&signature_arg.name);

			if( std::get_if<Synt::EmptyVariant>( &signature_arg.default_value ) == nullptr )
			{
				CheckTemplateSignatureParameter( signature_arg.default_value, names_scope, *global_function_context_, template_parameters, template_parameters_usage_flags );
				type_template->default_signature_params.push_back(&signature_arg.default_value);
			}
			else
			{
				const size_t index= type_template->signature_params.size() - 1u;
				if (index > type_template->first_optional_signature_param )
					REPORT_ERROR( MandatoryTemplateSignatureArgumentAfterOptionalArgument, names_scope.GetErrors(), type_template_declaration.file_pos_ );

				type_template->default_signature_params.push_back(nullptr);
				++type_template->first_optional_signature_param;
			}
		}
	}
	U_ASSERT( type_template->signature_params.size() == type_template->default_signature_params.size() );
	U_ASSERT( type_template->first_optional_signature_param <= type_template->default_signature_params.size() );

	type_template->params_types.resize( type_template->template_params.size() );
	for( size_t i= 0u; i < type_template->template_params.size(); ++i )
	{
		if( type_template->template_params[i].type_name != nullptr )
			type_template->params_types[i]=
				CheckTemplateSignatureParameter(
					type_template_declaration.file_pos_,
					*type_template->template_params[i].type_name,
					names_scope,
					*global_function_context_,
					template_parameters,
					template_parameters_usage_flags );
	}

	for( size_t i= 0u; i < type_template->template_params.size(); ++i )
		if( !template_parameters_usage_flags[i] )
			REPORT_ERROR( TemplateArgumentNotUsedInSignature, names_scope.GetErrors(), type_template_declaration.file_pos_, type_template->template_params[i].name );
}

void CodeBuilder::PrepareFunctionTemplate(
	const Synt::FunctionTemplate& function_template_declaration,
	OverloadedFunctionsSet& functions_set,
	NamesScope& names_scope,
	const ClassProxyPtr& base_class )
{
	const auto& full_name= function_template_declaration.function_->name_;
	const std::string& function_template_name= full_name.front();

	if( full_name.size() > 1u )
		REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), function_template_declaration.file_pos_ );

	if( function_template_declaration.function_->block_ == nullptr )
		REPORT_ERROR( IncompleteMemberOfClassTemplate, names_scope.GetErrors(), function_template_declaration.file_pos_, function_template_name );
	if( function_template_declaration.function_->virtual_function_kind_ != Synt::VirtualFunctionKind::None )
		REPORT_ERROR( VirtualForFunctionTemplate, names_scope.GetErrors(), function_template_declaration.file_pos_, function_template_name );

	const auto function_template= std::make_shared<FunctionTemplate>();
	function_template->syntax_element= &function_template_declaration;
	function_template->file_pos= function_template_declaration.file_pos_;
	function_template->parent_namespace= &names_scope;
	function_template->base_class= base_class;

	std::vector<bool> template_parameters_usage_flags; // Currently unused, because function template have no signature.

	ProcessTemplateArgs(
		function_template_declaration.args_,
		names_scope,
		function_template_declaration.file_pos_,
		function_template->template_params,
		template_parameters_usage_flags );

	for( const Synt::FunctionArgument& function_param : function_template_declaration.function_->type_.arguments_ )
	{
		if( base_class != nullptr && function_param.name_ == Keyword( Keywords::this_ ) )
			function_template->signature_params_new.push_back( DeducedTemplateParameter::TypeParam{ Type(base_class) } );
		else
		{
			function_template->signature_params_new.push_back(
				CheckTemplateSignatureParameter( function_param.type_, names_scope, *global_function_context_, function_template->template_params, template_parameters_usage_flags ) );
		}
	}

	function_template->params_types.resize( function_template->template_params.size() );
	for( size_t i= 0u; i < function_template->template_params.size(); ++i )
	{
		if( function_template->template_params[i].type_name != nullptr )
			function_template->params_types[i]=
				CheckTemplateSignatureParameter(
					function_template_declaration.file_pos_,
					*function_template->template_params[i].type_name,
					names_scope,
					*global_function_context_,
					function_template->template_params,
					template_parameters_usage_flags );
	}

	// Do not report about unused template parameters because they may not be used in function signature or even in function type but used only inside body.
	// For example:
	// template</ type T /> fn Foo()
	// {
	//		T::DoSomething();
	// }

	// TODO - check duplicates and function templates with same signature.
	functions_set.template_functions.push_back( function_template );
}

void CodeBuilder::ProcessTemplateArgs(
	const std::vector<Synt::TemplateBase::Arg>& args,
	NamesScope& names_scope,
	const FilePos& file_pos,
	std::vector<TypeTemplate::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags )
{
	U_ASSERT( template_parameters.empty() );
	U_ASSERT( template_parameters_usage_flags.empty() );

	// Check and fill template parameters.
	for( const Synt::TemplateBase::Arg& arg : args )
	{
		U_ASSERT( std::get_if<std::string>( &arg.name->start_value ) != nullptr );
		const std::string& arg_name= std::get<std::string>(arg.name->start_value);

		// Check redefinition
		for( const auto& prev_arg : template_parameters )
		{
			if( prev_arg.name == arg_name )
			{
				REPORT_ERROR( Redefinition, names_scope.GetErrors(), file_pos, arg_name );
				continue;
			}
		}
		if( NameShadowsTemplateArgument( arg_name, names_scope ) )
			REPORT_ERROR( DeclarationShadowsTemplateArgument, names_scope.GetErrors(), file_pos, arg_name );

		template_parameters.emplace_back();
		template_parameters.back().name= arg_name;
		template_parameters_usage_flags.push_back(false);

		if( arg.arg_type != nullptr )
		{
			// If template parameter is variable.
			template_parameters.back().type_name= arg.arg_type;

			bool arg_type_is_template= false;
			const std::string* const arg_type_start= std::get_if<std::string>( &arg.arg_type->start_value );
			if( arg_type_start != nullptr && arg.arg_type->tail == nullptr )
			{
				for( const TypeTemplate::TemplateParameter& template_parameter : template_parameters )
				{
					if( template_parameter.name == *arg_type_start)
					{
						arg_type_is_template= true;
						template_parameters_usage_flags[ size_t(&template_parameter - template_parameters.data()) ]= true;
						break;
					}
				}
			}

			if( !arg_type_is_template )
			{
				// Resolve from outer space or from this template parameters.
				const Value type_value= ResolveValue( file_pos, names_scope, *global_function_context_, *arg.arg_type );
				const Type* const type= type_value.GetTypeName();
				if( type == nullptr )
				{
					REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), file_pos, *arg.arg_type );
					continue;
				}

				if( !TypeIsValidForTemplateVariableArgument( *type ) )
				{
					REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, names_scope.GetErrors(), file_pos, type );
					continue;
				}
			}
		}
	}

	U_ASSERT( template_parameters_usage_flags.size() == template_parameters.size() );
}

DeducedTemplateParameter CodeBuilder::CheckTemplateSignatureParameter(
	const FilePos& file_pos,
	const Synt::ComplexName& signature_parameter,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::vector<TemplateBase::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags )
{
	// If signature parameter is template parameter, set usage flag.
	const std::string* const signature_parameter_start= std::get_if<std::string>( &signature_parameter.start_value );
	if( signature_parameter_start != nullptr && signature_parameter.tail == nullptr )
	{
		for( const TypeTemplate::TemplateParameter& template_parameter : template_parameters )
		{
			if( template_parameter.name == *signature_parameter_start )
			{
				const size_t param_index= size_t(&template_parameter - template_parameters.data());
				template_parameters_usage_flags[ param_index ]= true;

				return DeducedTemplateParameter::TemplateParameter{ param_index };
			}
		}
	}

	const Value start_value= ResolveForTemplateSignatureParameter( file_pos, signature_parameter, names_scope );
	if( const auto type_templates_set= start_value.GetTypeTemplatesSet() )
	{
		const Synt::ComplexName::Component* name_component= signature_parameter.tail.get();
		if( name_component == nullptr )
		{
			REPORT_ERROR( TemplateInstantiationRequired, names_scope.GetErrors(), file_pos, "TODO: template name" );
			return DeducedTemplateParameter::InvalidParam();
		}

		while( name_component->next != nullptr )
			name_component= name_component->next.get();

		const auto last_template_parameters= std::get_if< std::vector<Synt::Expression> >( &name_component->name_or_template_paramenters );
		if( last_template_parameters == nullptr )
		{
			REPORT_ERROR( TemplateInstantiationRequired, names_scope.GetErrors(), file_pos, "TODO:  template name" );
			return DeducedTemplateParameter::InvalidParam();
		}

		DeducedTemplateParameter::SpecializedTemplateParam specialized_template;

		bool all_args_are_known= true;
		for( const Synt::Expression& template_parameter : *last_template_parameters )
		{
			specialized_template.params.push_back( CheckTemplateSignatureParameter( template_parameter, names_scope, function_context, template_parameters, template_parameters_usage_flags ) );
			all_args_are_known&= specialized_template.params.back().IsType() || specialized_template.params.back().IsVariable();
		}

		if( all_args_are_known )
			return ValueToTemplateParam( ResolveValue( file_pos, names_scope, function_context, signature_parameter ), names_scope );

		specialized_template.type_templates= type_templates_set->type_templates;

		return specialized_template;
	}

	return ValueToTemplateParam( ResolveValue( file_pos, names_scope, function_context, signature_parameter ), names_scope );
}

DeducedTemplateParameter CodeBuilder::CheckTemplateSignatureParameter(
	const Synt::Expression& template_parameter,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::vector<TemplateBase::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags )
{
	if( const auto named_operand= std::get_if<Synt::NamedOperand>( &template_parameter ) )
	{
		if( named_operand->postfix_operators_.empty() && named_operand->prefix_operators_.empty() )
			return CheckTemplateSignatureParameter( named_operand->file_pos_, named_operand->name_, names_scope, function_context, template_parameters, template_parameters_usage_flags );
	}
	else if( const auto type_name= std::get_if<Synt::TypeNameInExpression>( &template_parameter ) )
		return CheckTemplateSignatureParameter( type_name->type_name, names_scope, function_context, template_parameters, template_parameters_usage_flags );
	else if( const auto bracket_expression= std::get_if<Synt::BracketExpression>( &template_parameter ) )
	{
		if( bracket_expression->postfix_operators_.empty() && bracket_expression->prefix_operators_.empty() )
			return CheckTemplateSignatureParameter( *bracket_expression->expression_, names_scope, function_context, template_parameters, template_parameters_usage_flags );
	}

	return ValueToTemplateParam( BuildExpressionCode( template_parameter, names_scope, function_context ), names_scope );
}

DeducedTemplateParameter CodeBuilder::CheckTemplateSignatureParameter(
	const Synt::TypeName& type_name_template_parameter,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::vector<TemplateBase::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags )
{
	if( const auto named_type_name= std::get_if<Synt::NamedTypeName>(&type_name_template_parameter) )
		return CheckTemplateSignatureParameter( named_type_name->file_pos_, named_type_name->name, names_scope, function_context, template_parameters, template_parameters_usage_flags );
	else if( const auto array_type_name= std::get_if<Synt::ArrayTypeName>(&type_name_template_parameter) )
	{
		DeducedTemplateParameter::ArrayParam array_param;
		array_param.size= std::make_unique<DeducedTemplateParameter>( CheckTemplateSignatureParameter( *array_type_name->size, names_scope, function_context, template_parameters, template_parameters_usage_flags ) );
		array_param.type= std::make_unique<DeducedTemplateParameter>( CheckTemplateSignatureParameter( *array_type_name->element_type, names_scope, function_context, template_parameters, template_parameters_usage_flags ) );

		if( array_param.size->IsVariable() && array_param.type->IsType() )
			return DeducedTemplateParameter::TypeParam{ PrepareType( *array_type_name, names_scope, function_context ) };

		return array_param;
	}
	else if( const auto tuple_type_name= std::get_if<Synt::TupleType>(&type_name_template_parameter) )
	{
		DeducedTemplateParameter::TupleParam tuple_param;

		bool all_types_are_known= true;
		for( const auto& element_type : tuple_type_name->element_types_ )
		{
			tuple_param.element_types.push_back( CheckTemplateSignatureParameter( element_type, names_scope, function_context, template_parameters, template_parameters_usage_flags ) );
			all_types_are_known&= tuple_param.element_types.back().IsType();
		}

		if( all_types_are_known )
			return DeducedTemplateParameter::TypeParam{ PrepareType( *tuple_type_name, names_scope, function_context ) };

		return tuple_param;
	}
	else if( const auto function_pointer_type_name_ptr= std::get_if<Synt::FunctionTypePtr>(&type_name_template_parameter) )
	{
		DeducedTemplateParameter::FunctionParam function_param;

		bool all_types_are_known= true;

		const Synt::FunctionType& function_pointer_type_name= **function_pointer_type_name_ptr;

		function_param.return_value_is_reference= function_pointer_type_name.return_value_reference_modifier_ == Synt::ReferenceModifier::Reference;
		function_param.return_value_is_mutable= function_pointer_type_name.return_value_mutability_modifier_ == Synt::MutabilityModifier::Mutable;
		function_param.is_unsafe= function_pointer_type_name.unsafe_;

		// TODO - maybe check also reference tags?
		if( function_pointer_type_name.return_type_ != nullptr )
			function_param.return_type= std::make_unique<DeducedTemplateParameter>( CheckTemplateSignatureParameter( *function_pointer_type_name.return_type_, names_scope, function_context, template_parameters, template_parameters_usage_flags ) );
		else
			function_param.return_type= std::make_unique<DeducedTemplateParameter>( DeducedTemplateParameter::TypeParam{ void_type_for_ret_ } );

		all_types_are_known&= function_param.return_type->IsType();

		for( const Synt::FunctionArgument& arg : function_pointer_type_name.arguments_ )
		{
			auto t= CheckTemplateSignatureParameter( arg.type_, names_scope, function_context, template_parameters, template_parameters_usage_flags );
			all_types_are_known&= t.IsType();

			DeducedTemplateParameter::FunctionParam::Param param;
			param.type= std::make_unique<DeducedTemplateParameter>( std::move(t) );
			param.is_mutable= arg.mutability_modifier_ == Synt::MutabilityModifier::Mutable;
			param.is_reference= arg.reference_modifier_ == Synt::ReferenceModifier::Reference;

			function_param.params.push_back( std::move(param) );
		}

		if( all_types_are_known )
			return DeducedTemplateParameter::TypeParam{ PrepareType( function_pointer_type_name, names_scope, function_context ) };

		return function_param;
	}
	else U_ASSERT(false);

	return DeducedTemplateParameter::InvalidParam();
}

DeducedTemplateParameter CodeBuilder::ValueToTemplateParam( const Value& value, NamesScope& names_scope )
{
	if( const auto type= value.GetTypeName() )
		return DeducedTemplateParameter::TypeParam{ *type };

	if( const auto variable= value.GetVariable() )
	{
		if( !TypeIsValidForTemplateVariableArgument( variable->type ) )
		{
			REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, names_scope.GetErrors(), value.GetFilePos(), variable->type );
			return DeducedTemplateParameter::InvalidParam();
		}
		if( variable->constexpr_value == nullptr )
		{
			REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), value.GetFilePos() );
			return DeducedTemplateParameter::InvalidParam();
		}
		return DeducedTemplateParameter::VariableParam{ *variable };
	}

	return DeducedTemplateParameter::InvalidParam();
}

Value CodeBuilder::ResolveForTemplateSignatureParameter(
	const FilePos& file_pos,
	const Synt::ComplexName& signature_parameter,
	NamesScope& names_scope )
{
	return ResolveValue( file_pos, names_scope, *global_function_context_, signature_parameter, ResolveMode::ForTemplateSignatureParameter );
}

bool CodeBuilder::MatchTemplateArg(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const FilePos& file_pos,
	const DeducedTemplateParameter& template_param )
{
	return
		template_param.Visit(
			[&]( const auto& param )
			{
				return MatchTemplateArgImpl( template_, args_names_scope, template_arg, file_pos, param );
			} );
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const FilePos& file_pos,
	const DeducedTemplateParameter::InvalidParam& template_param )
{
	(void) template_;
	(void) args_names_scope;
	(void) template_arg;
	(void) template_param;
	(void)file_pos;
	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const FilePos& file_pos,
	const DeducedTemplateParameter::TypeParam& template_param )
{
	(void)template_;
	(void)args_names_scope;
	(void)file_pos;

	if( const auto given_type= std::get_if<Type>( &template_arg ) )
		return *given_type == template_param.t;
	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const FilePos& file_pos,
	const DeducedTemplateParameter::VariableParam& template_param )
{
	(void)template_;
	(void)args_names_scope;
	(void)file_pos;

	if( const auto given_variable= std::get_if<Variable>( &template_arg ) )
	{
		if( given_variable->type != template_param.v.type )
			return false;
		if( given_variable->constexpr_value->getUniqueInteger() != template_param.v.constexpr_value->getUniqueInteger() )
			return false;
		return true;
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const FilePos& file_pos,
	const DeducedTemplateParameter::TemplateParameter& template_param )
{
	Value* const value= args_names_scope.GetThisScopeValue( template_.template_params[ template_param.index ].name );
	U_ASSERT( value != nullptr );
	if( value->GetYetNotDeducedTemplateArg() != nullptr )
	{
		const auto param_type= template_.params_types[ template_param.index ];
		const bool is_variable_param= param_type != std::nullopt;

		if( const auto given_type= std::get_if<Type>( &template_arg ) )
		{
			if( is_variable_param )
				return false;

			*value= Value( *given_type, file_pos );
			return true;
		}
		if( const auto given_varaible= std::get_if<Variable>( &template_arg ) )
		{
			if( !is_variable_param )
				return false;

			if( !TypeIsValidForTemplateVariableArgument( given_varaible->type ) )
			{
				REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, args_names_scope.GetErrors(), file_pos, given_varaible->type );
				return false;
			}
			if( given_varaible->constexpr_value == nullptr )
			{
				REPORT_ERROR( ExpectedConstantExpression, args_names_scope.GetErrors(), file_pos );
				return false;
			}

			if( !MatchTemplateArg( template_, args_names_scope, given_varaible->type, file_pos, *param_type ) )
				return false;

			Variable variable_for_insertion;
			variable_for_insertion.type= given_varaible->type;
			variable_for_insertion.location= Variable::Location::Pointer;
			variable_for_insertion.value_type= ValueType::ConstReference;
			variable_for_insertion.llvm_value=
				CreateGlobalConstantVariable(
					given_varaible->type,
					template_.template_params[ template_param.index ].name,
					given_varaible->constexpr_value );
			variable_for_insertion.constexpr_value= given_varaible->constexpr_value;

			*value= Value( std::move(variable_for_insertion), file_pos );
			return true;
		}
	}
	else if( const auto prev_type= value->GetTypeName() )
	{
		if( const auto given_type= std::get_if<Type>( &template_arg ) )
			return *given_type == *prev_type;
		return false;
	}
	else if( const auto prev_variable= value->GetVariable() )
	{
		if( const auto given_varaible= std::get_if<Variable>( &template_arg ) )
		{
			if( given_varaible->type != prev_variable->type )
				return false;

			return given_varaible->constexpr_value->getUniqueInteger() == prev_variable->constexpr_value->getUniqueInteger();
		}

		return false;
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const FilePos& file_pos,
	const DeducedTemplateParameter::ArrayParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_array_type= given_type->GetArrayType() )
		{
			if( !MatchTemplateArg( template_, args_names_scope, given_array_type->type, file_pos, *template_param.type ) )
				return false;

			Variable size_variable;
			size_variable.type= size_type_;
			size_variable.constexpr_value=
				llvm::ConstantInt::get(
					size_type_.GetLLVMType(),
					llvm::APInt( static_cast<unsigned int>(size_type_.GetFundamentalType()->GetSize() * 8), given_array_type->size ) );

			if( !MatchTemplateArg( template_, args_names_scope, size_variable, file_pos, *template_param.size ) )
				return false;

			return true;
		}
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const FilePos& file_pos,
	const DeducedTemplateParameter::TupleParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_tuple_type= given_type->GetTupleType() )
		{
			if( given_tuple_type->elements.size() != template_param.element_types.size() )
				return false;

			for( size_t i= 0; i < template_param.element_types.size(); ++i )
			{
				if( !MatchTemplateArg( template_, args_names_scope, given_tuple_type->elements[i], file_pos, template_param.element_types[i] ) )
					return false;
			}

			return true;
		}
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const FilePos& file_pos,
	const DeducedTemplateParameter::FunctionParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_function_pointer_type= given_type->GetFunctionPointerType() )
		{
			const Function& given_function_type= given_function_pointer_type->function;

			if( given_function_type.unsafe != template_param.is_unsafe )
				return false;
			if( given_function_type.return_value_is_mutable != template_param.return_value_is_mutable )
				return false;
			if( given_function_type.return_value_is_reference != template_param.return_value_is_reference )
				return false;

			if( !MatchTemplateArg( template_, args_names_scope, given_function_type.return_type, file_pos, *template_param.return_type ) )
				return false;

			if( given_function_type.args.size() != template_param.params.size() )
				return false;

			for( size_t i= 0; i < template_param.params.size(); ++i )
			{
				if( given_function_type.args[i].is_mutable != template_param.params[i].is_mutable )
					return false;
				if( given_function_type.args[i].is_reference != template_param.params[i].is_reference )
					return false;
				if( ! MatchTemplateArg( template_, args_names_scope, given_function_type.args[i].type, file_pos, *template_param.params[i].type ) )
					return false;
			}

			return true;
		}
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const FilePos& file_pos,
	const DeducedTemplateParameter::SpecializedTemplateParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_class_type= given_type->GetClassTypeProxy() )
		{
			const Class& given_class= *given_class_type->class_;
			if( given_class.base_template == std::nullopt )
				return false;

			bool is_one_of_templates= false;
			for( const TypeTemplatePtr& type_template :template_param.type_templates )
				if( given_class.base_template->class_template == type_template )
				{
					is_one_of_templates= true;
					break;
				}

			if( !is_one_of_templates )
				return false;

			if( template_param.params.size() != given_class.base_template->signature_args.size() )
				return false;

			for( size_t i= 0; i < template_param.params.size(); ++i )
			{
				if( !MatchTemplateArg( template_, args_names_scope, given_class.base_template->signature_args[i], file_pos, template_param.params[i] ) )
					return false;
			}

			return true;
		}
	}

	return false;
}

Value* CodeBuilder::GenTemplateType(
	const FilePos& file_pos,
	const TypeTemplatesSet& type_templates_set,
	const std::vector<Synt::Expression>& template_arguments,
	NamesScope& arguments_names_scope,
	FunctionContext& function_context )
{
	std::vector<Value> arguments_calculated;
	arguments_calculated.reserve( template_arguments.size() );
	for( const Synt::Expression& expr : template_arguments )
		arguments_calculated.push_back( BuildExpressionCode( expr, arguments_names_scope, function_context ) );

	std::vector<TemplateTypeGenerationResult> generated_types;
	for( const TypeTemplatePtr& type_template : type_templates_set.type_templates )
	{
		TemplateTypeGenerationResult generated_type=
			GenTemplateType(
				file_pos,
				type_template,
				arguments_calculated,
				arguments_names_scope,
				true );
		if( generated_type.type_template != nullptr )
		{
			U_ASSERT(generated_type.deduced_template_parameters.size() >= template_arguments.size());
			generated_types.push_back( std::move(generated_type) );
		}
	}

	if( generated_types.empty() )
	{
		REPORT_ERROR( TemplateParametersDeductionFailed, arguments_names_scope.GetErrors(), file_pos );
		return nullptr;
	}

	if( const TemplateTypeGenerationResult* const selected_template= SelectTemplateType( generated_types, template_arguments.size() ) )
		return
			GenTemplateType(
				file_pos,
				selected_template->type_template,
				arguments_calculated,
				arguments_names_scope,
				false ).type;
	else
	{
		REPORT_ERROR( CouldNotSelectMoreSpicializedTypeTemplate, arguments_names_scope.GetErrors(), file_pos );
		return nullptr;
	}
}

CodeBuilder::TemplateTypeGenerationResult CodeBuilder::GenTemplateType(
	const FilePos& file_pos,
	const TypeTemplatePtr& type_template_ptr,
	const std::vector<Value>& template_arguments,
	NamesScope& arguments_names_scope,
	const bool skip_type_generation )
{
	// This method does not generate some errors, because instantiation may fail
	// for one class template, but success for other.

	TemplateTypeGenerationResult result;

	const TypeTemplate& type_template= *type_template_ptr;
	NamesScope& template_names_scope= *type_template.parent_namespace;

	if( template_arguments.size() < type_template.first_optional_signature_param ||
		template_arguments.size() > type_template.signature_params.size() )
		return result;

	const NamesScopePtr template_args_namespace= std::make_shared<NamesScope>( NamesScope::c_template_args_namespace_name, &template_names_scope );
	for( const TypeTemplate::TemplateParameter& param : type_template.template_params )
		template_args_namespace->AddName( param.name, YetNotDeducedTemplateArg() );

	bool deduction_failed= false;
	TemplateArgs result_signature_args(type_template.signature_params.size());
	result.deduced_template_parameters.resize(type_template.signature_params.size());
	for( size_t i= 0u; i < type_template.signature_params.size(); ++i )
	{
		Value value;
		if( i < template_arguments.size() )
			value= template_arguments[i];
		else
			value= BuildExpressionCode( *type_template.default_signature_params[i], *template_args_namespace, *global_function_context_ );

		if( const Type* const type_name= value.GetTypeName() )
			result_signature_args[i]= *type_name;
		else if( const Variable* const variable= value.GetVariable() )
			result_signature_args[i]= *variable;
		else
		{
			REPORT_ERROR( InvalidValueAsTemplateArgument, arguments_names_scope.GetErrors(), file_pos, value.GetKindName() );
			continue;
		}

		if( !MatchTemplateArg( type_template, *template_args_namespace, result_signature_args[i], file_pos, type_template.signature_params_new[i] ) )
		{
			deduction_failed= true;
			continue;
		}

	} // for signature arguments

	if( deduction_failed )
		return result;

	TemplateArgs deduced_template_args;
	for( size_t i = 0u; i < type_template.template_params.size() ; ++i )
	{
		const Value* const value= template_args_namespace->GetThisScopeValue( type_template.template_params[i].name );

		if( const auto type= value->GetTypeName() )
			deduced_template_args.push_back( *type );
		else if( const auto variable= value->GetVariable() )
			deduced_template_args.push_back( *variable );
		else
		{
			// SPRACHE_TODO - maybe not generate this error?
			// Other function templates, for example, can match given aruments.
			REPORT_ERROR( TemplateParametersDeductionFailed, arguments_names_scope.GetErrors(), file_pos );
			return result;
		}
	}

	result.deduced_template_parameters= type_template.signature_params_new;
	result.type_template= type_template_ptr;

	if( skip_type_generation )
		return result;

	// Encode name for caching. Name must be unique for each template and its parameters.
	const std::string name_encoded=
		std::to_string( reinterpret_cast<uintptr_t>( &type_template ) ) + // Encode template address, because we needs unique keys for templates with same name.
		MangleTemplateArgs( result_signature_args );

	{ // Check, if already type generated.
		const auto it= generated_template_things_storage_.find( name_encoded );
		if( it != generated_template_things_storage_.end() )
		{
			const NamesScopePtr template_parameters_space= it->second.GetNamespace();
			U_ASSERT( template_parameters_space != nullptr );
			result.type= template_parameters_space->GetThisScopeValue( Class::c_template_class_name );
			return result;
		}
	}

	generated_template_things_storage_.insert( std::make_pair( name_encoded, Value( template_args_namespace, type_template.syntax_element->file_pos_ ) ) );

	CreateTemplateErrorsContext( arguments_names_scope.GetErrors(), file_pos, template_args_namespace, type_template, type_template.syntax_element->name_, deduced_template_args );

	if( type_template.syntax_element->kind_ == Synt::TypeTemplateBase::Kind::Class )
	{
		const auto cache_class_it= template_classes_cache_.find( name_encoded );
		if( cache_class_it != template_classes_cache_.end() )
		{
			result.type=
				template_args_namespace->AddName(
					Class::c_template_class_name,
					Value(
						cache_class_it->second,
						type_template.syntax_element->file_pos_ /* TODO - check file_pos */ ) );
			return result;
		}

		const ClassProxyPtr class_proxy= NamesScopeFill( static_cast<const Synt::ClassTemplate*>( type_template.syntax_element )->class_, *template_args_namespace, Class::c_template_class_name );
		if( class_proxy == nullptr )
			return result;

		Class& the_class= *class_proxy->class_;
		// Save in class info about it`s base template.
		the_class.base_template.emplace();
		the_class.base_template->class_template= type_template_ptr;
		the_class.base_template->template_args= deduced_template_args;
		the_class.base_template->signature_args= std::move(result_signature_args);

		template_classes_cache_[name_encoded]= class_proxy;
		result.type= template_args_namespace->GetThisScopeValue( Class::c_template_class_name );

		class_proxy->class_->llvm_type->setName( MangleType( class_proxy ) ); // Update llvm type name after setting base template.

		return result;
	}
	else if( type_template.syntax_element->kind_ == Synt::TypeTemplateBase::Kind::Typedef )
	{
		const Type type= PrepareType( static_cast<const Synt::TypedefTemplate*>( type_template.syntax_element )->typedef_->value, *template_args_namespace, *global_function_context_ );

		if( type == invalid_type_ )
			return result;

		result.type= template_args_namespace->AddName( Class::c_template_class_name, Value( type, file_pos /* TODO - check file_pos */ ) );
		return result;
	}
	else U_ASSERT(false);

	return result;
}

const FunctionVariable* CodeBuilder::GenTemplateFunction(
	CodeBuilderErrorsContainer& errors_container,
	const FilePos& file_pos,
	const FunctionTemplatePtr& function_template_ptr,
	const ArgsVector<Function::Arg>& actual_args,
	const bool first_actual_arg_is_this,
	const bool skip_arguments )
{
	const FunctionTemplate& function_template= *function_template_ptr;
	const Synt::Function& function_declaration= *function_template.syntax_element->function_;
	const std::string& func_name= function_declaration.name_.back();

	NamesScope& template_names_scope= *function_template.parent_namespace;

	const Function::Arg* given_args= actual_args.data();
	size_t given_arg_count= actual_args.size();

	if( first_actual_arg_is_this &&
		!function_declaration.type_.arguments_.empty() && function_declaration.type_.arguments_.front().name_ != Keywords::this_ )
	{
		++given_args;
		--given_arg_count;
	}

	if( !skip_arguments && given_arg_count != function_declaration.type_.arguments_.size() )
		return nullptr;

	const auto template_args_namespace= std::make_shared<NamesScope>( NamesScope::c_template_args_namespace_name, &template_names_scope );

	for( size_t i= 0u; i < function_template.known_template_args.size(); ++i )
	{
		const TemplateArg& arg= function_template.known_template_args[i];

		Value v;
		if( const auto type= std::get_if<Type>( &arg ) ) v= Value( *type, file_pos );
		else if( const auto variable= std::get_if<Variable>( &arg ) ) v= Value( *variable, file_pos );
		else U_ASSERT(false);

		template_args_namespace->AddName( function_template.template_params[i].name, std::move(v) );
	}

	for( size_t i= function_template.known_template_args.size(); i < function_template.template_params.size(); ++i )
		template_args_namespace->AddName( function_template.template_params[i].name, YetNotDeducedTemplateArg() );

	for( size_t i= 0u; i < function_declaration.type_.arguments_.size() && !skip_arguments; ++i )
	{
		const Synt::FunctionArgument& function_argument= function_declaration.type_.arguments_[i];

		const bool expected_arg_is_mutalbe_reference=
			function_argument.mutability_modifier_ == Synt::MutabilityModifier::Mutable &&
			( function_argument.reference_modifier_ == Synt::ReferenceModifier::Reference || function_argument.name_ == Keywords::this_ );

		// Functin arg declared as "mut&", but given something immutable.
		if( expected_arg_is_mutalbe_reference && !given_args[i].is_mutable )
			return nullptr;

		const DeducedTemplateParameter& signature_param= function_template.signature_params_new[i];
		const Type& given_type= given_args[i].type;

		bool deduced_specially= false;
		if( const auto type_param = signature_param.GetType() )
		{
			const Type& type= type_param->t;
			if( type == given_type || ReferenceIsConvertible( given_type, type, errors_container, file_pos ) ||
				( !expected_arg_is_mutalbe_reference && GetConversionConstructor( given_type, type, errors_container, file_pos ) != nullptr ) )
				deduced_specially= true;
		}
		else if( const auto template_param= signature_param.GetTemplateParameter() )
		{
			if( const auto type= template_args_namespace->GetThisScopeValue( function_template.template_params[ template_param->index ].name )->GetTypeName() )
			{
				if( *type == given_type || ReferenceIsConvertible( given_type, *type, errors_container, file_pos ) ||
					( !expected_arg_is_mutalbe_reference && GetConversionConstructor( given_type, *type, errors_container, file_pos ) != nullptr ) )
					deduced_specially= true;
			}
		}

		if( !deduced_specially && !MatchTemplateArg( function_template, *template_args_namespace, given_type, file_pos, signature_param ) )
			return nullptr;

	} // for template function arguments

	TemplateArgs args_for_mangle;
	for( size_t i= 0u; i < function_template.template_params.size() ; ++i )
	{
		const Value* const value= template_args_namespace->GetThisScopeValue( function_template.template_params[i].name );

		if( const auto type= value->GetTypeName() )
			args_for_mangle.push_back( *type );
		else if( const auto variable= value->GetVariable() )
			args_for_mangle.push_back( *variable );
		else
		{
			// SPRACHE_TODO - maybe not generate this error?
			// Other function templates, for example, can match given aruments.
			REPORT_ERROR( TemplateParametersDeductionFailed, template_args_namespace->GetErrors(), file_pos );
			return nullptr;
		}
	}

	// Encode name for caching. Name must be unique for each template and its parameters.
	const std::string name_encoded=
		std::to_string( reinterpret_cast<uintptr_t>( function_template.parent != nullptr ? function_template.parent.get() : &function_template ) ) + // Encode template address, because we needs unique keys for templates with same name.
		MangleTemplateArgs( args_for_mangle );

	{
		const auto it= generated_template_things_storage_.find( name_encoded );
		if( it != generated_template_things_storage_.end() )
		{
			//Function for this template arguments already generated.
			const NamesScopePtr template_parameters_space= it->second.GetNamespace();
			U_ASSERT( template_parameters_space != nullptr );
			OverloadedFunctionsSet& result_functions_set= *template_parameters_space->GetThisScopeValue( func_name )->GetFunctionsSet();
			if( result_functions_set.functions.size() >= 1u )
				return &result_functions_set.functions.front();
			else
				return nullptr; // May be in case of error or in case of "enable_if".
		}
	}
	generated_template_things_storage_.insert( std::make_pair( name_encoded, Value( template_args_namespace, function_declaration.file_pos_ ) ) );

	CreateTemplateErrorsContext( errors_container, file_pos, template_args_namespace, function_template, func_name, args_for_mangle );

	// First, prepare only as prototype.
	NamesScopeFill( function_template.syntax_element->function_, *template_args_namespace, function_template.base_class );
	OverloadedFunctionsSet& result_functions_set= *template_args_namespace->GetThisScopeValue( func_name )->GetFunctionsSet();
	GlobalThingBuildFunctionsSet( *template_args_namespace, result_functions_set, false );

	if( result_functions_set.functions.empty() )
		return nullptr; // Function prepare failed

	FunctionVariable& function_variable= result_functions_set.functions.front();
	function_variable.deduced_temlpate_parameters= function_template.signature_params_new;

	// And generate function body after insertion of prototype.
	if( !function_variable.have_body ) // if function is constexpr, body may be already generated.
		BuildFuncCode(
			function_variable,
			function_template.base_class,
			*template_args_namespace,
			function_template.syntax_element->function_->name_.back(),
			function_template.syntax_element->function_->type_.arguments_,
			function_template.syntax_element->function_->block_.get(),
			function_template.syntax_element->function_->constructor_initialization_list_.get() );

	// Set correct mangled name
	if( function_variable.llvm_function != nullptr )
	{
		const std::string mangled_name =
			MangleFunction(
				template_names_scope,
				func_name,
				*function_variable.type.GetFunctionType(),
				&args_for_mangle );
		function_variable.llvm_function->setName( mangled_name );
	}

	// Two-step preparation needs for recursive function template call.

	return &function_variable;
}

Value* CodeBuilder::GenTemplateFunctionsUsingTemplateParameters(
	const FilePos& file_pos,
	const std::vector<FunctionTemplatePtr>& function_templates,
	const std::vector<Synt::Expression>& template_arguments,
	NamesScope& arguments_names_scope,
	FunctionContext& function_context )
{
	U_ASSERT( !function_templates.empty() );

	TemplateArgs template_args;
	for( const Synt::Expression& expr : template_arguments )
	{
		const Value value= BuildExpressionCode( expr, arguments_names_scope, function_context );
		if( const auto type_name= value.GetTypeName() )
			template_args.push_back( *type_name );
		else if( const auto variable= value.GetVariable() )
		{
			if( !TypeIsValidForTemplateVariableArgument( variable->type ) )
				REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, arguments_names_scope.GetErrors(), Synt::GetExpressionFilePos(expr), variable->type );
			else if( variable->constexpr_value == nullptr )
				REPORT_ERROR( ExpectedConstantExpression, arguments_names_scope.GetErrors(), Synt::GetExpressionFilePos(expr) );
			else
				template_args.push_back( *variable );
		}
		else
			REPORT_ERROR( InvalidValueAsTemplateArgument, arguments_names_scope.GetErrors(), file_pos, value.GetKindName() );

	} // for given template arguments.

	if( template_args.size() != template_arguments.size() )
		return nullptr;

	// We needs unique name here, so use for it address of function templates set and template parameters.
	std::string name_encoded= "</.../>";
	for( const FunctionTemplatePtr& template_ : function_templates )
	{
		name_encoded+= std::to_string( reinterpret_cast<uintptr_t>( &template_ ) );
		name_encoded+= "_";
	}
	name_encoded+= MangleTemplateArgs( template_args );

	{
		const auto it= generated_template_things_storage_.find( name_encoded );
		if( it != generated_template_things_storage_.end() )
			return &it->second; // Already generated.
	}

	OverloadedFunctionsSet result;
	for( const FunctionTemplatePtr& function_template_ptr : function_templates )
	{
		const FunctionTemplate& function_template= *function_template_ptr;
		if( template_args.size() > function_template.template_params.size() )
			continue;


		NamesScope args_names_scope("", function_template.parent_namespace );
		for( const TemplateBase::TemplateParameter& param : function_template.template_params )
			args_names_scope.AddName( param.name, YetNotDeducedTemplateArg() );

		bool ok= true;
		for( size_t i= 0u; i < template_args.size(); ++i )
		{
			ok&= MatchTemplateArg(
				function_template,
				args_names_scope,
				template_args[i],
				Synt::GetExpressionFilePos( *function_template.syntax_element->args_[i].name_expr ),
				DeducedTemplateParameter::TemplateParameter{ i } );
		}

		if( !ok )
			continue;

		const auto new_template= std::make_shared<FunctionTemplate>();
		// Reduce count of template arguments in new function template.
		new_template->template_params= function_template.template_params;

		new_template->parent_namespace= function_template.parent_namespace;
		new_template->file_pos= function_template.file_pos;
		new_template->syntax_element= function_template.syntax_element;
		new_template->base_class= function_template.base_class;
		new_template->signature_params_new= function_template.signature_params_new;
		new_template->params_types= function_template.params_types;
		new_template->parent= function_template_ptr;

		new_template->known_template_args= template_args;
		new_template->known_template_args.resize( std::min( new_template->known_template_args.size(), function_template.template_params.size() ) );

		result.template_functions.push_back( new_template );
	} // for function templates

	if( result.template_functions.empty() )
	{
		REPORT_ERROR( TemplateFunctionGenerationFailed, arguments_names_scope.GetErrors(), file_pos, function_templates.front()->syntax_element->function_->name_.back() );
		return nullptr;
	}

	return & generated_template_things_storage_.insert( std::make_pair( name_encoded, result ) ).first->second;
}

bool CodeBuilder::NameShadowsTemplateArgument( const std::string& name, NamesScope& names_scope )
{
	// Not implemented correctly yet.
	U_UNUSED(name);
	U_UNUSED(names_scope);
	return false;
}

bool CodeBuilder::TypeIsValidForTemplateVariableArgument( const Type& type )
{
	if( const FundamentalType* const fundamental= type.GetFundamentalType() )
	{
		if( IsInteger( fundamental->fundamental_type ) ||
			IsChar( fundamental->fundamental_type ) ||
			fundamental->fundamental_type == U_FundamentalType::Bool )
			return true;
	}
	if( type.GetEnumType() != nullptr )
	{
		U_ASSERT( TypeIsValidForTemplateVariableArgument( type.GetEnumType()->underlaying_type ) );
		return true;
	}

	return false;
}

void CodeBuilder::ReportAboutIncompleteMembersOfTemplateClass( const FilePos& file_pos, Class& class_ )
{
	class_.members.ForEachInThisScope(
		[this, file_pos, &class_]( const std::string& name, const Value& value )
		{
			if( const Type* const type= value.GetTypeName() )
			{
				if( Class* const subclass= type->GetClassType() )
				{
					if( !subclass->is_complete )
						REPORT_ERROR( IncompleteMemberOfClassTemplate, class_.members.GetErrors(), file_pos, name );
					else
						ReportAboutIncompleteMembersOfTemplateClass( file_pos, *subclass );
				}
			}
			else if( const OverloadedFunctionsSet* const functions_set= value.GetFunctionsSet() )
			{
				for( const FunctionVariable& function : functions_set->functions )
				{
					if( !function.have_body )
						REPORT_ERROR( IncompleteMemberOfClassTemplate, class_.members.GetErrors(), file_pos, name );
				}
			}
			else if( value.GetClassField() != nullptr ) {}
			else if( value.GetTypeTemplatesSet() != nullptr ) {}
			else if( value.GetVariable() != nullptr ) {}
			else U_ASSERT(false);
		} );
}

} // namespace CodeBuilderPrivate

} // namespace U
