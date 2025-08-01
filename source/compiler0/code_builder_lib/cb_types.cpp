#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/MD5.h>
#include <llvm/Support/ConvertUTF.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"


namespace U
{

Type CodeBuilder::PrepareType(
	const Synt::TypeName& type_name,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	return
		std::visit(
		[&](const auto& t)
		{
			return PrepareTypeImpl( names_scope, function_context, t );
		},
		type_name);
}

Type CodeBuilder::PrepareTypeImpl( NamesScope&, FunctionContext&, const Synt::EmptyVariant& )
{
	U_ASSERT(false);
	return invalid_type_;
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookup& root_namespace_lookup )
{
	return ValueToType( names_scope, ResolveValueImpl( names_scope, function_context, root_namespace_lookup ), root_namespace_lookup.src_loc );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookupCompletion& root_namespace_lookup_completion )
{
	return ValueToType( names_scope, ResolveValueImpl( names_scope, function_context, root_namespace_lookup_completion ), root_namespace_lookup_completion.src_loc );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookup& name_lookup )
{
	return ValueToType( names_scope, ResolveValueImpl( names_scope, function_context, name_lookup ), name_lookup.src_loc );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookupCompletion& name_lookup_completion )
{
	return ValueToType( names_scope, ResolveValueImpl( names_scope, function_context, name_lookup_completion ), name_lookup_completion.src_loc );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetch& names_scope_name_fetch )
{
	return ValueToType( names_scope, ResolveValueImpl( names_scope, function_context, names_scope_name_fetch ), names_scope_name_fetch.src_loc );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetchCompletion& names_scope_name_fetch_completion )
{
	return ValueToType( names_scope, ResolveValueImpl( names_scope, function_context, names_scope_name_fetch_completion ), names_scope_name_fetch_completion.src_loc );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TemplateParameterization& template_parameterization )
{
	return ValueToType( names_scope, ResolveValueImpl( names_scope, function_context, template_parameterization ), template_parameterization.src_loc );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ArrayTypeName& array_type_name )
{
	ArrayType array_type;
	array_type.element_type= PrepareType( array_type_name.element_type, names_scope, function_context );

	const Synt::Expression& num= array_type_name.size;
	const SrcLoc num_src_loc= Synt::GetSrcLoc( num );

	VariablePtr size_variable;
	{
		const StackVariablesStorage dummy_stack_variables_storage( function_context );
		size_variable= BuildExpressionCodeEnsureVariable( num, names_scope, function_context );
	}

	if( size_variable->constexpr_value != nullptr )
	{
		if( const FundamentalType* const size_fundamental_type= size_variable->type.GetFundamentalType() )
		{
			if( IsInteger( size_fundamental_type->fundamental_type ) )
			{
				const llvm::APInt& size_value= size_variable->constexpr_value->getUniqueInteger();
				if( IsSignedInteger( size_fundamental_type->fundamental_type ) && size_value.isNegative() )
					REPORT_ERROR( ArraySizeIsNegative, names_scope.GetErrors(), num_src_loc );
				else
					array_type.element_count= size_value.getLimitedValue();
			}
			else
				REPORT_ERROR( ArraySizeIsNotInteger, names_scope.GetErrors(), num_src_loc );
		}
		else
			REPORT_ERROR( ArraySizeIsNotInteger, names_scope.GetErrors(), num_src_loc );
	}
	else
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), num_src_loc );

	// TODO - generate error, if total size of type (incuding arrays) is more, than half of address space of target architecture.
	array_type.llvm_type= llvm::ArrayType::get( array_type.element_type.GetLLVMType(), array_type.element_count );
	return std::move(array_type);
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TypeofTypeName& typeof_type_name )
{
	Type result;

	const bool prev_is_functionless_context= function_context.is_functionless_context;
	function_context.is_functionless_context= true;

	const auto state= SaveFunctionContextState( function_context );
	{
		const StackVariablesStorage dummy_stack_variables_storage( function_context );
		const VariablePtr variable= BuildExpressionCodeEnsureVariable( typeof_type_name.expression, names_scope, function_context );
		result= variable->type;
	}

	RestoreFunctionContextState( function_context, state );
	function_context.is_functionless_context= prev_is_functionless_context;

	return result;
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::FunctionType& function_type_name )
{
	if( function_type_name.IsAutoReturn() )
		REPORT_ERROR( AutoForFunctionTypeReturnType, names_scope.GetErrors(), function_type_name.src_loc );

	return FunctionTypeToPointer( PrepareFunctionType( names_scope, function_context, function_type_name ) );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TupleType& tuple_type_name )
{
	TupleType tuple;
	tuple.element_types.reserve( tuple_type_name.element_types.size() );

	llvm::SmallVector<llvm::Type*, 16> elements_llvm_types;
	elements_llvm_types.reserve( tuple_type_name.element_types.size() );

	for( const Synt::TypeName& element_type_name : tuple_type_name.element_types )
	{
		Type element_type= PrepareType( element_type_name, names_scope, function_context );
		if( element_type == invalid_type_ )
			return invalid_type_;

		elements_llvm_types.push_back( element_type.GetLLVMType() );
		tuple.element_types.push_back( std::move(element_type) );
	}

	tuple.llvm_type= llvm::StructType::get( llvm_context_, elements_llvm_types );

	return std::move(tuple);
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RawPointerType& raw_pointer_type_name )
{
	RawPointerType raw_pointer;
	raw_pointer.element_type= PrepareType( raw_pointer_type_name.element_type, names_scope, function_context );
	raw_pointer.llvm_type= raw_pointer.element_type.GetLLVMType()->getPointerTo();

	return raw_pointer;
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::CoroutineType& coroutine_type_name )
{
	CoroutineTypeDescription coroutine_type_description;
	coroutine_type_description.kind= coroutine_type_name.kind;

	coroutine_type_description.return_type= PrepareType( coroutine_type_name.return_type, names_scope, function_context );

	if( coroutine_type_name.return_value_reference_modifier == ReferenceModifier::Reference )
		coroutine_type_description.return_value_type=
			coroutine_type_name.return_value_mutability_modifier == MutabilityModifier::Mutable
				? ValueType::ReferenceMut
				: ValueType::ReferenceImut;
	else
		coroutine_type_description.return_value_type= ValueType::Value;

	coroutine_type_description.inner_references.reserve( coroutine_type_name.inner_references.size() );
	for( const Synt::MutabilityModifier m : coroutine_type_name.inner_references )
		coroutine_type_description.inner_references.push_back( m == MutabilityModifier::Mutable ? InnerReferenceKind::Mut : InnerReferenceKind::Imut );

	coroutine_type_description.non_sync= ImmediateEvaluateNonSyncTag( names_scope, function_context, coroutine_type_name.non_sync_tag );

	if( !coroutine_type_description.non_sync && GetTypeNonSync( coroutine_type_description.return_type, names_scope, coroutine_type_name.src_loc ) )
		REPORT_ERROR( CoroutineNonSyncRequired, names_scope.GetErrors(), coroutine_type_name.src_loc );

	const size_t num_params= 1;
	if( coroutine_type_name.return_value_reference_expression != nullptr )
		coroutine_type_description.return_references= EvaluateFunctionReturnReferences( names_scope, function_context, *coroutine_type_name.return_value_reference_expression, num_params );
	if( coroutine_type_name.return_value_inner_references_expression != nullptr )
		coroutine_type_description.return_inner_references= EvaluateFunctionReturnInnerReferences( names_scope, function_context, *coroutine_type_name.return_value_inner_references_expression, num_params );

	return GetCoroutineType( *names_scope.GetRoot(), coroutine_type_description );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::Mixin& mixin_type_name )
{
	if( const auto type_name= ExpandTypeNameMixin( names_scope, function_context, mixin_type_name ) )
		return PrepareType( *type_name, names_scope, function_context );
	return invalid_type_;
}

