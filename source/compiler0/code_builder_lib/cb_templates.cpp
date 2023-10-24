#include <algorithm>

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
	type_template->src_loc= type_template_declaration.src_loc;

	std::vector<TypeTemplate::TemplateParameter>& template_parameters= type_template->template_params;
	template_parameters.reserve( type_template_declaration.params.size() );
	llvm::SmallVector<bool, 32> template_parameters_usage_flags;

	ProcessTemplateParams(
		type_template_declaration.params,
		names_scope,
		template_parameters,
		template_parameters_usage_flags );

	if( type_template_declaration.is_short_form )
	{
		U_ASSERT( type_template_declaration.signature_params.empty() );
		// Assign template params to signature params.
		for( size_t i= 0u; i < type_template_declaration.params.size(); ++i )
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
		for( const Synt::TypeTemplate::SignatureParam& signature_param : type_template_declaration.signature_params )
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
					REPORT_ERROR( MandatoryTemplateSignatureArgumentAfterOptionalArgument, names_scope.GetErrors(), type_template_declaration.src_loc );

				++type_template->first_optional_signature_param;
			}
		}
	}
	U_ASSERT( type_template->first_optional_signature_param <= type_template->signature_params.size() );

	for( size_t i= 0u; i < type_template->template_params.size(); ++i )
		if( !template_parameters_usage_flags[i] )
			REPORT_ERROR( TemplateArgumentNotUsedInSignature, names_scope.GetErrors(), type_template->template_params[i].src_loc, type_template->template_params[i].name );

	// Check for redefinition and insert.
	for( const TypeTemplatePtr& prev_template : type_templates_set.type_templates )
	{
		if( type_template->signature_params == prev_template->signature_params )
		{
			REPORT_ERROR( TypeTemplateRedefinition, names_scope.GetErrors(), type_template_declaration.src_loc, type_template_declaration.name );
			return;
		}
	}
	type_templates_set.type_templates.push_back( type_template );
}

