#include "assert.hpp"
#include "keywords.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void CodeBuilder::NamesScopeBuild( NamesScope& names_scope )
{
	names_scope.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& name_imut )
		{
			NamesScope::InsertedName& name= const_cast<NamesScope::InsertedName&>(name_imut); // TODO - remove const_cast

			if( const NamesScopePtr inner_namespace= name.second.GetNamespace() )
				NamesScopeBuild( *inner_namespace );
			else if( OverloadedFunctionsSet* const functions_set= name.second.GetFunctionsSet() )
				NamesScopeBuildFunctionsSet( names_scope, *functions_set, true );
			else if( const Type* const type= name.second.GetTypeName() )
			{
				if( type->GetFundamentalType() != nullptr )
				{}
				else U_ASSERT(false);
			}
			else U_ASSERT(false);
		});
}

void CodeBuilder::NamesScopeBuildFunctionsSet( NamesScope& names_scope, OverloadedFunctionsSet& functions_set, const bool build_body )
{
	const ClassProxyPtr base_class= nullptr; // TODO

	if( functions_set.is_incomplete )
	{
		for( const Synt::Function* const function : functions_set.syntax_elements )
			NamesScopeBuildFunction( names_scope, base_class, functions_set, *function );
		functions_set.is_incomplete= false;
	}

	if( build_body )
	{
		for( FunctionVariable& function_variable : functions_set.functions )
		{
			if( function_variable.syntax_element->block_ != nullptr )
			{
				BuildFuncCode(
					function_variable,
					base_class,
					names_scope,
					function_variable.syntax_element->name_.components.back().name,
					function_variable.syntax_element->type_.arguments_,
					function_variable.syntax_element->block_.get(),
					function_variable.syntax_element->constructor_initialization_list_.get() );
			}
		}
	}
}

void CodeBuilder::NamesScopeBuildFunction(
	NamesScope& names_scope,
	const ClassProxyPtr base_class,
	OverloadedFunctionsSet& functions_set,
	const Synt::Function& func )
{
	const ProgramString& func_name= func.name_.components.back().name;
	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	const bool is_special_method= is_constructor || is_destructor;

	FunctionVariable func_variable;
	func_variable.type= Function();
	Function& function_type= *func_variable.type.GetFunctionType();

	if( func.type_.return_type_ == nullptr )
		function_type.return_type= void_type_for_ret_;
	else
	{
		function_type.return_type= PrepareType( func.type_.return_type_, names_scope );
		if( function_type.return_type == invalid_type_ )
			return;
	}

	function_type.return_value_is_mutable= func.type_.return_value_mutability_modifier_ == MutabilityModifier::Mutable;
	function_type.return_value_is_reference= func.type_.return_value_reference_modifier_ == ReferenceModifier::Reference;

	// HACK. We have different llvm types for "void".
	// llvm::void used only for empty return value, for other purposes we use "i8" for Ü::void.
	if( !function_type.return_value_is_reference && function_type.return_type == void_type_ )
		function_type.return_type= void_type_for_ret_;

	if( !function_type.return_value_is_reference &&
		!( function_type.return_type.GetFundamentalType() != nullptr ||
		   function_type.return_type.GetClassType() != nullptr ||
		   function_type.return_type.GetEnumType() != nullptr ||
		   function_type.return_type.GetFunctionPointerType() != nullptr ) )
	{
		errors_.push_back( ReportNotImplemented( func.file_pos_, "return value types except fundamentals, enums, classes, function pointers" ) );
		return;
	}

	if( is_special_method && function_type.return_type != void_type_ )
		errors_.push_back( ReportConstructorAndDestructorMustReturnVoid( func.file_pos_ ) );

	ProcessFunctionReturnValueReferenceTags( func.type_, function_type );

	// Args.
	function_type.args.reserve( func.type_.arguments_.size() );

	// Generate "this" arg for constructors.
	if( is_special_method )
	{
		func_variable.is_this_call= true;

		function_type.args.emplace_back();
		Function::Arg& arg= function_type.args.back();
		arg.type= base_class;
		arg.is_reference= true;
		arg.is_mutable= true;
	}

	for( const Synt::FunctionArgumentPtr& arg : func.type_.arguments_ )
	{
		const bool is_this= arg == func.type_.arguments_.front() && arg->name_ == Keywords::this_;

		if( !is_this && IsKeyword( arg->name_ ) )
			errors_.push_back( ReportUsingKeywordAsName( arg->file_pos_ ) );

		if( is_this && is_destructor )
			errors_.push_back( ReportExplicitThisInDestructor( arg->file_pos_ ) );
		if( is_this && is_constructor )
		{
			// Explicit this for constructor.
			U_ASSERT( function_type.args.size() == 1u );
			ProcessFunctionArgReferencesTags( func.type_, function_type, *arg, function_type.args.back(), function_type.args.size() - 1u );
			continue;
		}

		function_type.args.emplace_back();
		Function::Arg& out_arg= function_type.args.back();

		if( is_this )
		{
			func_variable.is_this_call= true;
			if( base_class == nullptr )
			{
				errors_.push_back( ReportThisInNonclassFunction( func.file_pos_, func_name ) );
				return;
			}
			out_arg.type= base_class;
		}
		else
			out_arg.type= PrepareType( arg->type_, names_scope );

		out_arg.is_mutable= arg->mutability_modifier_ == MutabilityModifier::Mutable;
		out_arg.is_reference= is_this || arg->reference_modifier_ == ReferenceModifier::Reference;

		if( !out_arg.is_reference &&
			!( out_arg.type.GetFundamentalType() != nullptr ||
			   out_arg.type.GetClassType() != nullptr ||
			   out_arg.type.GetEnumType() != nullptr ||
			   out_arg.type.GetFunctionPointerType() != nullptr ) )
		{
			errors_.push_back( ReportNotImplemented( func.file_pos_, "parameters types except fundamentals, classes, enums, functionpointers" ) );
			return;
		}

		ProcessFunctionArgReferencesTags( func.type_, function_type, *arg, out_arg, function_type.args.size() - 1u );
	} // for arguments

	function_type.unsafe= func.type_.unsafe_;

	// TODO - process "constexpr"

	TryGenerateFunctionReturnReferencesMapping( func.type_, function_type );
	ProcessFunctionReferencesPollution( func, function_type, base_class );
	CheckOverloadedOperator( base_class, function_type, func.overloaded_operator_, func.file_pos_ );

	// TODO - check virtual

	// TODO - Check "=default" / "=delete".

	const bool overloading_ok= ApplyOverloadedFunction( functions_set, func_variable, func.file_pos_ );
	if( !overloading_ok )
		return;

	FunctionVariable& inserted_func_variable= functions_set.functions.back();
	inserted_func_variable.syntax_element= &func;

	// TODO - process visibility

	// TODO - set correct file_pos

	BuildFuncCode(
		inserted_func_variable,
		base_class,
		names_scope,
		func_name,
		func.type_.arguments_,
		nullptr,
		func.constructor_initialization_list_.get() );
}

} // namespace CodeBuilderPrivate

} // namespace U
