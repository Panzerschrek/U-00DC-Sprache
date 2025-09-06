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
				else if( const auto type_templates_set= value->value.GetTypeTemplatesSet() )
				{
					if( type_templates_set->type_templates.size() == 1 )
						args_description+= type_templates_set->type_templates.front()->ToString();
				}

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

void CheckSignatureParamIsValidForTemplateValueArgumentType(
	const TemplateSignatureParam& param,
	NamesScope& names_scope,
	const std::string_view param_name,
	const SrcLoc& src_loc )
{
	if( const auto type_param= param.GetType() )
	{
		if( !type_param->t.IsValidForTemplateVariableArgument() )
			REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, names_scope.GetErrors(), src_loc, type_param->t );
	}
	else if( param.IsTemplateParam() ) {}
	else if( const auto array_param= param.GetArray() )
		CheckSignatureParamIsValidForTemplateValueArgumentType( *array_param->element_type, names_scope, param_name, src_loc );
	else if( const auto tuple_param= param.GetTuple() )
	{
		for( const auto& element_param : tuple_param->element_types )
			CheckSignatureParamIsValidForTemplateValueArgumentType( element_param, names_scope, param_name, src_loc );
	}
	else
		REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), src_loc, param_name );
}

} // namespace

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

	std::vector<TemplateParameter>& template_parameters= type_template->template_params;
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
			type_template->signature_params.push_back( TemplateSignatureParam::TemplateParam{ i, template_parameters[i].kind_data.index() } );
			template_parameters_usage_flags[i]= true;
		}

		type_template->first_optional_signature_param= type_template->signature_params.size();
	}
	else
	{
		// Check and fill signature args.
		type_template->first_optional_signature_param= 0u;
		if( !type_template_declaration.signature_params.empty() )
		{
			WithGlobalFunctionContext(
				[&]( FunctionContext& function_context )
				{
					for( const Synt::TypeTemplate::SignatureParam& signature_param : type_template_declaration.signature_params )
					{
						type_template->signature_params.push_back(
							CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, signature_param.name ) );

						if( !std::holds_alternative<Synt::EmptyVariant>( signature_param.default_value ) )
							CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, signature_param.default_value );
						else
						{
							const size_t index= type_template->signature_params.size() - 1u;
							if (index > type_template->first_optional_signature_param )
								REPORT_ERROR( MandatoryTemplateSignatureArgumentAfterOptionalArgument, names_scope.GetErrors(), type_template_declaration.src_loc );

							++type_template->first_optional_signature_param;
						}
					}
				} );
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
		if( type_template->src_loc.GetFileIndex() != prev_template->src_loc.GetFileIndex() )
		{
			// Require defining a set of overloaded type templates in a single file.
			// This is needed in order to prevent possible mangling problems of template types with arguments - overloaded type templates.
			REPORT_ERROR( OverloadingImportedTypeTemplate, names_scope.GetErrors(),  type_template->src_loc );
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

	llvm::SmallVector<bool, 32> template_parameters_usage_flags; // Currently unused, because function template has no signature.

	ProcessTemplateParams(
		function_template_declaration.params,
		names_scope,
		function_template->template_params,
		template_parameters_usage_flags );

	if( !function_template_declaration.function->type.params.empty() )
	{
		WithGlobalFunctionContext(
			[&]( FunctionContext& function_context )
			{
				for( const Synt::FunctionParam& function_param : function_template_declaration.function->type.params )
				{
					if( base_class != nullptr && function_param.name == Keyword( Keywords::this_ ) )
						function_template->signature_params.push_back( TemplateSignatureParam::Type{ Type(base_class) } );
					else
						function_template->signature_params.push_back(
							CreateTemplateSignatureParameter( names_scope, function_context, function_template->template_params, template_parameters_usage_flags, function_param.type ) );
				}
			} );
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
	const llvm::ArrayRef<Synt::TemplateParam> params,
	NamesScope& names_scope,
	std::vector<TemplateParameter>& template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags )
{
	U_ASSERT( template_parameters.empty() );
	U_ASSERT( template_parameters_usage_flags.empty() );

	// Check and fill template parameters.
	for( const Synt::TemplateParam& param : params )
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

		TemplateParameter out_param;
		out_param.name= param.name;
		out_param.src_loc= param.src_loc;

		if( std::holds_alternative< Synt::TemplateParam::TypeParamData >( param.kind_data ) )
			out_param.kind_data= TemplateParameter::TypeParamData{};
		else if( std::holds_alternative< Synt::TemplateParam::TypeTemplateParamData >( param.kind_data ) )
			out_param.kind_data= TemplateParameter::TypeTemplateParamData{};
		else if( std::holds_alternative< Synt::TemplateParam::VariableParamData >( param.kind_data ) )
			out_param.kind_data= TemplateParameter::VariableParamData(); // Set type of variable later.
		else U_ASSERT(false);

		template_parameters.push_back( std::move(out_param) );

		template_parameters_usage_flags.push_back(false);
	}

	U_ASSERT( template_parameters_usage_flags.size() == template_parameters.size() );

	for( size_t i= 0u; i < template_parameters.size(); ++i )
	{
		if( const auto variable_param_data = std::get_if<Synt::TemplateParam::VariableParamData>( &params[i].kind_data ) )
		{
			auto variable_param_type=
				WithGlobalFunctionContext(
					[&]( FunctionContext& function_context )
					{
						return
							CreateTemplateSignatureParameter(
								names_scope,
								function_context,
								template_parameters,
								template_parameters_usage_flags,
								variable_param_data->type );
					});

			CheckSignatureParamIsValidForTemplateValueArgumentType(
				variable_param_type,
				names_scope,
				params[i].name,
				template_parameters[i].src_loc );

			template_parameters[i].kind_data = TemplateParameter::VariableParamData{ std::move(variable_param_type) };
		}
	}
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::EmptyVariant& empty_variant )
{
	U_ASSERT(false);
	U_UNUSED(empty_variant);
	U_UNUSED(names_scope);
	U_UNUSED(function_context);
	U_UNUSED(template_parameters);
	U_UNUSED(template_parameters_usage_flags);
	return TemplateSignatureParam::Type{ invalid_type_ };
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::ArrayTypeName& array_type_name )
{
	TemplateSignatureParam::Array array_param;
	array_param.element_count= std::make_unique<TemplateSignatureParam>( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, array_type_name.size ) );
	array_param.element_type= std::make_unique<TemplateSignatureParam>( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, array_type_name.element_type ) );

	if( array_param.element_count->IsVariable() && array_param.element_type->IsType() )
		return TemplateSignatureParam::Type{ PrepareTypeImpl( names_scope, function_context, array_type_name ) };

	return std::move(array_param);
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::FunctionType& function_pointer_type_name )
{
	TemplateSignatureParam::Function function_param;

	bool all_types_are_known= true;

	if( function_pointer_type_name.return_value_reference_modifier == ReferenceModifier::None )
		function_param.return_value_type= ValueType::Value;
	else
		function_param.return_value_type= function_pointer_type_name.return_value_mutability_modifier == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;

	function_param.is_unsafe= function_pointer_type_name.unsafe;
	function_param.calling_convention= PrepareCallingConvention( names_scope, function_context, function_pointer_type_name.calling_convention );

	// TODO - maybe check also reference tags?
	if( function_pointer_type_name.return_type != nullptr )
		function_param.return_type= std::make_unique<TemplateSignatureParam>( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, *function_pointer_type_name.return_type ) );
	else
		function_param.return_type= std::make_unique<TemplateSignatureParam>( TemplateSignatureParam::Type{ void_type_ } );

	all_types_are_known&= function_param.return_type->IsType();

	for( const Synt::FunctionParam& in_param : function_pointer_type_name.params )
	{
		auto t= CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, in_param.type );
		all_types_are_known&= t.IsType();

		TemplateSignatureParam::Function::Param out_param;
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
		return TemplateSignatureParam::Type{ PrepareTypeImpl( names_scope, function_context, function_pointer_type_name ) };

	return std::move(function_param);
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::TupleType& tuple_type_name )
{
	TemplateSignatureParam::Tuple tuple_param;

	bool all_types_are_known= true;
	for( const auto& element_type : tuple_type_name.element_types )
	{
		tuple_param.element_types.push_back( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, element_type ) );
		all_types_are_known&= tuple_param.element_types.back().IsType();
	}

	if( all_types_are_known )
		return TemplateSignatureParam::Type{ PrepareTypeImpl( names_scope, function_context, tuple_type_name ) };

	return tuple_param;
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::RawPointerType& raw_pointer_type_name )
{
	TemplateSignatureParam::RawPointer raw_pointer_param;
	raw_pointer_param.element_type=
		std::make_unique<TemplateSignatureParam>(
			CreateTemplateSignatureParameter(
				names_scope,
				function_context,
				template_parameters,
				template_parameters_usage_flags,
				raw_pointer_type_name.element_type ) );

	if( raw_pointer_param.element_type->IsType() )
		return TemplateSignatureParam::Type{ PrepareTypeImpl( names_scope, function_context, raw_pointer_type_name ) };

	return raw_pointer_param;
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const llvm::ArrayRef<TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::CoroutineType& coroutine_type_name )
{
	TemplateSignatureParam::Coroutine coroutine_param;

	coroutine_param.return_type=
		std::make_unique<TemplateSignatureParam>(
			CreateTemplateSignatureParameter(
				names_scope,
				function_context,
				template_parameters,
				template_parameters_usage_flags,
				coroutine_type_name.return_type ) );

	if( coroutine_param.return_type->IsType() )
		return TemplateSignatureParam::Type{ PrepareTypeImpl( names_scope, function_context, coroutine_type_name ) };

	coroutine_param.kind= coroutine_type_name.kind;

	if( coroutine_type_name.return_value_reference_modifier == ReferenceModifier::Reference )
		coroutine_param.return_value_type=
			coroutine_type_name.return_value_mutability_modifier == MutabilityModifier::Mutable
				? ValueType::ReferenceMut
				: ValueType::ReferenceImut;
	else
		coroutine_param.return_value_type= ValueType::Value;

	coroutine_param.inner_references.reserve( coroutine_type_name.inner_references.size() );
	for( const Synt::MutabilityModifier m : coroutine_type_name.inner_references )
		coroutine_param.inner_references.push_back( m == MutabilityModifier::Mutable ? InnerReferenceKind::Mut : InnerReferenceKind::Imut );

	coroutine_param.non_sync= ImmediateEvaluateNonSyncTag( names_scope, function_context, coroutine_type_name.non_sync_tag );

	const size_t num_params= 1;
	if( coroutine_type_name.return_value_reference_expression != nullptr )
		coroutine_param.return_references= EvaluateFunctionReturnReferences( names_scope, function_context, *coroutine_type_name.return_value_reference_expression, num_params );
	if( coroutine_type_name.return_value_inner_references_expression != nullptr )
		coroutine_param.return_inner_references= EvaluateFunctionReturnInnerReferences( names_scope, function_context, *coroutine_type_name.return_value_inner_references_expression, num_params );

	return coroutine_param;
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	llvm::ArrayRef<TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::NameLookup& name_lookup )
{
	for( const TemplateParameter& template_parameter : template_parameters )
	{
		if( name_lookup.name == template_parameter.name )
		{
			const size_t param_index= size_t(&template_parameter - template_parameters.data());
			template_parameters_usage_flags[ param_index ]= true;

			return TemplateSignatureParam::TemplateParam{ param_index, template_parameter.kind_data.index() };
		}
	}

	return ValueToTemplateParam( ResolveValueImpl( names_scope, function_context, name_lookup ), names_scope, name_lookup.src_loc );
}