void CodeBuilder::PrepareFunctionTemplate(
	const Synt::FunctionTemplate& function_template_declaration,
	OverloadedFunctionsSet& functions_set,
	NamesScope& names_scope,
	const ClassPtr base_class )
{
	const auto& full_name= function_template_declaration.function->name;
	const std::string& function_template_name= full_name.front().name;

	if( full_name.size() > 1u )
		REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), function_template_declaration.src_loc );

	if( function_template_declaration.function->block == nullptr )
		REPORT_ERROR( IncompleteMemberOfClassTemplate, names_scope.GetErrors(), function_template_declaration.src_loc, function_template_name );
	if( function_template_declaration.function->virtual_function_kind != Synt::VirtualFunctionKind::None )
		REPORT_ERROR( VirtualForFunctionTemplate, names_scope.GetErrors(), function_template_declaration.src_loc, function_template_name );

	const auto function_template= std::make_shared<FunctionTemplate>();
	function_template->syntax_element= &function_template_declaration;
	function_template->src_loc= function_template_declaration.src_loc;
	function_template->parent_namespace= &names_scope;
	function_template->base_class= base_class;

	llvm::SmallVector<bool, 32> template_parameters_usage_flags; // Currently unused, because function template have no signature.

	ProcessTemplateParams(
		function_template_declaration.params,
		names_scope,
		function_template->template_params,
		template_parameters_usage_flags );

	for( const Synt::FunctionParam& function_param : function_template_declaration.function->type.params )
	{
		if( base_class != nullptr && function_param.name == Keyword( Keywords::this_ ) )
			function_template->signature_params.push_back( TemplateSignatureParam::TypeParam{ Type(base_class) } );
		else
		{
			function_template->signature_params.push_back(
				CreateTemplateSignatureParameter( names_scope, *global_function_context_, function_template->template_params, template_parameters_usage_flags, function_param.type ) );
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
	const llvm::ArrayRef<Synt::TemplateBase::Param> params,
	NamesScope& names_scope,
	std::vector<TypeTemplate::TemplateParameter>& template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags )
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
				REPORT_ERROR( Redefinition, names_scope.GetErrors(), param.src_loc, param.name );
				continue;
			}
		}

		template_parameters.emplace_back();
		template_parameters.back().name= param.name;
		template_parameters.back().src_loc= param.src_loc;
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
				REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, names_scope.GetErrors(), template_parameters[i].src_loc, type_param->t );
		}
		else if( template_parameters[i].type->IsTemplateParam() ) {}
		else
			REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), template_parameters[i].src_loc, *params[i].param_type );
	}
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
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

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::ArrayTypeName& array_type_name )
{
	TemplateSignatureParam::ArrayParam array_param;
	array_param.element_count= std::make_unique<TemplateSignatureParam>( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, array_type_name.size ) );
	array_param.element_type= std::make_unique<TemplateSignatureParam>( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, array_type_name.element_type ) );

	if( array_param.element_count->IsVariable() && array_param.element_type->IsType() )
		return TemplateSignatureParam::TypeParam{ PrepareTypeImpl( names_scope, function_context, array_type_name ) };

	return std::move(array_param);
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::FunctionType& function_pointer_type_name )
{
	TemplateSignatureParam::FunctionParam function_param;

	bool all_types_are_known= true;

	if( function_pointer_type_name.return_value_reference_modifier == ReferenceModifier::None )
		function_param.return_value_type= ValueType::Value;
	else
		function_param.return_value_type= function_pointer_type_name.return_value_mutability_modifier == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;

	function_param.is_unsafe= function_pointer_type_name.unsafe;
	function_param.calling_convention= GetLLVMCallingConvention(function_pointer_type_name.calling_convention, function_pointer_type_name.src_loc, names_scope.GetErrors() );

	// TODO - maybe check also reference tags?
	if( function_pointer_type_name.return_type != nullptr )
		function_param.return_type= std::make_unique<TemplateSignatureParam>( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, *function_pointer_type_name.return_type ) );
	else
		function_param.return_type= std::make_unique<TemplateSignatureParam>( TemplateSignatureParam::TypeParam{ void_type_ } );

	all_types_are_known&= function_param.return_type->IsType();

	for( const Synt::FunctionParam& in_param : function_pointer_type_name.params )
	{
		auto t= CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, in_param.type );
		all_types_are_known&= t.IsType();

		TemplateSignatureParam::FunctionParam::Param out_param;
		out_param.type= std::make_unique<TemplateSignatureParam>( std::move(t) );
		if( in_param.reference_modifier == Synt::ReferenceModifier::None )
			out_param.value_type= ValueType::Value;
		else if( in_param.mutability_modifier == Synt::MutabilityModifier::Mutable )
			out_param.value_type= ValueType::ReferenceMut;
		else
			out_param.value_type= ValueType::ReferenceImut;

		function_param.params.push_back( std::move(out_param) );
	}

	if( all_types_are_known )
		return TemplateSignatureParam::TypeParam{ PrepareTypeImpl( names_scope, function_context, function_pointer_type_name ) };

	return std::move(function_param);
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::TupleType& tuple_type_name )
{
	TemplateSignatureParam::TupleParam tuple_param;

	bool all_types_are_known= true;
	for( const auto& element_type : tuple_type_name.element_types )
	{
		tuple_param.element_types.push_back( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, element_type ) );
		all_types_are_known&= tuple_param.element_types.back().IsType();
	}

	if( all_types_are_known )
		return TemplateSignatureParam::TypeParam{ PrepareTypeImpl( names_scope, function_context, tuple_type_name ) };

	return tuple_param;
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
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
				raw_pointer_type_name.element_type ) );

	if( raw_pointer_param.element_type->IsType() )
		return TemplateSignatureParam::TypeParam{ PrepareTypeImpl( names_scope, function_context, raw_pointer_type_name ) };

	return raw_pointer_param;
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::GeneratorType& generator_type_name )
{
	TemplateSignatureParam::CoroutineParam coroutine_param;

	coroutine_param.return_type=
		std::make_unique<TemplateSignatureParam>(
			CreateTemplateSignatureParameter(
				names_scope,
				function_context,
				template_parameters,
				template_parameters_usage_flags,
				generator_type_name.return_type ) );

	if( coroutine_param.return_type->IsType() )
		return TemplateSignatureParam::TypeParam{ PrepareTypeImpl( names_scope, function_context, generator_type_name ) };

	coroutine_param.kind= CoroutineKind::Generator;

	if( generator_type_name.return_value_reference_modifier == ReferenceModifier::Reference )
		coroutine_param.return_value_type=
			generator_type_name.return_value_mutability_modifier == MutabilityModifier::Mutable
				? ValueType::ReferenceMut
				: ValueType::ReferenceImut;
	else
		coroutine_param.return_value_type= ValueType::Value;

	if( generator_type_name.inner_reference_tag != nullptr )
		coroutine_param.inner_references.push_back(
			generator_type_name.inner_reference_tag->mutability_modifier == MutabilityModifier::Mutable
				? InnerReferenceType::Mut
				: InnerReferenceType::Imut );

	coroutine_param.non_sync= ImmediateEvaluateNonSyncTag( names_scope, function_context, generator_type_name.non_sync_tag );

	return coroutine_param;
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::NameLookup& name_lookup )
{
	for( const TypeTemplate::TemplateParameter& template_parameter : template_parameters )
	{
		if( name_lookup.name == template_parameter.name )
		{
			const size_t param_index= size_t(&template_parameter - template_parameters.data());
			template_parameters_usage_flags[ param_index ]= true;

			return TemplateSignatureParam::TemplateParam{ param_index };
		}
	}

	return ValueToTemplateParam( ResolveValueImpl( names_scope, function_context, name_lookup ), names_scope, name_lookup.src_loc );
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::TemplateParametrization& template_parametrization )
{
	const Value base_value= ResolveValue( names_scope, *global_function_context_, template_parametrization.base );
	if( const auto type_templates_set= base_value.GetTypeTemplatesSet() )
	{
		TemplateSignatureParam::SpecializedTemplateParam specialized_template;

		bool all_args_are_known= true;
		for( const Synt::Expression& template_arg : template_parametrization.template_args )
		{
			specialized_template.params.push_back( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, template_arg ) );
			all_args_are_known&= specialized_template.params.back().IsType() || specialized_template.params.back().IsVariable();
		}

		if( all_args_are_known )
			return ValueToTemplateParam( ResolveValueImpl( names_scope, function_context, template_parametrization ), names_scope, template_parametrization.src_loc );

		specialized_template.type_templates= type_templates_set->type_templates;

		return specialized_template;
	}

	return ValueToTemplateParam( ResolveValueImpl( names_scope, function_context, template_parametrization ), names_scope, template_parametrization.src_loc );
}

