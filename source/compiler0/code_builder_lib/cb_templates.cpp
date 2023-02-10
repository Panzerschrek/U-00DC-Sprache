﻿#include <algorithm>

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/GlobalVariable.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"

namespace U
{

namespace
{

void CreateTemplateErrorsContext(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const NamesScopePtr& template_parameters_namespace,
	const TemplateBase& template_,
	const std::string& template_name,
	const TemplateArgs& template_args )
{
	REPORT_ERROR( TemplateContext, errors_container, src_loc );
	const auto template_error_context= std::make_shared<TemplateErrorsContext>();
	template_error_context->context_declaration_src_loc= template_.src_loc;
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
			else if( const auto variable_ptr= std::get_if<VariablePtr>( &arg ) )
				args_description+= ConstantVariableToString( **variable_ptr );
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
	const Synt::TypeTemplate& type_template_declaration,
	TypeTemplatesSet& type_templates_set,
	NamesScope& names_scope )
{
	/* SPRACHE_TODO:
	 *) Support default template arguments for short form.
	 *) Add "enable_if".
	 *) Support template-dependent types for value parameters, such template</ type T, U</ T /> ut />.
	*/

	const auto type_template= std::make_shared<TypeTemplate>();

	type_template->parent_namespace= &names_scope;
	type_template->syntax_element= &type_template_declaration;
	type_template->src_loc= type_template_declaration.src_loc_;

	std::vector<TypeTemplate::TemplateParameter>& template_parameters= type_template->template_params;
	template_parameters.reserve( type_template_declaration.params_.size() );
	std::vector<bool> template_parameters_usage_flags;

	ProcessTemplateParams(
		type_template_declaration.params_,
		names_scope,
		type_template_declaration.src_loc_,
		template_parameters,
		template_parameters_usage_flags );

	if( type_template_declaration.is_short_form_ )
	{
		U_ASSERT( type_template_declaration.signature_params_.empty() );
		// Assign template params to signature params.
		for( size_t i= 0u; i < type_template_declaration.params_.size(); ++i )
		{
			type_template->signature_params.push_back( TemplateSignatureParam::TemplateParam{ i } );
			template_parameters_usage_flags[i]= true;
		}

		type_template->first_optional_signature_param= type_template->signature_params.size();
	}
	else
	{
		// Check and fill signature args.
		type_template->first_optional_signature_param= 0u;
		for( const Synt::TypeTemplate::SignatureParam& signature_param : type_template_declaration.signature_params_ )
		{
			type_template->signature_params.push_back(
				CreateTemplateSignatureParameter( names_scope, *global_function_context_, template_parameters, template_parameters_usage_flags, signature_param.name ) );

			if( std::get_if<Synt::EmptyVariant>( &signature_param.default_value ) == nullptr )
			{
				CreateTemplateSignatureParameter( names_scope, *global_function_context_, template_parameters, template_parameters_usage_flags, signature_param.default_value );
			}
			else
			{
				const size_t index= type_template->signature_params.size() - 1u;
				if (index > type_template->first_optional_signature_param )
					REPORT_ERROR( MandatoryTemplateSignatureArgumentAfterOptionalArgument, names_scope.GetErrors(), type_template_declaration.src_loc_ );

				++type_template->first_optional_signature_param;
			}
		}
	}
	U_ASSERT( type_template->first_optional_signature_param <= type_template->signature_params.size() );

	for( size_t i= 0u; i < type_template->template_params.size(); ++i )
		if( !template_parameters_usage_flags[i] )
			REPORT_ERROR( TemplateArgumentNotUsedInSignature, names_scope.GetErrors(), type_template_declaration.src_loc_, type_template->template_params[i].name );

	// Check for redefinition and insert.
	for( const TypeTemplatePtr& prev_template : type_templates_set.type_templates )
	{
		if(type_template->signature_params == prev_template->signature_params )
		{
			REPORT_ERROR( TypeTemplateRedefinition, names_scope.GetErrors(), type_template_declaration.src_loc_, type_template_declaration.name_ );
			return;
		}
	}
	type_templates_set.type_templates.push_back( type_template );
}

void CodeBuilder::PrepareFunctionTemplate(
	const Synt::FunctionTemplate& function_template_declaration,
	OverloadedFunctionsSet& functions_set,
	NamesScope& names_scope,
	const ClassPtr& base_class )
{
	const auto& full_name= function_template_declaration.function_->name_;
	const std::string& function_template_name= full_name.front();

	if( full_name.size() > 1u )
		REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), function_template_declaration.src_loc_ );

	if( function_template_declaration.function_->block_ == nullptr )
		REPORT_ERROR( IncompleteMemberOfClassTemplate, names_scope.GetErrors(), function_template_declaration.src_loc_, function_template_name );
	if( function_template_declaration.function_->virtual_function_kind_ != Synt::VirtualFunctionKind::None )
		REPORT_ERROR( VirtualForFunctionTemplate, names_scope.GetErrors(), function_template_declaration.src_loc_, function_template_name );

	const auto function_template= std::make_shared<FunctionTemplate>();
	function_template->syntax_element= &function_template_declaration;
	function_template->src_loc= function_template_declaration.src_loc_;
	function_template->parent_namespace= &names_scope;
	function_template->base_class= base_class;

	std::vector<bool> template_parameters_usage_flags; // Currently unused, because function template have no signature.

	ProcessTemplateParams(
		function_template_declaration.params_,
		names_scope,
		function_template_declaration.src_loc_,
		function_template->template_params,
		template_parameters_usage_flags );