TemplateSignatureParam CodeBuilder::CreateTemplateSignatureParameterImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	llvm::ArrayRef<TemplateParameter> template_parameters,
	llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
	const Synt::TemplateParameterization& template_parameterization )
{
	if( const auto name_lookup = std::get_if<Synt::NameLookup>( &template_parameterization.base ) )
	{
		for( const TemplateParameter& template_parameter : template_parameters )
		{
			if( name_lookup->name == template_parameter.name &&
				std::holds_alternative< TemplateParameter::TypeTemplateParamData >( template_parameter.kind_data ) )
			{
				const size_t param_index= size_t(&template_parameter - template_parameters.data());
				template_parameters_usage_flags[ param_index ]= true;

				std::vector<TemplateSignatureParam> specialized_template_params;
				specialized_template_params.reserve( template_parameterization.template_args.size() );
				for( const Synt::Expression& template_arg : template_parameterization.template_args )
					specialized_template_params.push_back( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, template_arg ) );

				return TemplateSignatureParam::SpecializedTemplate
				{
					{ TemplateSignatureParam( TemplateSignatureParam::TemplateParam{ param_index, template_parameter.kind_data.index() } ) },
					std::move(specialized_template_params),
				};
			}
		}
	}

	const Value base_value= ResolveValue( names_scope, function_context, template_parameterization.base );

	if( const auto type_templates_set= base_value.GetTypeTemplatesSet() )
	{
		std::vector<TemplateSignatureParam> specialized_template_params;
		specialized_template_params.reserve( template_parameterization.template_args.size() );
		bool all_params_are_known= true;
		for( const Synt::Expression& template_arg : template_parameterization.template_args )
		{
			specialized_template_params.push_back( CreateTemplateSignatureParameter( names_scope, function_context, template_parameters, template_parameters_usage_flags, template_arg ) );
			all_params_are_known&= specialized_template_params.back().IsType() || specialized_template_params.back().IsVariable();
		}

		if( all_params_are_known )
			return ValueToTemplateParam( ResolveValueImpl( names_scope, function_context, template_parameterization ), names_scope, template_parameterization.src_loc );

		if( type_templates_set->type_templates.size() == 1 )
		{
			const TypeTemplatePtr single_type_template= type_templates_set->type_templates.front();
			if( single_type_template->syntax_element != nullptr )
			{
				// Process a case with single trivial type alias.
				// Expand it, if it's possible.
				if( const auto type_alias_ptr= std::get_if< std::unique_ptr<const Synt::TypeAlias> >( &single_type_template->syntax_element->something ) )
				{
					if( single_type_template->signature_params.size() == specialized_template_params.size() )
					{
						// Try to match this type alias params against given signature params.
						llvm::SmallVector<TemplateSignatureParam, 4> alias_template_params_to_signature_params_mapping;
						alias_template_params_to_signature_params_mapping.resize( single_type_template->template_params.size(), TemplateSignatureParam() );

						llvm::SmallVector<bool, 32> param_known_flags;
						param_known_flags.resize( single_type_template->template_params.size(), false );
						bool params_matching_ok= true;

						for( size_t i= 0; i < single_type_template->signature_params.size(); ++i )
						{
							const TemplateSignatureParam& dst_param= single_type_template->signature_params[i];
							const TemplateSignatureParam& src_param= specialized_template_params[i];
							if( const auto dst_template_param= dst_param.GetTemplateParam() )
							{
								// Template param in signature param. Build mapping for it and check if possible repetitions of this param produce the same result.
								bool& known= param_known_flags[ dst_template_param->index ];
								if( !known )
								{
									alias_template_params_to_signature_params_mapping[ dst_template_param->index ]= src_param;
									known= true;
								}
								else
									params_matching_ok&= alias_template_params_to_signature_params_mapping[ dst_template_param->index ] == src_param;
							}
							else if( const auto dst_type= dst_param.GetType() )
							{
								// Trivial type in signature param.
								if( const auto src_type= src_param.GetType() )
									params_matching_ok= *src_type == *dst_type;
								else
									params_matching_ok= false;
							}
							else if( const auto dst_variable= dst_param.GetVariable() )
							{
								// Trivial variable in signature param.
								if( const auto src_variable= src_param.GetVariable() )
									params_matching_ok= *src_variable == *dst_variable;
								else
									params_matching_ok= false;
							}
							else if( const auto dst_type_template= dst_param.GetTypeTemplate() )
							{
								// Trivial type template in signature param.
								if( const auto src_type_template= src_param.GetTypeTemplate() )
									params_matching_ok= *src_type_template == *dst_type_template;
								else
									params_matching_ok= false;
							}
							else
							{
								// For now we support only trivial type alias templates.
								// Avoid performing deep matching.
								params_matching_ok= false;
							}
						}

						for( const bool& known : param_known_flags )
							params_matching_ok &= known;

						if( params_matching_ok )
						{
							llvm::SmallVector<bool, 32> alias_template_parameters_usage_flags;
							alias_template_parameters_usage_flags.resize( single_type_template->template_params.size(), false );

							const TemplateSignatureParam alias_body_signature_param=
								CreateTemplateSignatureParameter(
									*single_type_template->parent_namespace, // Use namespace of type alias to access proper names.
									function_context,
									single_type_template->template_params,
									alias_template_parameters_usage_flags,
									(*type_alias_ptr)->value );

							return MapTemplateParamsToSignatureParams( alias_template_params_to_signature_params_mapping, alias_body_signature_param );
						}
					}
				}
			}
		}

		std::vector<TemplateSignatureParam> type_templates;
		type_templates.reserve( type_templates_set->type_templates.size() );
		for( const TypeTemplatePtr& type_template : type_templates_set->type_templates )
			type_templates.push_back( TemplateSignatureParam::TypeTemplate{ type_template } );

		return TemplateSignatureParam::SpecializedTemplate{ std::move(type_templates), std::move(specialized_template_params) };
	}

	return ValueToTemplateParam( ResolveValueImpl( names_scope, function_context, template_parameterization ), names_scope, template_parameterization.src_loc );
}