Type CodeBuilder::ValueToType( const NamesScope& names_scope, const Value& value, const SrcLoc& src_loc )
{
	if( const Type* const type= value.GetTypeName() )
		return *type;
	else
		REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), src_loc, value.GetKindName() );

	return invalid_type_;
}

FunctionType CodeBuilder::PrepareFunctionType( NamesScope& names_scope, FunctionContext& function_context, const Synt::FunctionType& function_type_name, const ClassPtr class_ )
{
	FunctionType function_type;

	if( function_type_name.return_type == nullptr )
		function_type.return_type= void_type_;
	else if( function_type_name.IsAutoReturn() )
		function_type.return_type= void_type_;
	else
		function_type.return_type= PrepareType( *function_type_name.return_type, names_scope, function_context );

	if( function_type_name.return_value_reference_modifier == ReferenceModifier::None )
		function_type.return_value_type= ValueType::Value;
	else
		function_type.return_value_type= function_type_name.return_value_mutability_modifier == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;

	function_type.params.reserve( function_type_name.params.size() );
	for( const Synt::FunctionParam& in_param : function_type_name.params )
	{
		FunctionType::Param out_param;

		const bool is_this=
			&in_param == &function_type_name.params.front() &&
			in_param.name == Keywords::this_ &&
			std::holds_alternative<Synt::EmptyVariant>(in_param.type);
		if( is_this )
		{
			if( class_ == nullptr )
			{
				REPORT_ERROR( ThisInNonclassFunction, names_scope.GetErrors(), in_param.src_loc );
				out_param.type= invalid_type_;
			}
			else
				out_param.type= class_;
		}
		else
		{
			if( IsKeyword( in_param.name ) )
				REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), in_param.src_loc );

			out_param.type= PrepareType( in_param.type, names_scope, function_context );
		}

		if( in_param.reference_modifier == Synt::ReferenceModifier::None )
			out_param.value_type= ValueType::Value;
		else if( in_param.mutability_modifier == Synt::MutabilityModifier::Mutable )
			out_param.value_type= ValueType::ReferenceMut;
		else
			out_param.value_type= ValueType::ReferenceImut;


		function_type.params.push_back( std::move(out_param) );
	}

	function_type.unsafe= function_type_name.unsafe;
	function_type.calling_convention= PrepareCallingConvention( names_scope, function_context, function_type_name.calling_convention );

	const size_t num_params= function_type.params.size();
	if( function_type_name.references_pollution_expression != nullptr )
		function_type.references_pollution= EvaluateFunctionReferencePollution( names_scope, function_context, *function_type_name.references_pollution_expression, num_params );
	if( function_type_name.return_value_reference_expression != nullptr )
		function_type.return_references= EvaluateFunctionReturnReferences( names_scope, function_context, *function_type_name.return_value_reference_expression, num_params );
	if( function_type_name.return_value_inner_references_expression != nullptr )
		function_type.return_inner_references= EvaluateFunctionReturnInnerReferences( names_scope, function_context, *function_type_name.return_value_inner_references_expression, num_params );

	// Generate mapping of input references to output references if return reference notation is not specified.
	// Assume that immutable return reference may point to any reference param and mutable return reference only to mutable reference params.
	if( function_type.return_value_type != ValueType::Value &&
		function_type_name.return_value_reference_expression == nullptr &&
		function_type_name.return_value_inner_references_expression == nullptr )
	{
		for( size_t i= 0u; i < function_type.params.size(); ++i )
		{
			if( ( function_type.return_value_type == ValueType::ReferenceImut && function_type.params[i].value_type != ValueType::Value ) ||
				( function_type.return_value_type == ValueType::ReferenceMut  && function_type.params[i].value_type == ValueType::ReferenceMut ) )
				function_type.return_references.emplace_back( i, FunctionType::c_param_reference_number );
		}
		NormalizeParamReferencesList( function_type.return_references );
	}

	return function_type;
}