	for( const Synt::FunctionParam& function_param : function_template_declaration.function_->type_.params_ )
	{
		if( base_class != nullptr && function_param.name_ == Keyword( Keywords::this_ ) )
			function_template->signature_params.push_back( TemplateSignatureParam::TypeParam{ Type(base_class) } );
		else
		{
			function_template->signature_params.push_back(
				CreateTemplateSignatureParameter( names_scope, *global_function_context_, function_template->template_params, template_parameters_usage_flags, function_param.type_ ) );
		}
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

void CodeBuilder::ProcessTemplateParams(
	const std::vector<Synt::TemplateBase::Param>& params,
	NamesScope& names_scope,
	const SrcLoc& src_loc,
	std::vector<TypeTemplate::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags )
{
	U_ASSERT( template_parameters.empty() );
	U_ASSERT( template_parameters_usage_flags.empty() );

	// Check and fill template parameters.
	for( const Synt::TemplateBase::Param& param : params )
	{
		// Check redefinition
		for( const auto& prev_param : template_parameters )
		{
			if( prev_param.name == param.name )
			{
				REPORT_ERROR( Redefinition, names_scope.GetErrors(), src_loc, param.name );
				continue;
			}
		}

		template_parameters.emplace_back();
		template_parameters.back().name= param.name;
		template_parameters_usage_flags.push_back(false);
	}

	U_ASSERT( template_parameters_usage_flags.size() == template_parameters.size() );

	for( size_t i= 0u; i < template_parameters.size(); ++i )
	{
		if( params[i].param_type == std::nullopt )
			continue;

		template_parameters[i].type=
			CreateTemplateSignatureParameter(
				names_scope,
				*global_function_context_,
				template_parameters,
				template_parameters_usage_flags,
				*params[i].param_type );

		if( const auto type_param= template_parameters[i].type->GetType() )
		{
			if( !TypeIsValidForTemplateVariableArgument( type_param->t ) )
				REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, names_scope.GetErrors(), src_loc, type_param->t );
		}
		else if( template_parameters[i].type->IsTemplateParam() ) {}
		else
			REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), src_loc, *params[i].param_type );
	}
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameter(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::vector<TemplateBase::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags,
	const Synt::ComplexName& signature_parameter )
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

				return TemplateSignatureParam::TemplateParam{ param_index };
			}
		}
	}

	const Value start_value= ResolveForTemplateSignatureParameter( signature_parameter, names_scope );
	if( const auto type_templates_set= start_value.GetTypeTemplatesSet() )
	{
		const Synt::ComplexName::Component* name_component= signature_parameter.tail.get();
		if( name_component == nullptr )
		{
			REPORT_ERROR( TemplateInstantiationRequired, names_scope.GetErrors(), signature_parameter.src_loc_, "TODO: template name" );
			return TemplateSignatureParam::TypeParam();
		}

		while( name_component->next != nullptr )
			name_component= name_component->next.get();

		const auto last_template_parameters= std::get_if< std::vector<Synt::Expression> >( &name_component->name_or_template_paramenters );
		if( last_template_parameters == nullptr )
		{
			REPORT_ERROR( TemplateInstantiationRequired, names_scope.GetErrors(), signature_parameter.src_loc_, "TODO:  template name" );
			return TemplateSignatureParam::TypeParam();
		}

		TemplateSignatureParam::SpecializedTemplateParam specialized_template;

		bool all_args_are_known= true;
		for( const Synt::Expression& template_parameter : *last_template_parameters )
		{
			specialized_template.params.push_back( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, template_parameter ) );
			all_args_are_known&= specialized_template.params.back().IsType() || specialized_template.params.back().IsVariable();
		}

		if( all_args_are_known )
			return ValueToTemplateParam( ResolveValue( names_scope, function_context, signature_parameter ), names_scope );

		specialized_template.type_templates= type_templates_set->type_templates;

		return specialized_template;
	}

	return ValueToTemplateParam( ResolveValue( names_scope, function_context, signature_parameter ), names_scope );
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameter(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::vector<TemplateBase::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags,
	const Synt::Expression& template_parameter )
{
	if( const auto named_operand= std::get_if<Synt::ComplexName>( &template_parameter ) )
		return CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, *named_operand );
	else if( const auto array_type_name= std::get_if<Synt::ArrayTypeName>(&template_parameter) )
		return CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, *array_type_name );
	else if( const auto tuple_type_name= std::get_if<Synt::TupleType>(&template_parameter) )
		return CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, *tuple_type_name );
	else if( const auto raw_pointer_type_name= std::get_if<Synt::RawPointerType>(&template_parameter) )
		return CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, *raw_pointer_type_name );
	else if( const auto function_pointer_type_name_ptr= std::get_if<Synt::FunctionTypePtr>(&template_parameter) )
		return CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, *function_pointer_type_name_ptr );

	return ValueToTemplateParam( BuildExpressionCode( template_parameter, names_scope, function_context ), names_scope );
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameter(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::vector<TemplateBase::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags,
	const Synt::TypeName& type_name_template_parameter )
{
	return
		std::visit(
			[&]( const auto& t ) { return CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, t ); },
			type_name_template_parameter );
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameter(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::vector<TemplateBase::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags,
	const Synt::EmptyVariant& empty_variant )
{
	U_ASSERT(false);
	U_UNUSED(empty_variant);
	U_UNUSED(names_scope);
	U_UNUSED(function_context);
	U_UNUSED(template_parameters);
	U_UNUSED(template_parameters_usage_flags);
	return TemplateSignatureParam::TypeParam();
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameter(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::vector<TemplateBase::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags,
	const Synt::ArrayTypeName& array_type_name )
{
	TemplateSignatureParam::ArrayParam array_param;
	array_param.element_count= std::make_unique<TemplateSignatureParam>( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, *array_type_name.size ) );
	array_param.element_type= std::make_unique<TemplateSignatureParam>( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, *array_type_name.element_type ) );

	if( array_param.element_count->IsVariable() && array_param.element_type->IsType() )
		return TemplateSignatureParam::TypeParam{ PrepareTypeImpl( names_scope, function_context, array_type_name ) };

	return std::move(array_param);
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameter(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::vector<TemplateBase::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags,
	const Synt::FunctionTypePtr& function_pointer_type_name_ptr )
{
	TemplateSignatureParam::FunctionParam function_param;

	bool all_types_are_known= true;

	const Synt::FunctionType& function_pointer_type_name= *function_pointer_type_name_ptr;

	if( function_pointer_type_name.return_value_reference_modifier_ == ReferenceModifier::None )
		function_param.return_value_type= ValueType::Value;
	else
		function_param.return_value_type= function_pointer_type_name.return_value_mutability_modifier_ == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;

	function_param.is_unsafe= function_pointer_type_name.unsafe_;
	function_param.calling_convention= GetLLVMCallingConvention(function_pointer_type_name.calling_convention_, function_pointer_type_name_ptr->src_loc_, names_scope.GetErrors() );

	// TODO - maybe check also reference tags?
	if( function_pointer_type_name.return_type_ != nullptr )
		function_param.return_type= std::make_unique<TemplateSignatureParam>( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, *function_pointer_type_name.return_type_ ) );
	else
		function_param.return_type= std::make_unique<TemplateSignatureParam>( TemplateSignatureParam::TypeParam{ void_type_ } );

	all_types_are_known&= function_param.return_type->IsType();

	for( const Synt::FunctionParam& in_param : function_pointer_type_name.params_ )
	{
		auto t= CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, in_param.type_ );
		all_types_are_known&= t.IsType();

		TemplateSignatureParam::FunctionParam::Param out_param;
		out_param.type= std::make_unique<TemplateSignatureParam>( std::move(t) );
		if( in_param.reference_modifier_ == Synt::ReferenceModifier::None )
			out_param.value_type= ValueType::Value;
		else if( in_param.mutability_modifier_ == Synt::MutabilityModifier::Mutable )
			out_param.value_type= ValueType::ReferenceMut;
		else
			out_param.value_type= ValueType::ReferenceImut;

		function_param.params.push_back( std::move(out_param) );
	}

	if( all_types_are_known )
		return TemplateSignatureParam::TypeParam{ PrepareTypeImpl( names_scope, function_context, function_pointer_type_name_ptr ) };

	return std::move(function_param);
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameter(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::vector<TemplateBase::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags,
	const Synt::TupleType& tuple_type_name )
{
	TemplateSignatureParam::TupleParam tuple_param;

	bool all_types_are_known= true;
	for( const auto& element_type : tuple_type_name.element_types_ )
	{
		tuple_param.element_types.push_back( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, element_type ) );
		all_types_are_known&= tuple_param.element_types.back().IsType();
	}

	if( all_types_are_known )
		return TemplateSignatureParam::TypeParam{ PrepareTypeImpl( names_scope, function_context, tuple_type_name ) };

	return tuple_param;
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameter(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::vector<TemplateBase::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags,
	const Synt::RawPointerType& raw_pointer_type_name )
{
	TemplateSignatureParam::RawPointerParam raw_pointer_param;
	raw_pointer_param.element_type=
		std::make_unique<TemplateSignatureParam>(
			CreateTemplateSignatureParameter(
				names_scope,
				function_context,
				template_parameters,
				template_parameters_usage_flags,
				*raw_pointer_type_name.element_type ) );

	if( raw_pointer_param.element_type->IsType() )
		return TemplateSignatureParam::TypeParam{ PrepareTypeImpl( names_scope, function_context, raw_pointer_type_name ) };

	return raw_pointer_param;
}