TemplateSignatureParam CodeBuilder::ValueToTemplateParam( const Value& value, NamesScope& names_scope, const SrcLoc& src_loc )
{
	if( const auto type= value.GetTypeName() )
		return TemplateSignatureParam::Type{ *type };

	if( const auto variable= value.GetVariable() )
	{
		if( !variable->type.IsValidForTemplateVariableArgument() )
		{
			REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, names_scope.GetErrors(), src_loc, variable->type );
			return TemplateSignatureParam::Type{ invalid_type_ };
		}
		if( variable->constexpr_value == nullptr )
		{
			REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), src_loc );
			return TemplateSignatureParam::Type{ invalid_type_ };
		}
		return TemplateSignatureParam::Variable{ variable->type, variable->constexpr_value };
	}

	if( const auto type_templates_set = value.GetTypeTemplatesSet() )
	{
		if( type_templates_set->type_templates.size() == 1 )
			return TemplateSignatureParam::TypeTemplate{ type_templates_set->type_templates.front() };

		REPORT_ERROR( MoreThanOneTypeTemplateAsTemplateArgument, names_scope.GetErrors(), src_loc );
		return TemplateSignatureParam::Type{ invalid_type_ };
	}

	REPORT_ERROR( InvalidValueAsTemplateArgument, names_scope.GetErrors(), src_loc, value.GetKindName() );
	return TemplateSignatureParam::Type{ invalid_type_ };
}