FunctionPointerType CodeBuilder::FunctionTypeToPointer( FunctionType function_type )
{
	return FunctionPointerType{ std::move(function_type), llvm::PointerType::get( llvm_context_, 0 ) };
}

llvm::FunctionType* CodeBuilder::GetLLVMFunctionType( const FunctionType& function_type )
{
	if( function_type.return_value_type == ValueType::Value )
	{
		// Require complete type in order to know how to return values.
		EnsureTypeComplete( function_type.return_type );
	}

	for( const FunctionType::Param& param : function_type.params )
	{
		if( param.value_type == ValueType::Value )
		{
			// Require complete type in order to know how to pass value args.
			EnsureTypeComplete( param.type );
		}
	}

	const ICallingConventionInfo::CallInfo call_info=
		calling_convention_infos_[ size_t( function_type.calling_convention ) ]->CalculateFunctionCallInfo( function_type );

	return GetLLVMFunctionType( function_type, call_info );
}

llvm::FunctionType* CodeBuilder::GetLLVMFunctionType(
	const FunctionType& function_type,
	const ICallingConventionInfo::CallInfo& call_info )
{
	llvm::Type* llvm_function_return_type= nullptr;
	llvm::SmallVector<llvm::Type*, 16> params_llvm_types;

	if( function_type.return_value_type == ValueType::Value )
	{
		if( function_type.return_type == void_type_ )
		{
			// Use true "void" LLVM type only for function return value. Use own "void" type in other cases.
			llvm_function_return_type= fundamental_llvm_types_.void_for_ret_;
		}
		else
		{
			switch( call_info.return_value_passing.kind )
			{
			case ICallingConventionInfo::ReturnValuePassingKind::Direct:
			case ICallingConventionInfo::ReturnValuePassingKind::DirectZExt:
			case ICallingConventionInfo::ReturnValuePassingKind::DirectSExt:
				llvm_function_return_type= call_info.return_value_passing.llvm_type;
				break;
			case ICallingConventionInfo::ReturnValuePassingKind::ByPointer:
				llvm_function_return_type= fundamental_llvm_types_.void_for_ret_;
				params_llvm_types.push_back( function_type.return_type.GetLLVMType()->getPointerTo() );
			}
		}
	}
	else
		llvm_function_return_type= function_type.return_type.GetLLVMType()->getPointerTo();

	for( size_t i= 0; i < function_type.params.size(); ++i )
		params_llvm_types.push_back( call_info.arguments_passing[i].llvm_type );

	return llvm::FunctionType::get( llvm_function_return_type, params_llvm_types, false );
}