TemplateSignatureParam CodeBuilder::ValueToTemplateParam( const Value& value, NamesScope& names_scope )
{
	if( const auto type= value.GetTypeName() )
		return TemplateSignatureParam::TypeParam{ *type };

	if( const auto variable= value.GetVariable() )
	{
		if( !TypeIsValidForTemplateVariableArgument( variable->type ) )
		{
			REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, names_scope.GetErrors(), value.GetSrcLoc(), variable->type );
			return TemplateSignatureParam::TypeParam();
		}
		if( variable->constexpr_value == nullptr )
		{
			REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), value.GetSrcLoc() );
			return TemplateSignatureParam::TypeParam();
		}
		return TemplateSignatureParam::VariableParam{ variable->type, variable->constexpr_value };
	}

	REPORT_ERROR( InvalidValueAsTemplateArgument, names_scope.GetErrors(), value.GetSrcLoc(), invalid_type_ );
	return TemplateSignatureParam::TypeParam();
}

Value CodeBuilder::ResolveForTemplateSignatureParameter(
	const Synt::ComplexName& signature_parameter,
	NamesScope& names_scope )
{
	return ResolveValue( names_scope, *global_function_context_, signature_parameter, ResolveMode::ForTemplateSignatureParameter );
}

bool CodeBuilder::MatchTemplateArg(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const SrcLoc& src_loc,
	const TemplateSignatureParam& template_param )
{
	return
		template_param.Visit(
			[&]( const auto& param )
			{
				return MatchTemplateArgImpl( template_, args_names_scope, template_arg, src_loc, param );
			} );
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const SrcLoc& src_loc,
	const TemplateSignatureParam::TypeParam& template_param )
{
	(void)template_;
	(void)args_names_scope;
	(void)src_loc;

	if( const auto given_type= std::get_if<Type>( &template_arg ) )
		return *given_type == template_param.t;
	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const SrcLoc& src_loc,
	const TemplateSignatureParam::VariableParam& template_param )
{
	(void)template_;
	(void)args_names_scope;
	(void)src_loc;

	if( const auto given_variable_ptr_ptr= std::get_if<VariablePtr>( &template_arg ) )
	{
		const VariablePtr& given_variable= *given_variable_ptr_ptr;

		return
			given_variable->type == template_param.type &&
			given_variable->constexpr_value->getUniqueInteger() == template_param.constexpr_value->getUniqueInteger();
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const SrcLoc& src_loc,
	const TemplateSignatureParam::TemplateParam& template_param )
{
	const std::string& name= template_.template_params[ template_param.index ].name;

	Value* const value= args_names_scope.GetThisScopeValue( name );
	U_ASSERT( value != nullptr );
	if( value->GetYetNotDeducedTemplateArg() != nullptr )
	{
		const auto& param_type= template_.template_params[ template_param.index ].type;
		const bool is_variable_param= param_type != std::nullopt;

		if( const auto given_type= std::get_if<Type>( &template_arg ) )
		{
			if( is_variable_param )
				return false;

			*value= Value( *given_type, src_loc );
			return true;
		}
		if( const auto given_variable_ptr_ptr= std::get_if<VariablePtr>( &template_arg ) )
		{
			const VariablePtr& given_variable= *given_variable_ptr_ptr;

			if( !is_variable_param )
				return false;

			if( !TypeIsValidForTemplateVariableArgument( given_variable->type ) )
			{
				REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, args_names_scope.GetErrors(), src_loc, given_variable->type );
				return false;
			}
			if( given_variable->constexpr_value == nullptr )
			{
				REPORT_ERROR( ExpectedConstantExpression, args_names_scope.GetErrors(), src_loc );
				return false;
			}

			if( !MatchTemplateArg( template_, args_names_scope, given_variable->type, src_loc, *param_type ) )
				return false;

			const VariableMutPtr variable_for_insertion=
				std::make_shared<Variable>(
					given_variable->type,
					ValueType::ReferenceImut,
					Variable::Location::Pointer,
					name,
					CreateGlobalConstantVariable(
						given_variable->type,
						name,
						given_variable->constexpr_value ));
			variable_for_insertion->constexpr_value= given_variable->constexpr_value;

			*value= Value( variable_for_insertion, src_loc );
			return true;
		}
	}
	else if( const auto prev_type= value->GetTypeName() )
	{
		if( const auto given_type= std::get_if<Type>( &template_arg ) )
			return *given_type == *prev_type;
	}
	else if( const auto prev_variable= value->GetVariable() )
	{
		if( const auto given_variable_ptr_ptr= std::get_if<VariablePtr>( &template_arg ) )
		{
			const VariablePtr& given_variable= *given_variable_ptr_ptr;

			return
				given_variable->type == prev_variable->type &&
				given_variable->constexpr_value->getUniqueInteger() == prev_variable->constexpr_value->getUniqueInteger();
		}
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const SrcLoc& src_loc,
	const TemplateSignatureParam::ArrayParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_array_type= given_type->GetArrayType() )
		{
			if( !MatchTemplateArg( template_, args_names_scope, given_array_type->element_type, src_loc, *template_param.element_type ) )
				return false;

			const std::string name= "array_size " + std::to_string(given_array_type->element_count);

			const VariableMutPtr size_variable=
				std::make_shared<Variable>(
					size_type_,
					ValueType::ReferenceImut,
					Variable::Location::Pointer,
					name );

			size_variable->constexpr_value=
				llvm::ConstantInt::get(
					size_type_.GetLLVMType(),
					llvm::APInt(
						static_cast<unsigned int>(size_type_.GetFundamentalType()->GetSize() * 8),
						given_array_type->element_count ) );

			size_variable->llvm_value= CreateGlobalConstantVariable( size_type_, name, size_variable->constexpr_value );

			if( !MatchTemplateArg( template_, args_names_scope, size_variable, src_loc, *template_param.element_count ) )
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
	const SrcLoc& src_loc,
	const TemplateSignatureParam::TupleParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_tuple_type= given_type->GetTupleType() )
		{
			if( given_tuple_type->element_types.size() != template_param.element_types.size() )
				return false;

			for( size_t i= 0; i < template_param.element_types.size(); ++i )
			{
				if( !MatchTemplateArg( template_, args_names_scope, given_tuple_type->element_types[i], src_loc, template_param.element_types[i] ) )
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
	const SrcLoc& src_loc,
	const TemplateSignatureParam::RawPointerParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_raw_ponter_type= given_type->GetRawPointerType() )
		{
			return MatchTemplateArg( template_, args_names_scope, given_raw_ponter_type->element_type, src_loc, *template_param.element_type );
		}
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const SrcLoc& src_loc,
	const TemplateSignatureParam::FunctionParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_function_pointer_type= given_type->GetFunctionPointerType() )
		{
			const FunctionType& given_function_type= given_function_pointer_type->function_type;

			if( !(
				given_function_type.unsafe == template_param.is_unsafe &&
				given_function_type.calling_convention == template_param.calling_convention &&
				given_function_type.return_value_type == template_param.return_value_type &&
				MatchTemplateArg( template_, args_names_scope, given_function_type.return_type, src_loc, *template_param.return_type ) &&
				given_function_type.params.size() == template_param.params.size()
				) )
				return false;

			for( size_t i= 0; i < template_param.params.size(); ++i )
			{
				if( !(
					given_function_type.params[i].value_type == template_param.params[i].value_type &&
					MatchTemplateArg( template_, args_names_scope, given_function_type.params[i].type, src_loc, *template_param.params[i].type )
					) )
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
	const SrcLoc& src_loc,
	const TemplateSignatureParam::SpecializedTemplateParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_class_type= given_type->GetClassType() )
		{
			const Class& given_class= *given_class_type;

			if( !(
					given_class.base_template != std::nullopt &&
					std::find(
						template_param.type_templates.begin(),
						template_param.type_templates.end(),
						given_class.base_template->class_template ) != template_param.type_templates.end() &&
					template_param.params.size() == given_class.base_template->signature_args.size()
				) )
				return false;

			for( size_t i= 0; i < template_param.params.size(); ++i )
			{
				if( !MatchTemplateArg( template_, args_names_scope, given_class.base_template->signature_args[i], src_loc, template_param.params[i] ) )
					return false;
			}

			return true;
		}
	}

	return false;
}