bool CodeBuilder::MatchTemplateArg(
	const llvm::ArrayRef<TemplateParameter> template_params,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam& template_signature_param )
{
	return
		template_signature_param.Visit(
			[&]( const auto& param )
			{
				return MatchTemplateArgImpl( template_params, args_names_scope, template_arg, param );
			} );
}

bool CodeBuilder::MatchTemplateArgImpl(
	const llvm::ArrayRef<TemplateParameter> template_params,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::Type& type_param )
{
	(void)template_params;
	(void)args_names_scope;

	if( const auto given_type= std::get_if<Type>( &template_arg ) )
		return *given_type == type_param.t;
	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const llvm::ArrayRef<TemplateParameter> template_params,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::Variable& variable_param )
{
	(void)template_params;
	(void)args_names_scope;

	if( const auto given_variable= std::get_if<TemplateVariableArg>( &template_arg ) )
	{
		return
			given_variable->type == variable_param.type &&
			// LLVM constants are deduplicated, so, comparing pointers should work.
			given_variable->constexpr_value == variable_param.constexpr_value;
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const llvm::ArrayRef<TemplateParameter> template_params,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::TypeTemplate& type_template_param )
{
	(void)template_params;
	(void)args_names_scope;

	if( const auto given_type_template= std::get_if<TypeTemplatePtr>( &template_arg ) )
		return *given_type_template == type_template_param.type_template;

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const llvm::ArrayRef<TemplateParameter> template_params,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::TemplateParam& template_param )
{
	const std::string& name= template_params[ template_param.index ].name;

	NamesScopeValue* const value= args_names_scope.GetThisScopeValue( name );
	U_ASSERT( value != nullptr );
	if( value->value.GetYetNotDeducedTemplateArg() != nullptr )
	{
		const auto& kind_payload= template_params[ template_param.index ].kind_data;

		if( const auto given_type= std::get_if<Type>( &template_arg ) )
		{
			if( std::holds_alternative< TemplateParameter::TypeParamData >( kind_payload ) )
			{
				value->value= *given_type;
				return true;
			}
		}
		else if( const auto given_variable= std::get_if<TemplateVariableArg>( &template_arg ) )
		{
			if( const auto variable_param= std::get_if< TemplateParameter::VariableParamData >( &kind_payload ) )
			{
				if( !given_variable->type.IsValidForTemplateVariableArgument() || given_variable->constexpr_value == nullptr )
				{
					// May be in case of error.
					return false;
				}

				if( !MatchTemplateArg( template_params, args_names_scope, given_variable->type, variable_param->type ) )
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
		else if( const auto given_type_template= std::get_if<TypeTemplatePtr>( &template_arg ) )
		{
			if( std::holds_alternative< TemplateParameter::TypeTemplateParamData >( kind_payload ) )
			{
				TypeTemplatesSet type_templates_set;
				type_templates_set.type_templates.push_back( *given_type_template );
				value->value= std::move(type_templates_set);
				return true;
			}
		}
		else U_ASSERT(false);
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
				// LLVM constants are deduplicated, so, comparing pointers should work.
				given_variable->constexpr_value == prev_variable->constexpr_value;
		}
	}
	else if( const auto prev_type_templates_set = value->value.GetTypeTemplatesSet() )
	{
		if( const auto given_type_template = std::get_if<TypeTemplatePtr>( &template_arg ) )
			return
				prev_type_templates_set->type_templates.size() == 1 &&
				prev_type_templates_set->type_templates.front() == *given_type_template;
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const llvm::ArrayRef<TemplateParameter> template_params,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::Array& array_type_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_array_type= given_type->GetArrayType() )
		{
			if( !MatchTemplateArg( template_params, args_names_scope, given_array_type->element_type, *array_type_param.element_type ) )
				return false;

			TemplateVariableArg size_variable;
			size_variable.type= size_type_;
			size_variable.constexpr_value=
				llvm::ConstantInt::get( fundamental_llvm_types_.size_type_, given_array_type->element_count, false );

			return MatchTemplateArg( template_params, args_names_scope, size_variable, *array_type_param.element_count );
		}
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const llvm::ArrayRef<TemplateParameter> template_params,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::Tuple& tuple_type_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_tuple_type= given_type->GetTupleType() )
		{
			if( given_tuple_type->element_types.size() != tuple_type_param.element_types.size() )
				return false;

			for( size_t i= 0; i < tuple_type_param.element_types.size(); ++i )
			{
				if( !MatchTemplateArg( template_params, args_names_scope, given_tuple_type->element_types[i], tuple_type_param.element_types[i] ) )
					return false;
			}

			return true;
		}
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const llvm::ArrayRef<TemplateParameter> template_params,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::RawPointer& raw_pointer_type_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_raw_ponter_type= given_type->GetRawPointerType() )
		{
			return MatchTemplateArg( template_params, args_names_scope, given_raw_ponter_type->element_type, *raw_pointer_type_param.element_type );
		}
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const llvm::ArrayRef<TemplateParameter> template_params,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::Function& function_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_function_pointer_type= given_type->GetFunctionPointerType() )
		{
			const FunctionType& given_function_type= given_function_pointer_type->function_type;

			if( !(
				given_function_type.unsafe == function_param.is_unsafe &&
				given_function_type.calling_convention == function_param.calling_convention &&
				given_function_type.return_value_type == function_param.return_value_type &&
				MatchTemplateArg( template_params, args_names_scope, given_function_type.return_type, *function_param.return_type ) &&
				given_function_type.params.size() == function_param.params.size()
				) )
				return false;

			for( size_t i= 0; i < function_param.params.size(); ++i )
			{
				if( !(
					given_function_type.params[i].value_type == function_param.params[i].value_type &&
					MatchTemplateArg( template_params, args_names_scope, given_function_type.params[i].type, *function_param.params[i].type )
					) )
					return false;
			}

			return true;
		}
	}

	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const llvm::ArrayRef<TemplateParameter> template_params,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::Coroutine& coroutine_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_class= given_type->GetClassType() )
		{
			if( const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &given_class->generated_class_data ) )
			{
				return
					coroutine_type_description->kind == coroutine_param.kind &&
					coroutine_type_description->return_value_type == coroutine_param.return_value_type &&
					coroutine_type_description->return_references == coroutine_param.return_references &&
					coroutine_type_description->return_inner_references == coroutine_param.return_inner_references &&
					coroutine_type_description->inner_references == coroutine_param.inner_references &&
					coroutine_type_description->non_sync == coroutine_param.non_sync &&
					MatchTemplateArg( template_params, args_names_scope, coroutine_type_description->return_type, *coroutine_param.return_type );
			}
		}
	}
	return false;
}

