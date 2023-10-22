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

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const std::unique_ptr<const Synt::NamesScopeNameFetch>& names_scope_name_fetch )
{
	return ValueToType( names_scope, ResolveValueImpl( names_scope, function_context, names_scope_name_fetch ), names_scope_name_fetch->src_loc );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const std::unique_ptr<const Synt::NamesScopeNameFetchCompletion>& names_scope_name_fetch_completion )
{
	return ValueToType( names_scope, ResolveValueImpl( names_scope, function_context, names_scope_name_fetch_completion ), names_scope_name_fetch_completion->src_loc );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const std::unique_ptr<const Synt::TemplateParametrization>& template_parametrization )
{
	return ValueToType( names_scope, ResolveValueImpl( names_scope, function_context, template_parametrization ), template_parametrization->src_loc );
}

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ArrayTypeName& array_type_name )
{
	ArrayType array_type;
	array_type.element_type= PrepareType( array_type_name.element_type, names_scope, function_context );

	const Synt::Expression& num= array_type_name.size;
	const SrcLoc num_src_loc= Synt::GetExpressionSrcLoc( num );

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

Type CodeBuilder::PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::GeneratorType& generator_type_name )
{
	CoroutineTypeDescription coroutine_type_description;
	coroutine_type_description.kind= CoroutineKind::Generator;
	coroutine_type_description.return_type= PrepareType( generator_type_name.return_type, names_scope, function_context );

	if( generator_type_name.return_value_reference_modifier == ReferenceModifier::Reference )
		coroutine_type_description.return_value_type=
			generator_type_name.return_value_mutability_modifier == MutabilityModifier::Mutable
				? ValueType::ReferenceMut
				: ValueType::ReferenceImut;
	else
		coroutine_type_description.return_value_type= ValueType::Value;

	if( generator_type_name.inner_reference_tag == nullptr )
		coroutine_type_description.inner_reference_type= std::nullopt;
	else
		coroutine_type_description.inner_reference_type=
			generator_type_name.inner_reference_tag->mutability_modifier == MutabilityModifier::Mutable
				? InnerReferenceType::Mut
				: InnerReferenceType::Imut;

	coroutine_type_description.non_sync= ImmediateEvaluateNonSyncTag( names_scope, function_context, generator_type_name.non_sync_tag );

	if( !coroutine_type_description.non_sync && GetTypeNonSync( coroutine_type_description.return_type, names_scope, generator_type_name.src_loc ) )
		REPORT_ERROR( GeneratorNonSyncRequired, names_scope.GetErrors(), generator_type_name.src_loc );

	if( !generator_type_name.return_value_reference_tag.empty() )
	{
		bool found= false;
		if( generator_type_name.inner_reference_tag != nullptr && generator_type_name.inner_reference_tag->name == generator_type_name.return_value_reference_tag )
			found= true;

		if( !found )
			REPORT_ERROR( NameNotFound, names_scope.GetErrors(), generator_type_name.src_loc, generator_type_name.return_value_reference_tag );
	}
	for( const std::string& tag : generator_type_name.return_value_inner_reference_tags )
	{
		bool found= false;
		if( generator_type_name.inner_reference_tag != nullptr && generator_type_name.inner_reference_tag->name == tag )
			found= true;

		if( !found )
			REPORT_ERROR( NameNotFound, names_scope.GetErrors(), generator_type_name.src_loc, tag );
	}

	// For now there is no reason to process reference tag.
	// Assume, that if generator returns a reference, it points to single possible reference tag - inner reference tag.

	return GetCoroutineType( *names_scope.GetRoot(), coroutine_type_description );
}

Type CodeBuilder::ValueToType( NamesScope& names_scope, const Value& value, const SrcLoc& src_loc )
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
	else
	{
		bool is_auto_return= false;
		if( const auto name_lookup = std::get_if<Synt::NameLookup>( function_type_name.return_type.get() ) )
			if( name_lookup->name == Keywords::auto_ )
				is_auto_return= true;

		if( is_auto_return )
			function_type.return_type= void_type_;
		else
			function_type.return_type= PrepareType( *function_type_name.return_type, names_scope, function_context );
	}

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
			std::get_if<Synt::EmptyVariant>(&in_param.type) != nullptr;
		if( is_this )
		{
			if( class_ == nullptr )
			{
				REPORT_ERROR( ThisInNonclassFunction, names_scope.GetErrors(), in_param.src_loc, "TODO - func name" );
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

		ProcessFunctionParamReferencesTags( function_type_name, function_type, in_param, out_param, function_type.params.size() );

		function_type.params.push_back( std::move(out_param) );
	}

	function_type.unsafe= function_type_name.unsafe;
	function_type.calling_convention= GetLLVMCallingConvention( function_type_name.calling_convention, function_type_name.src_loc, names_scope.GetErrors() );

	ProcessFunctionReturnValueReferenceTags( names_scope.GetErrors(), function_type_name, function_type );
	TryGenerateFunctionReturnReferencesMapping( function_type_name, function_type );

	if( function_type_name.return_value_reference_expression != nullptr )
		function_type.return_references= EvaluateFunctionReturnReferences( names_scope, *function_type_name.return_value_reference_expression );

	if( function_type_name.references_pollution_expression != nullptr )
		function_type.references_pollution= EvaluateFunctionReferencePollution( names_scope, *function_type_name.references_pollution_expression );

	return function_type;
}