Value* CodeBuilder::GenTemplateType(
	const SrcLoc& src_loc,
	const TypeTemplatesSet& type_templates_set,
	const std::vector<Synt::Expression>& template_arguments,
	NamesScope& arguments_names_scope,
	FunctionContext& function_context )
{
	std::vector<Value> arguments_calculated;
	arguments_calculated.reserve( template_arguments.size() );

	{
		const bool prev_is_functionless_context= function_context.is_functionless_context;
		function_context.is_functionless_context= true;

		const StackVariablesStorage dummy_stack_variables_storage( function_context );

		for( const Synt::Expression& expr : template_arguments )
			arguments_calculated.push_back( BuildExpressionCode( expr, arguments_names_scope, function_context ) );

		DestroyUnusedTemporaryVariables( function_context, arguments_names_scope.GetErrors(), src_loc );
		function_context.is_functionless_context= prev_is_functionless_context;
	}

	std::vector<TemplateTypePreparationResult> prepared_types;
	for( const TypeTemplatePtr& type_template : type_templates_set.type_templates )
	{
		TemplateTypePreparationResult generated_type=
			PrepareTemplateType(
				src_loc,
				type_template,
				arguments_calculated,
				arguments_names_scope );
		if( generated_type.type_template != nullptr )
			prepared_types.push_back( std::move(generated_type) );
	}

	if( prepared_types.empty() )
	{
		REPORT_ERROR( TemplateParametersDeductionFailed, arguments_names_scope.GetErrors(), src_loc );
		return nullptr;
	}

	if( const auto selected_template= SelectTemplateType( prepared_types, template_arguments.size() ) )
		return FinishTemplateTypeGeneration( src_loc, arguments_names_scope, *selected_template );

	REPORT_ERROR( CouldNotSelectMoreSpicializedTypeTemplate, arguments_names_scope.GetErrors(), src_loc );
	return nullptr;
}