TemplateSignatureParam CodeBuilder::ValueToTemplateParam( const Value& value, NamesScope& names_scope, const SrcLoc& src_loc )
{
	if( const auto type= value.GetTypeName() )
		return TemplateSignatureParam::TypeParam{ *type };

	if( const auto variable= value.GetVariable() )
	{
		if( !TypeIsValidForTemplateVariableArgument( variable->type ) )
		{
			REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, names_scope.GetErrors(), src_loc, variable->type );
			return TemplateSignatureParam::TypeParam();
		}
		if( variable->constexpr_value == nullptr )
		{
			REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), src_loc );
			return TemplateSignatureParam::TypeParam();
		}
		return TemplateSignatureParam::VariableParam{ variable->type, variable->constexpr_value };
	}

	if( value.GetTypeTemplatesSet() != nullptr )
		REPORT_ERROR( TemplateInstantiationRequired, names_scope.GetErrors(), src_loc, "" );

	REPORT_ERROR( InvalidValueAsTemplateArgument, names_scope.GetErrors(), src_loc, value.GetKindName() );
	return TemplateSignatureParam::TypeParam();
}

bool CodeBuilder::MatchTemplateArg(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam& template_param )
{
	return
		template_param.Visit(
			[&]( const auto& param )
			{
				return MatchTemplateArgImpl( template_, args_names_scope, template_arg, param );
			} );
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::TypeParam& template_param )
{
	(void)template_;
	(void)args_names_scope;

	if( const auto given_type= std::get_if<Type>( &template_arg ) )
		return *given_type == template_param.t;
	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::VariableParam& template_param )
{
	(void)template_;
	(void)args_names_scope;

	if( const auto given_variable= std::get_if<TemplateVariableArg>( &template_arg ) )
	{
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
	const TemplateSignatureParam::TemplateParam& template_param )
{
	const std::string& name= template_.template_params[ template_param.index ].name;

	NamesScopeValue* const value= args_names_scope.GetThisScopeValue( name );
	U_ASSERT( value != nullptr );
	if( value->value.GetYetNotDeducedTemplateArg() != nullptr )
	{
		const auto& param_type= template_.template_params[ template_param.index ].type;
		const bool is_variable_param= param_type != std::nullopt;

		if( const auto given_type= std::get_if<Type>( &template_arg ) )
		{
			if( is_variable_param )
				return false;

			value->value= *given_type;
			return true;
		}
		if( const auto given_variable= std::get_if<TemplateVariableArg>( &template_arg ) )
		{
			if( !is_variable_param )
				return false;

			if( !TypeIsValidForTemplateVariableArgument( given_variable->type ) || given_variable->constexpr_value == nullptr )
			{
				// May be in case of error.
				return false;
			}

			if( !MatchTemplateArg( template_, args_names_scope, given_variable->type, *param_type ) )
				return false;

			// Create global variable for given variable.
			// We can't just use given variable itself, because its address may be local for instantiation point.
			const VariablePtr variable_for_insertion=
				Variable::Create(
					given_variable->type,
					ValueType::ReferenceImut,
					Variable::Location::Pointer,
					name,
					CreateGlobalConstantVariable( given_variable->type, name, given_variable->constexpr_value ),
					given_variable->constexpr_value );

			value->value= variable_for_insertion;
			return true;
		}
	}
	else if( const auto prev_type= value->value.GetTypeName() )
	{
		if( const auto given_type= std::get_if<Type>( &template_arg ) )
			return *given_type == *prev_type;
	}
	else if( const auto prev_variable= value->value.GetVariable() )
	{
		if( const auto given_variable= std::get_if<TemplateVariableArg>( &template_arg ) )
		{
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
	const TemplateSignatureParam::ArrayParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_array_type= given_type->GetArrayType() )
		{
			if( !MatchTemplateArg( template_, args_names_scope, given_array_type->element_type, *template_param.element_type ) )
				return false;

			TemplateVariableArg size_variable;
			size_variable.type= size_type_;
			size_variable.constexpr_value=
				llvm::ConstantInt::get(
					size_type_.GetLLVMType(),
					llvm::APInt(
						uint32_t(size_type_.GetFundamentalType()->GetSize() * 8),
						given_array_type->element_count ) );

			return MatchTemplateArg( template_, args_names_scope, size_variable, *template_param.element_count );
		}
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
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
				if( !MatchTemplateArg( template_, args_names_scope, given_tuple_type->element_types[i], template_param.element_types[i] ) )
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
	const TemplateSignatureParam::RawPointerParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_raw_ponter_type= given_type->GetRawPointerType() )
		{
			return MatchTemplateArg( template_, args_names_scope, given_raw_ponter_type->element_type, *template_param.element_type );
		}
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
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
				MatchTemplateArg( template_, args_names_scope, given_function_type.return_type, *template_param.return_type ) &&
				given_function_type.params.size() == template_param.params.size()
				) )
				return false;

			for( size_t i= 0; i < template_param.params.size(); ++i )
			{
				if( !(
					given_function_type.params[i].value_type == template_param.params[i].value_type &&
					MatchTemplateArg( template_, args_names_scope, given_function_type.params[i].type, *template_param.params[i].type )
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
	const TemplateSignatureParam::CoroutineParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_class= given_type->GetClassType() )
		{
			if( const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &given_class->generated_class_data ) )
			{
				return
					coroutine_type_description->kind == template_param.kind &&
					coroutine_type_description->return_value_type == template_param.return_value_type &&
					coroutine_type_description->inner_references == template_param.inner_references &&
					coroutine_type_description->non_sync == template_param.non_sync &&
					MatchTemplateArg( template_, args_names_scope, coroutine_type_description->return_type, *template_param.return_type );
			}
		}
	}
	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const TemplateBase& template_,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::SpecializedTemplateParam& template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_class_type= given_type->GetClassType() )
		{
			if( const auto base_template= std::get_if< Class::BaseTemplate >( &given_class_type->generated_class_data ) )
			{
				if( !(
						std::find(
							template_param.type_templates.begin(),
							template_param.type_templates.end(),
							base_template->class_template ) != template_param.type_templates.end() &&
						template_param.params.size() == base_template->signature_args.size()
					) )
					return false;

				for( size_t i= 0; i < template_param.params.size(); ++i )
				{
					if( !MatchTemplateArg( template_, args_names_scope, base_template->signature_args[i], template_param.params[i] ) )
						return false;
				}

				return true;
			}
		}
	}

	return false;
}