FunctionPointerType CodeBuilder::FunctionTypeToPointer( FunctionType function_type )
{
	return FunctionPointerType{ std::move(function_type), llvm::PointerType::get( llvm_context_, 0 ) };
}

llvm::FunctionType* CodeBuilder::GetLLVMFunctionType( const FunctionType& function_type )
{
	llvm::SmallVector<llvm::Type*, 16> args_llvm_types;

	// Require complete type in order to know how to return values.
	if( function_type.return_value_type == ValueType::Value )
		EnsureTypeComplete( function_type.return_type );

	const bool first_arg_is_sret= FunctionTypeIsSRet( function_type );

	if( first_arg_is_sret )
		args_llvm_types.push_back( function_type.return_type.GetLLVMType()->getPointerTo() );

	for( const FunctionType::Param& param : function_type.params )
	{
		llvm::Type* type= param.type.GetLLVMType();
		if( param.value_type == ValueType::Value )
		{
			// Require complete type in order to know how to pass value args.
			if( EnsureTypeComplete( param.type ) )
			{
				if( param.type.GetFundamentalType() != nullptr ||
					param.type.GetEnumType() != nullptr ||
					param.type.GetRawPointerType() != nullptr ||
					param.type.GetFunctionPointerType() )
				{}
				else if( param.type.GetClassType() != nullptr || param.type.GetArrayType() != nullptr || param.type.GetTupleType() != nullptr )
				{
					if( const auto single_scalar= GetSingleScalarType( param.type.GetLLVMType() ) )
					{
						// Pass composite types with single scalar inside in register, using type of this scalar.
						type= single_scalar;
					}
					else
					{
						// Pass composite types by hidden pointer.
						type= type->getPointerTo();
					}
				}
				else U_ASSERT( false );
			}
		}
		else
		{
			// Do not need to have complete type in order to know how to pass reference args.
			// It is important not to trigger type completeness build, since this function may be called within class build code,
			// in case of special and/or generated methods.
			type= type->getPointerTo();
		}

		args_llvm_types.push_back( type );
	}

	llvm::Type* llvm_function_return_type= function_type.return_type.GetLLVMType();
	if( function_type.return_value_type == ValueType::Value )
	{
		if( first_arg_is_sret || function_type.return_type == void_type_ )
		{
			// Use true "void" LLVM type only for function return value. Use own "void" type in other cases.
			llvm_function_return_type= fundamental_llvm_types_.void_for_ret_;
		}
		else
		{
			llvm_function_return_type= GetSingleScalarType( function_type.return_type.GetLLVMType() );
			U_ASSERT( llvm_function_return_type != nullptr );
		}
	}
	else
		llvm_function_return_type= llvm_function_return_type->getPointerTo();

	return llvm::FunctionType::get( llvm_function_return_type, args_llvm_types, false );
}

llvm::CallingConv::ID CodeBuilder::GetLLVMCallingConvention(
	const std::optional<std::string>& calling_convention_name,
	const SrcLoc& src_loc,
	CodeBuilderErrorsContainer& errors )
{
	if( calling_convention_name == std::nullopt )
		return llvm::CallingConv::C;

	if( *calling_convention_name == "C" ||
		*calling_convention_name == "default" ||
		*calling_convention_name == "Ü" )
		return llvm::CallingConv::C;

	if( *calling_convention_name == "fast" )
		return llvm::CallingConv::Fast;

	if( *calling_convention_name == "cold" )
		return llvm::CallingConv::Cold;

	if( *calling_convention_name == "system" )
	{
		if( target_triple_.getArch() == llvm::Triple::x86 && target_triple_.getOS() == llvm::Triple::Win32 )
			return llvm::CallingConv::X86_StdCall;

		return llvm::CallingConv::C;
	}

	REPORT_ERROR( UnknownCallingConvention, errors, src_loc, *calling_convention_name );
	return llvm::CallingConv::C;
}

bool CodeBuilder::FunctionTypeIsSRet( const FunctionType& function_type )
{
	return
		function_type.ReturnsCompositeValue() &&
		GetSingleScalarType( function_type.return_type.GetLLVMType() ) == nullptr;
}

llvm::Type* CodeBuilder::GetSingleScalarType( llvm::Type* type )
{
	U_ASSERT( type->isSized() && "expected sized type!" );

	while( true )
	{
		if( type->isStructTy() && type->getStructNumElements() == 1 )
		{
			type= type->getStructElementType(0);
			continue;
		}
		if( type->isArrayTy() && type->getArrayNumElements() == 1 )
		{
			type= type->getArrayElementType();
			continue;
		}

		break; // Not a composite.
	}

	if( type->isIntegerTy() || type->isFloatingPointTy() || type->isPointerTy() )
		return type;

	return nullptr;
}

} // namespace U