CodeBuilder::TemplateTypePreparationResult CodeBuilder::PrepareTemplateType(
	const SrcLoc& src_loc,
	const TypeTemplatePtr& type_template_ptr,
	const std::vector<Value>& template_arguments,
	NamesScope& arguments_names_scope )
{
	// This method does not generate some errors, because instantiation may fail
	// for one class template, but success for other.

	TemplateTypePreparationResult result;

	const TypeTemplate& type_template= *type_template_ptr;
	NamesScope& template_names_scope= *type_template.parent_namespace;

	if( template_arguments.size() < type_template.first_optional_signature_param ||
		template_arguments.size() > type_template.signature_params.size() )
		return result;

	result.template_args_namespace= std::make_shared<NamesScope>( NamesScope::c_template_args_namespace_name, &template_names_scope );
	for( const TypeTemplate::TemplateParameter& param : type_template.template_params )
		result.template_args_namespace->AddName( param.name, YetNotDeducedTemplateArg() );

	result.signature_args.resize( type_template.signature_params.size() );
	for( size_t i= 0u; i < type_template.signature_params.size(); ++i )
	{
		const Value value= i < template_arguments.size()
			? template_arguments[i]
			: BuildExpressionCode( type_template.syntax_element->signature_params_[i].default_value, *result.template_args_namespace, *global_function_context_ );

		if( const Type* const type_name= value.GetTypeName() )
			result.signature_args[i]= *type_name;
		else if( const auto variable= value.GetVariable() )
			result.signature_args[i]= variable;
		else
		{
			REPORT_ERROR( InvalidValueAsTemplateArgument, arguments_names_scope.GetErrors(), src_loc, value.GetKindName() );
			continue;
		}

		if( !MatchTemplateArg( type_template, *result.template_args_namespace, result.signature_args[i], src_loc, type_template.signature_params[i] ) )
			return result;
	} // for signature arguments

	for( const auto& template_param : type_template.template_params )
	{
		const Value* const value= result.template_args_namespace->GetThisScopeValue( template_param.name );

		if( const auto type= value->GetTypeName() )
			result.template_args.push_back( *type );
		else if( const auto variable= value->GetVariable() )
			result.template_args.push_back( variable );
		else
		{
			// SPRACHE_TODO - maybe not generate this error?
			// Other function templates, for example, can match given aruments.
			REPORT_ERROR( TemplateParametersDeductionFailed, arguments_names_scope.GetErrors(), src_loc );
			return result;
		}
	}

	result.type_template= type_template_ptr;

	return result;
}