std::optional<Type> CodeBuilder::GenTemplateType(
	const SrcLoc& src_loc,
	const TypeTemplatesSet& type_templates_set,
	const llvm::ArrayRef<Synt::Expression> template_arguments,
	NamesScope& arguments_names_scope,
	FunctionContext& function_context )
{
	llvm::SmallVector<TemplateArg, 8> arguments_calculated;
	EvaluateTemplateArgs( template_arguments, src_loc, arguments_names_scope, function_context, arguments_calculated );

	if( arguments_calculated.size() != template_arguments.size() )
	{
		REPORT_ERROR( TemplateParametersDeductionFailed, arguments_names_scope.GetErrors(), src_loc );
		return std::nullopt;
	}

	llvm::SmallVector<TemplateTypePreparationResult, 4> prepared_types;
	for( const TypeTemplatePtr& type_template : type_templates_set.type_templates )
	{
		TemplateTypePreparationResult generated_type= PrepareTemplateType( type_template, arguments_calculated );
		if( generated_type.type_template != nullptr )
			prepared_types.push_back( std::move(generated_type) );
	}

	if( prepared_types.empty() )
	{
		REPORT_ERROR( TemplateParametersDeductionFailed, arguments_names_scope.GetErrors(), src_loc );
		return std::nullopt;
	}

	if( const auto selected_template= SelectTemplateType( prepared_types, template_arguments.size() ) )
	{
		selected_template->type_template->used= true;
		return FinishTemplateTypeGeneration( src_loc, arguments_names_scope, *selected_template );
	}

	REPORT_ERROR( CouldNotSelectMoreSpicializedTypeTemplate, arguments_names_scope.GetErrors(), src_loc );
	return std::nullopt;
}