CallingConvention CodeBuilder::PrepareCallingConvention(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const std::unique_ptr<const Synt::Expression>& calling_convention_name )
{
	if( calling_convention_name == nullptr )
		return CallingConvention::Default;

	const Synt::Expression& expr= *calling_convention_name;

	const VariablePtr v= BuildExpressionCodeEnsureVariable( expr, names_scope, function_context );

	if( const auto array_type= v->type.GetArrayType() )
	{
		if( const auto fundamental_type= array_type->element_type.GetFundamentalType() )
		{
			if( fundamental_type->fundamental_type == U_FundamentalType::char8_ )
			{
				if( v->constexpr_value != nullptr )
				{
					if( const auto constant_data= llvm::dyn_cast<llvm::ConstantDataArray>( v->constexpr_value ) )
					{
						const std::string_view name_str= constant_data->getRawDataValues();
						if( const auto cc_opt= StringToCallingConvention( name_str ) )
							return *cc_opt;

						REPORT_ERROR( UnknownCallingConvention, names_scope.GetErrors(), Synt::GetSrcLoc( expr ), name_str );
						return CallingConvention::Default;
					}
					else
					{
						REPORT_ERROR( UnknownCallingConvention, names_scope.GetErrors(), Synt::GetSrcLoc( expr ), "<non-trivial constant expression>" );
						return CallingConvention::Default;
					}
				}
				else
				{
					REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), Synt::GetSrcLoc( expr ) );
					return CallingConvention::Default;
				}
			}
		}
	}

	REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), Synt::GetSrcLoc( expr ), "char8 array", v->type );
	return CallingConvention::Default;
}

llvm::CallingConv::ID CodeBuilder::GetLLVMCallingConvention( const CallingConvention calling_convention )
{
	switch( calling_convention )
	{
	case CallingConvention::Default:
		return llvm::CallingConv::Fast; // Use fast calling convention, which should be generally faster compared to C calling convention.
	case CallingConvention::C:
		return llvm::CallingConv::C;
	case CallingConvention::Fast:
		return llvm::CallingConv::Fast;
	case CallingConvention::Cold:
		return llvm::CallingConv::Cold;
	case CallingConvention::System:
		if( target_triple_.getArch() == llvm::Triple::x86 && target_triple_.getOS() == llvm::Triple::Win32 )
			return llvm::CallingConv::X86_StdCall;
		else
			return llvm::CallingConv::C;
	}

	U_ASSERT(false);
	return llvm::CallingConv::C;
}

} // namespace U
