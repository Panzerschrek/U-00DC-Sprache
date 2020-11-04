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

namespace CodeBuilderPrivate
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
			return PrepareType( t, names_scope, function_context );
		},
		type_name);
}

Type CodeBuilder::PrepareType( const Synt::EmptyVariant&, NamesScope&, FunctionContext& )
{
	U_ASSERT(false);
	return invalid_type_;
}

Type CodeBuilder::PrepareType( const Synt::ArrayTypeName& array_type_name, NamesScope& names_scope, FunctionContext& function_context )
{
	Array array_type;
	array_type.type= PrepareType( *array_type_name.element_type, names_scope, function_context );

	const Synt::Expression& num= *array_type_name.size;
	const FilePos num_file_pos= Synt::GetExpressionFilePos( num );

	const Variable size_variable= BuildExpressionCodeEnsureVariable( num, names_scope, function_context );
	if( size_variable.constexpr_value != nullptr )
	{
		if( const FundamentalType* const size_fundamental_type= size_variable.type.GetFundamentalType() )
		{
			if( IsInteger( size_fundamental_type->fundamental_type ) )
			{
				if( llvm::dyn_cast<llvm::UndefValue>(size_variable.constexpr_value) != nullptr )
					array_type.size= 1u;
				else
				{
					const llvm::APInt& size_value= size_variable.constexpr_value->getUniqueInteger();
					if( IsSignedInteger( size_fundamental_type->fundamental_type ) && size_value.isNegative() )
						REPORT_ERROR( ArraySizeIsNegative, names_scope.GetErrors(), num_file_pos );
					else
						array_type.size= size_value.getLimitedValue();
				}
			}
			else
				REPORT_ERROR( ArraySizeIsNotInteger, names_scope.GetErrors(), num_file_pos );
		}
		else
			U_ASSERT( false && "Nonfundamental constexpr? WTF?" );
	}
	else
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), num_file_pos );

	// TODO - generate error, if total size of type (incuding arrays) is more, than half of address space of target architecture.
	array_type.llvm_type= llvm::ArrayType::get( array_type.type.GetLLVMType(), array_type.size );
	return std::move(array_type);
}

Type CodeBuilder::PrepareType( const Synt::TypeofTypeName& typeof_type_name, NamesScope& names_scope, FunctionContext& function_context )
{
	Type result;
	const auto prev_state= SaveInstructionsState( function_context );
	{
		const StackVariablesStorage dummy_stack_variables_storage( function_context );
		const Variable variable= BuildExpressionCodeEnsureVariable( *typeof_type_name.expression, names_scope, function_context );
		result= std::move(variable.type);
	}
	RestoreInstructionsState( function_context, prev_state );
	return result;
}

Type CodeBuilder::PrepareType( const Synt::FunctionTypePtr& function_type_name_ptr, NamesScope& names_scope, FunctionContext& function_context )
{
	const Synt::FunctionType& function_type_name= *function_type_name_ptr;
	FunctionPointer function_pointer_type;
	Function& function_type= function_pointer_type.function;

	if( function_type_name.return_type_ == nullptr )
		function_type.return_type= void_type_for_ret_;
	else
		function_type.return_type= PrepareType( *function_type_name.return_type_, names_scope, function_context );
	function_type.return_value_is_mutable= function_type_name.return_value_mutability_modifier_ == MutabilityModifier::Mutable;
	function_type.return_value_is_reference= function_type_name.return_value_reference_modifier_ == ReferenceModifier::Reference;

	if( !function_type.return_value_is_reference &&
		!( function_type.return_type.GetFundamentalType() != nullptr ||
		   function_type.return_type.GetClassType() != nullptr ||
		   function_type.return_type.GetTupleType() != nullptr ||
		   function_type.return_type.GetEnumType() != nullptr ||
		   function_type.return_type.GetFunctionPointerType() != nullptr ) )
		REPORT_ERROR( NotImplemented, names_scope.GetErrors(), function_type_name.file_pos_, "return value types except fundamentals, enums, classes, function pointers" );

	for( const Synt::FunctionArgument& arg : function_type_name.arguments_ )
	{
		if( IsKeyword( arg.name_ ) )
			REPORT_ERROR( UsingKeywordAsName, names_scope.GetErrors(), arg.file_pos_ );

		function_type.args.emplace_back();
		Function::Arg& out_arg= function_type.args.back();
		out_arg.type= PrepareType( arg.type_, names_scope, function_context );

		out_arg.is_mutable= arg.mutability_modifier_ == MutabilityModifier::Mutable;
		out_arg.is_reference= arg.reference_modifier_ == ReferenceModifier::Reference;

		if( !out_arg.is_reference &&
			!( out_arg.type.GetFundamentalType() != nullptr ||
			   out_arg.type.GetClassType() != nullptr ||
			   out_arg.type.GetTupleType() != nullptr ||
			   out_arg.type.GetEnumType() != nullptr ||
			   out_arg.type.GetFunctionPointerType() != nullptr ) )
			REPORT_ERROR( NotImplemented, names_scope.GetErrors(), arg.file_pos_, "parameters types except fundamentals, classes, enums, functionpointers" );

		ProcessFunctionArgReferencesTags( names_scope.GetErrors(), function_type_name, function_type, arg, out_arg, function_type.args.size() - 1u );
	}

	function_type.unsafe= function_type_name.unsafe_;

	TryGenerateFunctionReturnReferencesMapping( names_scope.GetErrors(), function_type_name, function_type );
	ProcessFunctionTypeReferencesPollution( names_scope.GetErrors(), function_type_name, function_type );

	function_type.llvm_function_type= GetLLVMFunctionType( function_type );
	function_pointer_type.llvm_function_pointer_type= function_type.llvm_function_type->getPointerTo();
	return std::move(function_pointer_type);
}