CodeBuilder::TemplateTypePreparationResult CodeBuilder::PrepareTemplateType(
	const TypeTemplatePtr& type_template_ptr,
	const llvm::ArrayRef<TemplateArg> template_arguments )
{
	// This method does not generate some errors, because instantiation may fail
	// for one class template, but success for other.

	TemplateTypePreparationResult result;

	const TypeTemplate& type_template= *type_template_ptr;

	if( template_arguments.size() < type_template.first_optional_signature_param ||
		template_arguments.size() > type_template.signature_params.size() )
		return result;

	result.template_args_namespace= std::make_shared<NamesScope>( NamesScope::c_template_args_namespace_name, type_template.parent_namespace );
	for( const TypeTemplate::TemplateParameter& param : type_template.template_params )
		result.template_args_namespace->AddName( param.name, NamesScopeValue( YetNotDeducedTemplateArg(), param.src_loc ) );

	result.signature_args.resize( type_template.signature_params.size() );

	for( size_t i= 0u; i < type_template.signature_params.size(); ++i )
	{
		TemplateArg& out_signature_arg= result.signature_args[i];
		if( i < template_arguments.size() )
			out_signature_arg= template_arguments[i];
		else
		{
			const auto& expr= type_template.syntax_element->signature_params[i].default_value;
			const Value value= BuildExpressionCode( expr, *result.template_args_namespace, *global_function_context_ );
			auto template_arg_opt= ValueToTemplateArg( value, result.template_args_namespace->GetErrors(), Synt::GetExpressionSrcLoc(expr) );
			if( template_arg_opt != std::nullopt )
				out_signature_arg= std::move( *template_arg_opt );
		}

		if( !MatchTemplateArg( type_template, *result.template_args_namespace, out_signature_arg, type_template.signature_params[i] ) )
			return result;
	} // for signature arguments

	result.type_template= type_template_ptr;

	return result;
}

std::optional<Type> CodeBuilder::FinishTemplateTypeGeneration(
	const SrcLoc& src_loc,
	NamesScope& arguments_names_scope,
	const TemplateTypePreparationResult& template_type_preparation_result )
{
	const TypeTemplatePtr& type_template_ptr= template_type_preparation_result.type_template;
	const TypeTemplate& type_template= *type_template_ptr;
	const NamesScopePtr& template_args_namespace= template_type_preparation_result.template_args_namespace;

	{
		TemplateKey template_key{ type_template_ptr, template_type_preparation_result.signature_args };

		// Check, if type already generated.
		if( const auto it= generated_template_things_storage_.find( template_key ); it != generated_template_things_storage_.end() )
		{
			const NamesScopePtr template_parameters_space= it->second;
			U_ASSERT( template_parameters_space != nullptr );
			if( const auto value= template_parameters_space->GetThisScopeValue( Class::c_template_class_name ) )
			{
				if( const auto type= value->value.GetTypeName() )
					return *type;
			}
			else
				return std::nullopt;
		}
		AddNewTemplateThing( std::move(template_key), template_args_namespace );
	}

	CreateTemplateErrorsContext(
		arguments_names_scope.GetErrors(),
		src_loc,
		template_args_namespace,
		type_template,
		type_template.syntax_element->name );

	if( const auto class_ptr= std::get_if< std::unique_ptr<const Synt::Class> >( &type_template.syntax_element->something ) )
	{
		U_ASSERT( (*class_ptr)->name == Class::c_template_class_name );

		const ClassPtr class_type= NamesScopeFill( *template_args_namespace, **class_ptr, Class::BaseTemplate{ type_template_ptr, template_type_preparation_result.signature_args } );
		if( class_type == nullptr )
			return std::nullopt;

		return Type(class_type);
	}
	if( const auto type_alias= std::get_if< std::unique_ptr<const Synt::TypeAlias> >( &type_template.syntax_element->something ) )
	{
		const Type type= PrepareType( (*type_alias)->value, *template_args_namespace, *global_function_context_ );
		template_args_namespace->AddName( Class::c_template_class_name, NamesScopeValue( type, src_loc /* TODO - check src_loc */ ) );
		return type;
	}
	else U_ASSERT(false);

	return std::nullopt;
}