Value* CodeBuilder::FinishTemplateTypeGeneration(
	const SrcLoc& src_loc,
	NamesScope& arguments_names_scope,
	const TemplateTypePreparationResult& template_type_preparation_result )
{
	const TypeTemplatePtr& type_template_ptr= template_type_preparation_result.type_template;
	const TypeTemplate& type_template= *type_template_ptr;
	const NamesScopePtr& template_args_namespace= template_type_preparation_result.template_args_namespace;

	// Encode name for caching. Name must be unique for each template and its parameters.
	const std::string name_encoded=
		std::to_string( reinterpret_cast<uintptr_t>( &type_template ) ) + // Encode template address, because we needs unique keys for templates with same name.
		mangler_->MangleTemplateArgs( template_type_preparation_result.signature_args );

	// Check, if already type generated.
	if( const auto it= generated_template_things_storage_.find( name_encoded ); it != generated_template_things_storage_.end() )
	{
		const NamesScopePtr template_parameters_space= it->second.GetNamespace();
		U_ASSERT( template_parameters_space != nullptr );
		return template_parameters_space->GetThisScopeValue( Class::c_template_class_name );
	}
	AddNewTemplateThing( name_encoded, Value( template_args_namespace, type_template.syntax_element->src_loc_ ) );

	CreateTemplateErrorsContext(
		arguments_names_scope.GetErrors(),
		src_loc,
		template_args_namespace,
		type_template,
		type_template.syntax_element->name_,
		template_type_preparation_result.template_args );

	if( const auto class_ptr= std::get_if<Synt::ClassPtr>( &type_template.syntax_element->something_ ) )
	{
		U_ASSERT( (*class_ptr)->name_ == Class::c_template_class_name );

		if( const auto cache_class_it= template_classes_cache_.find( name_encoded ); cache_class_it != template_classes_cache_.end() )
		{
			return
				template_args_namespace->AddName(
					Class::c_template_class_name,
					Value( cache_class_it->second, type_template.syntax_element->src_loc_ /* TODO - check src_loc */ ) );
		}

		const ClassPtr class_type= NamesScopeFill( *template_args_namespace, *class_ptr );
		if( class_type == nullptr )
			return nullptr;

		Class& the_class= *class_type;
		// Save in class info about its base template.
		the_class.base_template.emplace();
		the_class.base_template->class_template= type_template_ptr;
		the_class.base_template->signature_args= template_type_preparation_result.signature_args;

		template_classes_cache_[name_encoded]= class_type;

		class_type->llvm_type->setName( mangler_->MangleType( class_type ) ); // Update llvm type name after setting base template.

		return template_args_namespace->GetThisScopeValue( Class::c_template_class_name );
	}
	if( const auto type_alias= std::get_if< std::unique_ptr<Synt::TypeAlias> >( &type_template.syntax_element->something_ ) )
	{
		const Type type= PrepareType( (*type_alias)->value, *template_args_namespace, *global_function_context_ );

		if( type == invalid_type_ )
			return nullptr;

		return template_args_namespace->AddName( Class::c_template_class_name, Value( type, src_loc /* TODO - check src_loc */ ) );
	}
	else U_ASSERT(false);

	return nullptr;
}

const FunctionVariable* CodeBuilder::GenTemplateFunction(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const FunctionTemplatePtr& function_template_ptr,
	const ArgsVector<FunctionType::Param>& actual_args,
	const bool first_actual_arg_is_this )
{
	const auto res= PrepareTemplateFunction( errors_container, src_loc, function_template_ptr, actual_args, first_actual_arg_is_this );
	if( res.function_template != nullptr )
		return FinishTemplateFunctionGeneration( errors_container, src_loc, res );
	return nullptr;
}