Type CodeBuilder::PrepareType( const Synt::TupleType& tuple_type_name, NamesScope& names_scope, FunctionContext& function_context )
{
	Tuple tuple;
	tuple.elements.reserve( tuple_type_name.element_types_.size() );

	std::vector<llvm::Type*> elements_llvm_types;
	elements_llvm_types.reserve( tuple_type_name.element_types_.size() );

	for( const Synt::TypeName& element_type_name : tuple_type_name.element_types_ )
	{
		Type element_type= PrepareType( element_type_name, names_scope, function_context );
		if( element_type == invalid_type_ )
			return invalid_type_;

		elements_llvm_types.push_back( element_type.GetLLVMType() );
		tuple.elements.push_back( std::move(element_type) );
	}

	tuple.llvm_type= llvm::StructType::get( llvm_context_, elements_llvm_types );

	return std::move(tuple);
}

Type CodeBuilder::PrepareType( const Synt::NamedTypeName& named_type_name, NamesScope& names_scope, FunctionContext& function_context )
{
	const Value value= ResolveValue( named_type_name.file_pos_, names_scope, function_context, named_type_name.name );
	if( const Type* const type= value.GetTypeName() )
		return *type;
	else
		REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), named_type_name.file_pos_, named_type_name.name );

	return invalid_type_;
}

llvm::FunctionType* CodeBuilder::GetLLVMFunctionType( const Function& function_type )
{
	ArgsVector<llvm::Type*> args_llvm_types;

	bool first_arg_is_sret= false;
	if( !function_type.return_value_is_reference )
	{
		if( function_type.return_type.GetFundamentalType() != nullptr ||
			function_type.return_type.GetEnumType() != nullptr ||
			function_type.return_type.GetFunctionPointerType() != nullptr )
		{}
		else if( function_type.return_type.GetClassType() != nullptr || function_type.return_type.GetTupleType() != nullptr )
		{
			// Add return-value ponter as "sret" argument for class and tuple types.
			args_llvm_types.push_back( function_type.return_type.GetLLVMType()->getPointerTo() );
			first_arg_is_sret= true;
		}
		else U_ASSERT( false );
	}

	for( const Function::Arg& arg : function_type.args )
	{
		llvm::Type* type= arg.type.GetLLVMType();
		if( arg.is_reference )
			type= type->getPointerTo();
		else
		{
			if( arg.type.GetFundamentalType() != nullptr || arg.type.GetEnumType() != nullptr || arg.type.GetFunctionPointerType() )
			{}
			else if( arg.type.GetClassType() != nullptr || arg.type.GetTupleType() )
			{
				// Mark value-parameters of class and tuple types as pointer.
				type= type->getPointerTo();
			}
			else U_ASSERT( false );
		}
		args_llvm_types.push_back( type );
	}

	llvm::Type* llvm_function_return_type= nullptr;
	if( first_arg_is_sret )
		llvm_function_return_type= fundamental_llvm_types_.void_for_ret;
	else
	{
		llvm_function_return_type= function_type.return_type.GetLLVMType();
		if( function_type.return_value_is_reference )
			llvm_function_return_type= llvm_function_return_type->getPointerTo();
	}

	return llvm::FunctionType::get( llvm_function_return_type, args_llvm_types, false );
}


} // namespace CodeBuilderPrivate

} // namespace U