bool CodeBuilder::MatchTemplateArgImpl(
	const llvm::ArrayRef<TemplateParameter> template_params,
	NamesScope& args_names_scope,
	const TemplateArg& template_arg,
	const TemplateSignatureParam::SpecializedTemplate& specialized_template_param )
{
	if( const auto given_type= std::get_if<Type>( &template_arg ) )
	{
		if( const auto given_class_type= given_type->GetClassType() )
		{
			if( const auto base_template= std::get_if< Class::BaseTemplate >( &given_class_type->generated_class_data ) )
			{
				if( specialized_template_param.params.size() > base_template->signature_args.size() )
					return false;

				for( const TemplateSignatureParam& type_template_signature_param : specialized_template_param.type_templates )
				{
					if( MatchTemplateArg( template_params, args_names_scope, base_template->class_template, type_template_signature_param ) )
					{
						for( size_t i= 0; i < specialized_template_param.params.size(); ++i )
						{
							if( !MatchTemplateArg( template_params, args_names_scope, base_template->signature_args[i], specialized_template_param.params[i] ) )
								return false;
						}

						// Check also for default arguments, if specialized template param has less params as given signature.
						for(size_t i= specialized_template_param.params.size(); i < base_template->signature_args.size(); ++i )
						{
							if( base_template->class_template->syntax_element->signature_params.size() <= i )
								return false;

							const auto& default_param_syntax_element= base_template->class_template->syntax_element->signature_params[i].default_value;
							if( std::holds_alternative<Synt::EmptyVariant>(default_param_syntax_element) )
								return false;

							NamesScope& given_class_template_args_scope= *given_class_type->members->GetParent();

							const Value default_param_value=
								WithGlobalFunctionContext(
									[&]( FunctionContext& function_context )
									{
										return BuildExpressionCode( default_param_syntax_element, given_class_template_args_scope, function_context );
									} );

							if( const auto template_arg_opt=
									ValueToTemplateArg(
										default_param_value,
										given_class_template_args_scope.GetErrors(),
										Synt::GetSrcLoc(default_param_syntax_element) ) )
							{
								// Check if given class template is instantiated with optional signature params equal to their default values.
								if( base_template->signature_args[i] != *template_arg_opt )
									return false;
							}
							else
								return false;
						}

						return true;
					}
				}
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

	REPORT_ERROR( CouldNotSelectMoreSpecializedTypeTemplate, arguments_names_scope.GetErrors(), src_loc );
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

	result.template_args_namespace= std::make_shared<NamesScope>( std::string( NamesScope::c_template_args_namespace_name ), type_template.parent_namespace );
	for( const TemplateParameter& param : type_template.template_params )
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

			const Value value=
				WithGlobalFunctionContext(
					[&]( FunctionContext& function_context )
					{
						return BuildExpressionCode( expr, *result.template_args_namespace, function_context );
					} );

			auto template_arg_opt= ValueToTemplateArg( value, result.template_args_namespace->GetErrors(), Synt::GetSrcLoc(expr) );
			if( template_arg_opt != std::nullopt )
				out_signature_arg= std::move( *template_arg_opt );
		}

		if( !MatchTemplateArg( type_template.template_params, *result.template_args_namespace, out_signature_arg, type_template.signature_params[i] ) )
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
				if( value->value.GetTypeAlias() != nullptr )
					GlobalThingBuildTypeAlias( *template_parameters_space, value->value ); // Possible detect globals loop here.
				if( const auto type= value->value.GetTypeName() )
					return *type;
			}
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

		// Expand mixins just in class template instantiation.
		// This is the only proper place to do so.
		// Expanding mixins later isn't possible.
		ProcessClassMixins( class_type );

		return Type(class_type);
	}
	if( const auto type_alias= std::get_if< std::unique_ptr<const Synt::TypeAlias> >( &type_template.syntax_element->something ) )
	{
		Value& type_alias_value=
			template_args_namespace->AddName(
				Class::c_template_class_name,
				NamesScopeValue( TypeAlias{ type_alias->get() }, (*type_alias)->src_loc ) )->value;

		// Call this function to enable globals loop detection.
		GlobalThingBuildTypeAlias( *template_args_namespace, type_alias_value );

		if(const auto type= type_alias_value.GetTypeName() )
			return *type;
	}
	else U_ASSERT(false);

	return std::nullopt;
}