CodeBuilder::TemplateFunctionPreparationResult CodeBuilder::PrepareTemplateFunction(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const FunctionTemplatePtr& function_template_ptr,
	const llvm::ArrayRef<FunctionType::Param> actual_args,
	const bool first_actual_arg_is_this )
{
	const FunctionTemplate& function_template= *function_template_ptr;
	const Synt::Function& function_declaration= *function_template.syntax_element->function;

	const FunctionType::Param* given_args= actual_args.data();
	size_t given_arg_count= actual_args.size();

	if( first_actual_arg_is_this &&
		!function_declaration.type.params.empty() && function_declaration.type.params.front().name != Keywords::this_ )
	{
		++given_args;
		--given_arg_count;
	}

	TemplateFunctionPreparationResult result;

	if( given_arg_count != function_declaration.type.params.size() )
		return result;

	result.template_args_namespace= std::make_shared<NamesScope>( NamesScope::c_template_args_namespace_name, function_template.parent_namespace );
	FillKnownFunctionTemplateArgsIntoNamespace( function_template, *result.template_args_namespace );

	for( size_t i= 0u; i < function_declaration.type.params.size(); ++i )
	{
		const Synt::FunctionParam& function_param= function_declaration.type.params[i];

		const bool expected_arg_is_mutalbe_reference=
			function_param.reference_modifier == Synt::ReferenceModifier::Reference &&
			function_param.mutability_modifier == Synt::MutabilityModifier::Mutable;

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
				( !expected_arg_is_mutalbe_reference && HasConversionConstructor( given_type, type, errors_container, src_loc ) ) )
				deduced_specially= true;
		}
		else if( const auto template_param= signature_param.GetTemplateParam() )
		{
			if( const auto type= result.template_args_namespace->GetThisScopeValue( function_template.template_params[ template_param->index ].name )->value.GetTypeName() )
			{
				if( *type == given_type || ReferenceIsConvertible( given_type, *type, errors_container, src_loc ) ||
					( !expected_arg_is_mutalbe_reference && HasConversionConstructor( given_type, *type, errors_container, src_loc ) ) )
					deduced_specially= true;
			}
		}

		if( !deduced_specially && !MatchTemplateArg( function_template, *result.template_args_namespace, given_type, signature_param ) )
			return result;

	} // for template function arguments

	// Process "enable_if" here - fail template function preparation if condition is false.
	if( std::get_if<Synt::EmptyVariant>( &function_declaration.condition ) == nullptr &&
		!EvaluateBoolConstantExpression( *result.template_args_namespace, *global_function_context_, function_declaration.condition ) )
		return result;

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
	result.function_template= function_template_ptr;
	result.template_args_namespace= std::make_shared<NamesScope>( NamesScope::c_template_args_namespace_name, function_template.parent_namespace );
	FillKnownFunctionTemplateArgsIntoNamespace( function_template, *result.template_args_namespace );

	return FinishTemplateFunctionGeneration( errors_container, src_loc, result );
}

const FunctionVariable* CodeBuilder::FinishTemplateFunctionGeneration(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const TemplateFunctionPreparationResult& template_function_preparation_result )
{
	const FunctionTemplatePtr& function_template_ptr= template_function_preparation_result.function_template;
	const FunctionTemplate& function_template= *function_template_ptr;
	const Synt::Function& function_declaration= *function_template.syntax_element->function;
	const std::string& func_name= function_declaration.name.back().name;

	const NamesScopePtr& template_args_namespace= template_function_preparation_result.template_args_namespace;

	TemplateArgs template_args;
	template_args.reserve( function_template.template_params.size() );
	for( const auto& template_param : function_template.template_params )
	{
		const NamesScopeValue* const value= template_args_namespace->GetThisScopeValue( template_param.name );

		if( const auto type= value->value.GetTypeName() )
			template_args.push_back( *type );
		else if( const auto variable= value->value.GetVariable() )
			template_args.push_back( TemplateVariableArg( *variable ) );
		else { /* generate error later */ }
	}

	if( template_args.size() != function_template.template_params.size() )
	{
		REPORT_ERROR( TemplateParametersDeductionFailed, template_args_namespace->GetErrors(), src_loc );
		return nullptr;
	}

	TemplateKey template_key
	{
		function_template_ptr->parent != nullptr ? function_template_ptr->parent : function_template_ptr,
		template_args,
	};

	if( const auto it= generated_template_things_storage_.find( template_key ); it != generated_template_things_storage_.end() )
	{
		//Function for this template arguments already generated.
		const NamesScopePtr template_parameters_space= it->second;
		U_ASSERT( template_parameters_space != nullptr );
		OverloadedFunctionsSet& result_functions_set= *template_parameters_space->GetThisScopeValue( func_name )->value.GetFunctionsSet();
		if( !result_functions_set.functions.empty() )
			return &result_functions_set.functions.front();
		else
			return nullptr; // May be in case of error or in case of "enable_if".
	}
	AddNewTemplateThing( std::move(template_key), template_args_namespace );

	CreateTemplateErrorsContext( errors_container, src_loc, template_args_namespace, function_template, func_name );

	// First, prepare only as prototype.
	NamesScopeFill( *template_args_namespace, *function_template.syntax_element->function, function_template.base_class );
	OverloadedFunctionsSet& result_functions_set= *template_args_namespace->GetThisScopeValue( func_name )->value.GetFunctionsSet();
	GlobalThingBuildFunctionsSet( *template_args_namespace, result_functions_set, false );

	if( result_functions_set.functions.empty() )
		return nullptr; // Function prepare failed

	FunctionVariable& function_variable= result_functions_set.functions.front();
	if( function_variable.constexpr_kind != FunctionVariable::ConstexprKind::ConstexprComplete )
	{
		bool can_be_constexpr= true;
		if( skip_building_generated_functions_ )
		{
			// If we skipping building of generate function, perform pre-check of constexpr possibility.
			// If param types/return type is not constexpr, function can't be constexpr and we can skip its building.

			// It is fine to require type completeness here, because we plan to build this function anyway and that too requires type completeness.

			if( !EnsureTypeComplete( function_variable.type.return_type ) || !function_variable.type.return_type.CanBeConstexpr() )
				can_be_constexpr= false;

			for( const FunctionType::Param& param : function_variable.type.params )
			{
				if( !EnsureTypeComplete( param.type ) || !param.type.CanBeConstexpr() )
					can_be_constexpr= false;
			}

			if( function_variable.type.unsafe )
				can_be_constexpr= false;
		}
		function_variable.constexpr_kind= can_be_constexpr ? FunctionVariable::ConstexprKind::ConstexprAuto : FunctionVariable::ConstexprKind::NonConstexpr;
	}

	// Set correct mangled name
	function_variable.llvm_function->name_mangled=
		mangler_->MangleFunction(
			*function_template.parent_namespace,
			func_name,
			function_variable.type,
			template_args );
	if( function_variable.llvm_function->function != nullptr )
		function_variable.llvm_function->function->setName( function_variable.llvm_function->name_mangled );

	// Generate function body after insertion of prototype.
	// if function is constexpr, body may be already generated.
	// Skip building body if generated functions building is disabled and if this function can't be constexpr.
	if( !function_variable.have_body &&
		!( skip_building_generated_functions_ && function_variable.constexpr_kind == FunctionVariable::ConstexprKind::NonConstexpr ) )
		BuildFuncCode(
			function_variable,
			function_template.base_class,
			*template_args_namespace,
			func_name,
			function_declaration.type.params,
			*function_declaration.block,
			function_declaration.constructor_initialization_list.get() );

	// Two-step preparation needs for recursive function template call.

	return &function_variable;
}