CodeBuilder::TemplateFunctionPreparationResult CodeBuilder::PrepareTemplateFunction(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const FunctionTemplatePtr& function_template_ptr,
	const ArgsVector<FunctionType::Param>& actual_args,
	const bool first_actual_arg_is_this )
{
	const FunctionTemplate& function_template= *function_template_ptr;
	const Synt::Function& function_declaration= *function_template.syntax_element->function_;

	const FunctionType::Param* given_args= actual_args.data();
	size_t given_arg_count= actual_args.size();

	if( first_actual_arg_is_this &&
		!function_declaration.type_.params_.empty() && function_declaration.type_.params_.front().name_ != Keywords::this_ )
	{
		++given_args;
		--given_arg_count;
	}

	TemplateFunctionPreparationResult result;

	if( given_arg_count != function_declaration.type_.params_.size() )
		return result;

	result.template_args_namespace= std::make_shared<NamesScope>( NamesScope::c_template_args_namespace_name, function_template.parent_namespace );

	for( size_t i= 0u; i < function_template.template_params.size(); ++i )
	{
		Value v;
		if( i < function_template.known_template_args.size() )
			v= std::visit( [&]( const auto& x ){ return Value( x, src_loc ); }, function_template.known_template_args[i] );
		else
			v= YetNotDeducedTemplateArg();

		result.template_args_namespace->AddName( function_template.template_params[i].name, std::move(v) );
	}

	for( size_t i= 0u; i < function_declaration.type_.params_.size(); ++i )
	{
		const Synt::FunctionParam& function_param= function_declaration.type_.params_[i];

		const bool expected_arg_is_mutalbe_reference=
			function_param.mutability_modifier_ == Synt::MutabilityModifier::Mutable &&
			( function_param.reference_modifier_ == Synt::ReferenceModifier::Reference || function_param.name_ == Keywords::this_ );

		// Functin arg declared as "mut&", but given something immutable.
		if( expected_arg_is_mutalbe_reference && given_args[i].value_type != ValueType::ReferenceMut )
			return result;

		const TemplateSignatureParam& signature_param= function_template.signature_params[i];
		const Type& given_type= given_args[i].type;

		bool deduced_specially= false;
		if( const auto type_param = signature_param.GetType() )
		{
			const Type& type= type_param->t;
			if( type == given_type || ReferenceIsConvertible( given_type, type, errors_container, src_loc ) ||
				( !expected_arg_is_mutalbe_reference && GetConversionConstructor( given_type, type, errors_container, src_loc ) != nullptr ) )
				deduced_specially= true;
		}
		else if( const auto template_param= signature_param.GetTemplateParam() )
		{
			if( const auto type= result.template_args_namespace->GetThisScopeValue( function_template.template_params[ template_param->index ].name )->GetTypeName() )
			{
				if( *type == given_type || ReferenceIsConvertible( given_type, *type, errors_container, src_loc ) ||
					( !expected_arg_is_mutalbe_reference && GetConversionConstructor( given_type, *type, errors_container, src_loc ) != nullptr ) )
					deduced_specially= true;
			}
		}

		if( !deduced_specially && !MatchTemplateArg( function_template, *result.template_args_namespace, given_type, src_loc, signature_param ) )
			return result;

	} // for template function arguments

	for( const auto& template_param : function_template.template_params )
	{
		const Value* const value= result.template_args_namespace->GetThisScopeValue( template_param.name );

		if( const auto type= value->GetTypeName() )
			result.template_args.push_back( *type );
		else if( const auto variable= value->GetVariable() )
			result.template_args.push_back( variable );
		else
		{
			// SPRACHE_TODO - maybe not generate this error?
			// Other function templates, for example, can match given aruments.
			REPORT_ERROR( TemplateParametersDeductionFailed, result.template_args_namespace->GetErrors(), src_loc );
			return result;
		}
	}

	result.function_template= function_template_ptr;
	return result;
}

const FunctionVariable* CodeBuilder::FinishTemplateFunctionParametrization(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const FunctionTemplatePtr& function_template_ptr )
{
	const FunctionTemplate& function_template= *function_template_ptr;

	if( function_template_ptr->template_params.size() != function_template_ptr->known_template_args.size() )
		return nullptr;

	TemplateFunctionPreparationResult result;

	result.template_args_namespace= std::make_shared<NamesScope>( NamesScope::c_template_args_namespace_name, function_template.parent_namespace );

	for( size_t i= 0u; i < function_template.template_params.size(); ++i )
	{
		result.template_args_namespace->AddName(
			function_template.template_params[i].name,
			std::visit( [&]( const auto& x ){ return Value( x, src_loc ); }, function_template.known_template_args[i] ) );
	}

	result.template_args= function_template.known_template_args;
	result.function_template= function_template_ptr;
	return FinishTemplateFunctionGeneration( errors_container, src_loc, result );
}

const FunctionVariable* CodeBuilder::FinishTemplateFunctionGeneration(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const TemplateFunctionPreparationResult& template_function_preparation_result )
{
	const FunctionTemplatePtr& function_template_ptr= template_function_preparation_result.function_template;
	const FunctionTemplate& function_template= *function_template_ptr;
	const Synt::Function& function_declaration= *function_template.syntax_element->function_;
	const std::string& func_name= function_declaration.name_.back();

	const NamesScopePtr& template_args_namespace= template_function_preparation_result.template_args_namespace;
	const TemplateArgs& template_args= template_function_preparation_result.template_args;

	// Encode name for caching. Name must be unique for each template and its parameters.
	const std::string name_encoded=
		std::to_string( reinterpret_cast<uintptr_t>( function_template.parent != nullptr ? function_template.parent.get() : &function_template ) ) + // Encode template address, because we needs unique keys for templates with same name.
		mangler_->MangleTemplateArgs( template_args );

	if( const auto it= generated_template_things_storage_.find( name_encoded ); it != generated_template_things_storage_.end() )
	{
		//Function for this template arguments already generated.
		const NamesScopePtr template_parameters_space= it->second.GetNamespace();
		U_ASSERT( template_parameters_space != nullptr );
		OverloadedFunctionsSet& result_functions_set= *template_parameters_space->GetThisScopeValue( func_name )->GetFunctionsSet();
		if( !result_functions_set.functions.empty() )
			return &result_functions_set.functions.front();
		else
			return nullptr; // May be in case of error or in case of "enable_if".
	}
	AddNewTemplateThing( std::move(name_encoded), Value( template_args_namespace, function_declaration.src_loc_ ) );

	CreateTemplateErrorsContext( errors_container, src_loc, template_args_namespace, function_template, func_name, template_args );

	// First, prepare only as prototype.
	NamesScopeFill( *template_args_namespace, function_template.syntax_element->function_, function_template.base_class );
	OverloadedFunctionsSet& result_functions_set= *template_args_namespace->GetThisScopeValue( func_name )->GetFunctionsSet();
	GlobalThingBuildFunctionsSet( *template_args_namespace, result_functions_set, false );

	if( result_functions_set.functions.empty() )
		return nullptr; // Function prepare failed

	FunctionVariable& function_variable= result_functions_set.functions.front();
	function_variable.base_template= function_template_ptr;

	// And generate function body after insertion of prototype.
	if( !function_variable.have_body ) // if function is constexpr, body may be already generated.
		BuildFuncCode(
			function_variable,
			function_template.base_class,
			*template_args_namespace,
			func_name,
			function_declaration.type_.params_,
			*function_declaration.block_,
			function_declaration.constructor_initialization_list_.get() );

	// Set correct mangled name
	if( function_variable.llvm_function != nullptr )
		function_variable.llvm_function->setName(
			mangler_->MangleFunction(
				*function_template.parent_namespace,
				func_name,
				*function_variable.type.GetFunctionType(),
				&template_args ) );

	// Two-step preparation needs for recursive function template call.

	return &function_variable;
}