CodeBuilder::TemplateFunctionPreparationResult CodeBuilder::PrepareTemplateFunction(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const FunctionTemplatePtr& function_template_ptr,
	const llvm::ArrayRef<FunctionType::Param> actual_args,
	const bool first_actual_arg_is_this,
	const bool enable_type_conversions )
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

	result.template_args_namespace= std::make_shared<NamesScope>( std::string( NamesScope::c_template_args_namespace_name ), function_template.parent_namespace );
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
			if( type == given_type ||
				ReferenceIsConvertible( given_type, type, errors_container, src_loc ) ||
				( !expected_arg_is_mutalbe_reference && enable_type_conversions && HasConversionConstructor( given_args[i], type, errors_container, src_loc ) ) )
				deduced_specially= true;
		}
		else if( const auto template_param= signature_param.GetTemplateParam() )
		{
			if( const auto type= result.template_args_namespace->GetThisScopeValue( function_template.template_params[ template_param->index ].name )->value.GetTypeName() )
			{
				if( *type == given_type ||
					ReferenceIsConvertible( given_type, *type, errors_container, src_loc ) ||
					( !expected_arg_is_mutalbe_reference && enable_type_conversions && HasConversionConstructor( given_args[i], *type, errors_container, src_loc ) ) )
					deduced_specially= true;
			}
		}

		if( !deduced_specially && !MatchTemplateArg( function_template.template_params, *result.template_args_namespace, given_type, signature_param ) )
			return result;

	} // for template function arguments

	// Process "enable_if" here - fail template function preparation if condition is false.
	if( std::get_if<Synt::EmptyVariant>( &function_declaration.condition ) == nullptr )
	{
		const bool res=
			WithGlobalFunctionContext(
				[&]( FunctionContext& function_context )
				{
					return EvaluateBoolConstantExpression( *result.template_args_namespace, function_context, function_declaration.condition );
				});

		if( !res )
			return result;
	}

	result.function_template= function_template_ptr;
	return result;
}