OverloadedFunctionsSetPtr CodeBuilder::ParametrizeFunctionTemplate(
	const SrcLoc& src_loc,
	const OverloadedFunctionsSetConstPtr& functions_set_ptr,
	const llvm::ArrayRef<Synt::Expression> template_arguments,
	NamesScope& arguments_names_scope,
	FunctionContext& function_context )
{
	const std::vector<FunctionTemplatePtr>& function_templates= functions_set_ptr->template_functions;
	U_ASSERT( !function_templates.empty() );

	TemplateArgs arguments_calculated;
	EvaluateTemplateArgs( template_arguments, src_loc, arguments_names_scope, function_context, arguments_calculated );

	if( arguments_calculated.size() != template_arguments.size() )
		return nullptr;

	ParametrizedFunctionTemplateKey template_key{ functions_set_ptr, arguments_calculated };

	if( const auto it= parametrized_template_functions_cache_.find( template_key ); it != parametrized_template_functions_cache_.end() )
		return it->second; // Already generated.

	auto result= std::make_shared<OverloadedFunctionsSet>();
	result->base_class= functions_set_ptr->base_class;
	for( const FunctionTemplatePtr& function_template_ptr : function_templates )
	{
		const FunctionTemplate& function_template= *function_template_ptr;
		if( function_template.template_params.size() < arguments_calculated.size() )
		{
			// Ignore functions with number of template parameters less than number of specified arguments.
			continue;
		}

		NamesScope args_names_scope("", function_template.parent_namespace );
		for( const TemplateBase::TemplateParameter& param : function_template.template_params )
			args_names_scope.AddName( param.name, NamesScopeValue( YetNotDeducedTemplateArg(), param.src_loc ) );

		bool ok= true;
		for( size_t i= 0u; i < arguments_calculated.size(); ++i )
		{
			ok&= MatchTemplateArg(
				function_template,
				args_names_scope,
				arguments_calculated[i],
				TemplateSignatureParam::TemplateParam{ i } );
		}

		if( !ok )
			continue;

		const auto new_template= std::make_shared<FunctionTemplate>(function_template);
		new_template->parent= function_template_ptr;
		new_template->known_template_args.insert( new_template->known_template_args.end(), arguments_calculated.begin(), arguments_calculated.end() );

		result->template_functions.push_back( new_template );
	} // for function templates

	if( result->template_functions.empty() )
	{
		REPORT_ERROR( TemplateFunctionGenerationFailed, arguments_names_scope.GetErrors(), src_loc, function_templates.front()->syntax_element->function->name.back().name );
		return nullptr;
	}

	return
		parametrized_template_functions_cache_.insert(
			std::make_pair( std::move(template_key), std::move(result) ) ).first->second;
}