Value* CodeBuilder::ParametrizeFunctionTemplate(
	const SrcLoc& src_loc,
	const OverloadedFunctionsSet& functions_set,
	const std::vector<Synt::Expression>& template_arguments,
	NamesScope& arguments_names_scope,
	FunctionContext& function_context )
{
	const std::vector<FunctionTemplatePtr>& function_templates= functions_set.template_functions;
	U_ASSERT( !function_templates.empty() );

	TemplateArgs template_args;

	{
		const bool prev_is_functionless_context= function_context.is_functionless_context;
		function_context.is_functionless_context= true;

		const StackVariablesStorage dummy_stack_variables_storage( function_context );

		for( const Synt::Expression& expr : template_arguments )
		{
			const Value value= BuildExpressionCode( expr, arguments_names_scope, function_context );
			if( const auto type_name= value.GetTypeName() )
				template_args.push_back( *type_name );
			else if( const auto variable= value.GetVariable() )
			{
				if( !TypeIsValidForTemplateVariableArgument( variable->type ) )
					REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, arguments_names_scope.GetErrors(), Synt::GetExpressionSrcLoc(expr), variable->type );
				else if( variable->constexpr_value == nullptr )
					REPORT_ERROR( ExpectedConstantExpression, arguments_names_scope.GetErrors(), Synt::GetExpressionSrcLoc(expr) );
				else
					template_args.push_back( variable );
			}
			else
				REPORT_ERROR( InvalidValueAsTemplateArgument, arguments_names_scope.GetErrors(), src_loc, value.GetKindName() );
		} // for given template arguments.

		DestroyUnusedTemporaryVariables( function_context, arguments_names_scope.GetErrors(), src_loc );
		function_context.is_functionless_context= prev_is_functionless_context;
	}

	if( template_args.size() != template_arguments.size() )
		return nullptr;

	// We needs unique name here, so use for it address of function templates set and template parameters.
	std::string name_encoded= "</.../>";
	for( const FunctionTemplatePtr& template_ : function_templates )
	{
		name_encoded+= std::to_string( reinterpret_cast<uintptr_t>( &template_ ) );
		name_encoded+= "_";
	}
	name_encoded+= mangler_->MangleTemplateArgs( template_args );

	if( const auto it= generated_template_things_storage_.find( name_encoded ); it != generated_template_things_storage_.end() )
		return &it->second; // Already generated.

	OverloadedFunctionsSet result;
	result.base_class= functions_set.base_class;
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
				src_loc,
				TemplateSignatureParam::TemplateParam{ i } );
		}

		if( !ok )
			continue;

		const auto new_template= std::make_shared<FunctionTemplate>(function_template);
		new_template->parent= function_template_ptr;

		// Reduce count of template arguments in new function template.
		new_template->known_template_args= template_args;
		new_template->known_template_args.resize( std::min( new_template->known_template_args.size(), function_template.template_params.size() ) );

		result.template_functions.push_back( new_template );
	} // for function templates

	if( result.template_functions.empty() )
	{
		REPORT_ERROR( TemplateFunctionGenerationFailed, arguments_names_scope.GetErrors(), src_loc, function_templates.front()->syntax_element->function_->name_.back() );
		return nullptr;
	}

	return AddNewTemplateThing( std::move(name_encoded), std::move(result) );
}

bool CodeBuilder::TypeIsValidForTemplateVariableArgument( const Type& type )
{
	if( const FundamentalType* const fundamental= type.GetFundamentalType() )
	{
		return
			IsInteger( fundamental->fundamental_type ) ||
			IsChar( fundamental->fundamental_type ) ||
			IsByte( fundamental->fundamental_type ) ||
			fundamental->fundamental_type == U_FundamentalType::bool_;
	}
	if( type.GetEnumType() != nullptr )
	{
		U_ASSERT( TypeIsValidForTemplateVariableArgument( type.GetEnumType()->underlaying_type ) );
		return true;
	}

	return false;
}

Value* CodeBuilder::AddNewTemplateThing( std::string key, Value thing )
{
	generated_template_things_sequence_.push_back( key );
	return & generated_template_things_storage_.insert( std::make_pair( std::move(key), std::move(thing) ) ).first->second;
}

} // namespace U