const FunctionVariable* CodeBuilder::FinishTemplateFunctionParameterization(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const FunctionTemplatePtr& function_template_ptr )
{
	const FunctionTemplate& function_template= *function_template_ptr;

	if( function_template_ptr->template_params.size() != function_template_ptr->known_template_args.size() )
		return nullptr;

	TemplateFunctionPreparationResult result;
	result.function_template= function_template_ptr;
	result.template_args_namespace= std::make_shared<NamesScope>( std::string( NamesScope::c_template_args_namespace_name ), function_template.parent_namespace );
	FillKnownFunctionTemplateArgsIntoNamespace( function_template, *result.template_args_namespace );

	return FinishTemplateFunctionGeneration( errors_container, src_loc, result );
}

const FunctionVariable* CodeBuilder::FinishTemplateFunctionGeneration(
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const TemplateFunctionPreparationResult& template_function_preparation_result )
{
	// Use inaccessible name instead of proper function name in order to avoid shadowing function template by the instantiated function.
	// This is needed in order to call properly overloaded functions and function templates with the same name within template functions.
	const std::string_view func_name_in_namespace= "_";

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
		else if( const auto type_template= value->value.GetTypeTemplatesSet() )
		{
			U_ASSERT( type_template->type_templates.size() == 1 );
			template_args.push_back( type_template->type_templates.front() );
		}
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
		OverloadedFunctionsSet& result_functions_set= *template_parameters_space->GetThisScopeValue( func_name_in_namespace )->value.GetFunctionsSet();
		if( !result_functions_set.functions.empty() )
			return &result_functions_set.functions.front();
		else
			return nullptr; // May be in case of error or in case of "enable_if".
	}
	AddNewTemplateThing( std::move(template_key), template_args_namespace );

	CreateTemplateErrorsContext( errors_container, src_loc, template_args_namespace, function_template, func_name );

	// First, prepare only as prototype.
	NamesScopeFillFunction( *template_args_namespace, *function_template.syntax_element->function, func_name_in_namespace, function_template.base_class, ClassMemberVisibility::Public );
	OverloadedFunctionsSet& result_functions_set= *template_args_namespace->GetThisScopeValue( func_name_in_namespace )->value.GetFunctionsSet();
	PrepareFunctionsSet( *template_args_namespace, result_functions_set );

	if( result_functions_set.functions.empty() )
		return nullptr; // Function prepare failed

	FunctionVariable& function_variable= result_functions_set.functions.front();
	if( function_variable.constexpr_kind == FunctionVariable::ConstexprKind::NonConstexpr )
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
	if( !function_variable.has_body &&
		!( skip_building_generated_functions_ && function_variable.constexpr_kind == FunctionVariable::ConstexprKind::NonConstexpr ) &&
		function_declaration.block != nullptr )
		BuildFuncCode( function_variable, function_template.base_class, *template_args_namespace, func_name );

	// Two-step preparation needs for recursive function template call.

	return &function_variable;
}