void CodeBuilder::EvaluateTemplateArgs(
	const llvm::ArrayRef<Synt::Expression> template_arguments,
	const SrcLoc& src_loc,
	NamesScope& arguments_names_scope,
	FunctionContext& function_context,
	llvm::SmallVectorImpl<TemplateArg>& out_args )
{
	out_args.reserve( template_arguments.size() );

	const bool prev_is_functionless_context= function_context.is_functionless_context;
	function_context.is_functionless_context= true;

	const StackVariablesStorage dummy_stack_variables_storage( function_context );

	for( const Synt::Expression& expr : template_arguments )
	{
		const Value value= BuildExpressionCode( expr, arguments_names_scope, function_context );
		auto template_arg_opt= ValueToTemplateArg( value, arguments_names_scope.GetErrors(), Synt::GetExpressionSrcLoc(expr) );
		if( template_arg_opt != std::nullopt )
			out_args.push_back( std::move( *template_arg_opt ) );
	}

	DestroyUnusedTemporaryVariables( function_context, arguments_names_scope.GetErrors(), src_loc );
	function_context.is_functionless_context= prev_is_functionless_context;
}

std::optional<TemplateArg> CodeBuilder::ValueToTemplateArg( const Value& value, CodeBuilderErrorsContainer& errors, const SrcLoc& src_loc )
{
	if( const Type* const type_name= value.GetTypeName() )
		return TemplateArg( *type_name );

	if( const auto variable= value.GetVariable() )
	{
		if( !TypeIsValidForTemplateVariableArgument( variable->type ) )
		{
			REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, errors, src_loc, variable->type );
			return std::nullopt;
		}
		if( variable->constexpr_value == nullptr )
		{
			REPORT_ERROR( ExpectedConstantExpression, errors, src_loc );
			return std::nullopt;
		}
		return TemplateArg( TemplateVariableArg( *variable ) );
	}

	REPORT_ERROR( InvalidValueAsTemplateArgument, errors, src_loc, value.GetKindName() );
	return std::nullopt;
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
	if( const auto enum_type= type.GetEnumType() )
	{
		U_ASSERT( TypeIsValidForTemplateVariableArgument( enum_type->underlaying_type ) );
		return true;
	}

	return false;
}

void CodeBuilder::FillKnownFunctionTemplateArgsIntoNamespace(
	const FunctionTemplate& function_template,
	NamesScope& target_namespace )
{
	for( size_t i= 0u; i < function_template.template_params.size(); ++i )
	{
		const std::string& name= function_template.template_params[i].name;

		Value v;
		if( i < function_template.known_template_args.size() )
		{
			const TemplateArg& known_template_arg= function_template.known_template_args[i];

			if( const auto type= std::get_if<Type>( &known_template_arg ) )
				v= *type;
			else if( const auto variable= std::get_if<TemplateVariableArg>( &known_template_arg ) )
			{
				const VariablePtr variable_for_insertion=
					Variable::Create(
						variable->type,
						ValueType::ReferenceImut,
						Variable::Location::Pointer,
						name,
						CreateGlobalConstantVariable( variable->type, name, variable->constexpr_value ),
						variable->constexpr_value );
				v= variable_for_insertion;
			}
			else U_ASSERT(false);
		}
		else
			v= YetNotDeducedTemplateArg();

		target_namespace.AddName( name, NamesScopeValue( std::move(v), function_template.template_params[i].src_loc ) );
	}
}

void CodeBuilder::AddNewTemplateThing( TemplateKey key, NamesScopePtr thing )
{
	generated_template_things_sequence_.push_back( key );
	generated_template_things_storage_.insert( std::make_pair( std::move(key), std::move(thing) ) );
}

void CodeBuilder::CreateTemplateErrorsContext(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const NamesScopePtr& template_args_namespace,
	const TemplateBase& template_,
	const std::string_view template_name )
{
	REPORT_ERROR( TemplateContext, errors_container, src_loc );
	const auto template_error_context= std::make_shared<TemplateErrorsContext>();
	template_error_context->context_declaration_src_loc= template_.src_loc;
	errors_container.back().template_context= template_error_context;

	// Use shared_ptr aliasing in order to create shared_ptr for errors container from shared_ptr for template errors context.
	std::shared_ptr<CodeBuilderErrorsContainer> template_errors_container( template_error_context, &template_error_context->errors );
	template_args_namespace->SetErrors( template_errors_container );

	{
		std::string args_description;
		args_description+= "[ with ";

		for( const auto& param : template_.template_params )
		{
			if( const auto value= template_args_namespace->GetThisScopeValue(param.name) )
			{
				args_description+= param.name + " = ";
				if( const Type* const type= value->value.GetTypeName() )
					args_description+= type->ToString();
				else if( const auto variable= value->value.GetVariable() )
					args_description+= ConstantVariableToString( TemplateVariableArg( *variable ) );
				else {}

				if( &param != &template_.template_params.back() )
					args_description+= ", ";
			}
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

} // namespace U