OverloadedFunctionsSetPtr CodeBuilder::ParameterizeFunctionTemplate(
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

	ParameterizedFunctionTemplateKey template_key{ functions_set_ptr, arguments_calculated };

	if( const auto it= parameterized_template_functions_cache_.find( template_key ); it != parameterized_template_functions_cache_.end() )
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
		for( const TemplateParameter& param : function_template.template_params )
			args_names_scope.AddName( param.name, NamesScopeValue( YetNotDeducedTemplateArg(), param.src_loc ) );

		bool ok= true;
		for( size_t i= 0u; i < arguments_calculated.size(); ++i )
		{
			ok&= MatchTemplateArg(
				function_template.template_params,
				args_names_scope,
				arguments_calculated[i],
				TemplateSignatureParam::TemplateParam{ i, function_template.template_params[i].kind_data.index() } );
		}

		if( !ok )
			continue;

		const auto new_template= std::make_shared<FunctionTemplate>(function_template);
		new_template->parent= function_template_ptr;
		new_template->known_template_args.insert( new_template->known_template_args.end(), arguments_calculated.begin(), arguments_calculated.end() );

		// Try to add further args, which was calculated indirectly.
		for( size_t i= new_template->known_template_args.size(); i < function_template.template_params.size(); ++i )
		{
			if( const auto val= args_names_scope.GetThisScopeValue( function_template.template_params[i].name ) )
			{
				if( const auto type= val->value.GetTypeName() )
					new_template->known_template_args.push_back( *type );
				else if( const auto variable= val->value.GetVariable() )
					new_template->known_template_args.push_back( TemplateVariableArg( *variable ) );
				else if( const auto type_templates_set= val->value.GetTypeTemplatesSet() )
				{
					if( type_templates_set->type_templates.size() == 1 )
						new_template->known_template_args.push_back( type_templates_set->type_templates.front() );
					else
						break;
				}
				else
					break; // End at first yet not known argument.
			}
		}

		result->template_functions.push_back( new_template );
	} // for function templates

	if( result->template_functions.empty() )
	{
		REPORT_ERROR( TemplateFunctionGenerationFailed, arguments_names_scope.GetErrors(), src_loc, function_templates.front()->syntax_element->function->name.back().name );
		return nullptr;
	}

	return
		parameterized_template_functions_cache_.insert(
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

	for( const Synt::Expression& expr : template_arguments )
	{
		const Value value= BuildExpressionCode( expr, arguments_names_scope, function_context );
		auto template_arg_opt= ValueToTemplateArg( value, arguments_names_scope.GetErrors(), Synt::GetSrcLoc(expr) );
		if( template_arg_opt != std::nullopt )
			out_args.push_back( std::move( *template_arg_opt ) );
	}

	DestroyUnusedTemporaryVariables( function_context, arguments_names_scope.GetErrors(), src_loc );
}

std::optional<TemplateArg> CodeBuilder::ValueToTemplateArg( const Value& value, CodeBuilderErrorsContainer& errors, const SrcLoc& src_loc )
{
	if( const Type* const type_name= value.GetTypeName() )
		return TemplateArg( *type_name );

	if( const auto type_templates_set= value.GetTypeTemplatesSet() )
	{
		// For now support only single type templates as template arguments.
		// It's too complicated to deal with sets of multiple templates - too complex to compare them, calculate specialization, mangle.

		if( type_templates_set->type_templates.size() == 1 )
			return type_templates_set->type_templates.front();

		REPORT_ERROR( MoreThanOneTypeTemplateAsTemplateArgument, errors, src_loc );
		return std::nullopt;
	}

	if( const auto variable= value.GetVariable() )
	{
		if( !variable->type.IsValidForTemplateVariableArgument() )
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
			else if( const auto type_template= std::get_if<TypeTemplatePtr>( &known_template_arg ) )
			{
				TypeTemplatesSet type_templates_set;
				type_templates_set.type_templates.push_back( *type_template );
				v= std::move(type_templates_set);
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

} // namespace U
