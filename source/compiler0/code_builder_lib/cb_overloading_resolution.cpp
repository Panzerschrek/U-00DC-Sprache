#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

namespace
{

const TemplateSignatureParam g_dummy_template_signature_param = TemplateSignatureParam::TypeParam();

// "Class" of function argument in terms of overloading.
enum class ArgOverloadingClass
{
	// Value-args (both mutable and immutable), immutable references.
	ImmutableReference,
	// Mutable references.
	MutalbeReference,
};

ArgOverloadingClass GetArgOverloadingClass( const FunctionType::Param& param )
{
	return param.value_type == ValueType::ReferenceMut ? ArgOverloadingClass::MutalbeReference : ArgOverloadingClass::ImmutableReference;
}

enum class ConversionsCompareResult
{
	Same,
	LeftIsBetter,
	RightIsBetter,
	Incomparable
};

ConversionsCompareResult CompareConversionsTypes(
	const Type& src,
	const Type& dst_left,
	const Type& dst_right )
{
	if( dst_left == dst_right )
		return ConversionsCompareResult::Same;
	else if( src == dst_left )
		return ConversionsCompareResult::LeftIsBetter;
	else if( src == dst_right )
		return ConversionsCompareResult::RightIsBetter;

	if( src.ReferenceIsConvertibleTo( dst_left ) && src.ReferenceIsConvertibleTo( dst_right ) )
	{
		// SPRACHE_TODO - select more relevant compare function.

		//const Class&   src_class= *src.GetClassType();
		//const Class&  left_class= *src.GetClassType();
		//const Class& right_class= *src.GetClassType();

		if( dst_right.ReferenceIsConvertibleTo( dst_left  ) )
			return ConversionsCompareResult::RightIsBetter;
		if( dst_left .ReferenceIsConvertibleTo( dst_right ) )
			return ConversionsCompareResult::LeftIsBetter;

		return ConversionsCompareResult::Incomparable;
	}
	// Reference conversions are better, then type conversions.
	else if( src.ReferenceIsConvertibleTo( dst_left  ) )
		return ConversionsCompareResult::LeftIsBetter ;
	else if( src.ReferenceIsConvertibleTo( dst_right ) )
		return ConversionsCompareResult::RightIsBetter;
	// Two type conversions are incomparable.

	return ConversionsCompareResult::Incomparable;
}

ConversionsCompareResult CompareConversionsMutability(
	const FunctionType::Param& src,
	const FunctionType::Param& dst_left,
	const FunctionType::Param& dst_right)
{
	U_UNUSED(src);
	//const ArgOverloadingClass   src_overloding_class= GetArgOverloadingClass(src);
	const ArgOverloadingClass  left_overloding_class= GetArgOverloadingClass(dst_left );
	const ArgOverloadingClass right_overloding_class= GetArgOverloadingClass(dst_right);

	if( left_overloding_class == right_overloding_class )
		return ConversionsCompareResult::Same;
	if( left_overloding_class == ArgOverloadingClass::MutalbeReference )
		return ConversionsCompareResult::LeftIsBetter;
	if( right_overloding_class == ArgOverloadingClass::MutalbeReference )
		return ConversionsCompareResult::RightIsBetter;

	return ConversionsCompareResult::Incomparable;
}

ConversionsCompareResult TemplateSpecializationCompare(
	const TemplateSignatureParam& left_template_parameter,
	const TemplateSignatureParam& right_template_parameter )
{
	if( left_template_parameter.IsType() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::Same;
		else if( right_template_parameter.IsTemplateParam() )
			return ConversionsCompareResult::LeftIsBetter; // Concrete type is better, then template parameter.
		else if( right_template_parameter.GetArray() != nullptr )
			return ConversionsCompareResult::LeftIsBetter; // Type is more specialized, then array.
		if( right_template_parameter.GetTuple() != nullptr )
			return ConversionsCompareResult::LeftIsBetter; // Type is more specialized, then tuple.
		if( right_template_parameter.GetRawPointer() != nullptr )
			return ConversionsCompareResult::LeftIsBetter; // Type is more specialized, then raw pointer.
		if( right_template_parameter.GetFunction() != nullptr )
			return ConversionsCompareResult::LeftIsBetter; // Type is more specialized, then function.
		if( right_template_parameter.GetCoroutine() != nullptr )
			return ConversionsCompareResult::LeftIsBetter; // Type is more specialized, then coroutine.
		if( right_template_parameter.GetTemplate() != nullptr )
			return ConversionsCompareResult::LeftIsBetter; // Type is more specialized, then template.
		else U_ASSERT(false);
	}
	else if( left_template_parameter.IsVariable() )
	{
		if( right_template_parameter.IsVariable() )
			return ConversionsCompareResult::Same;
		else if( right_template_parameter.IsTemplateParam() )
			return ConversionsCompareResult::LeftIsBetter; // Value is more specialized, then template parameter.
		else U_ASSERT(false);
	}
	else if( const auto l_array= left_template_parameter.GetArray() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::RightIsBetter; // Type is more specialized, then array.
		else if( right_template_parameter.IsTemplateParam() )
			return ConversionsCompareResult::LeftIsBetter; // Array is more specialized, then template parameter.
		else if( const auto r_array= right_template_parameter.GetArray() )
		{
			const ConversionsCompareResult size_compare_result= TemplateSpecializationCompare( *l_array->element_count, *r_array->element_count );
			const ConversionsCompareResult type_compare_result= TemplateSpecializationCompare( *l_array->element_type, *r_array->element_type );
			if( size_compare_result == ConversionsCompareResult::Incomparable || type_compare_result == ConversionsCompareResult::Incomparable )
				return ConversionsCompareResult::Incomparable;
			if( size_compare_result == ConversionsCompareResult::Same )
				return type_compare_result;
			if( type_compare_result == ConversionsCompareResult::Same )
				return size_compare_result;
			if( type_compare_result == size_compare_result )
				return size_compare_result;
			return ConversionsCompareResult::Incomparable;
		}
		else U_ASSERT(false);
	}
	else if( const auto l_tuple= left_template_parameter.GetTuple() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::RightIsBetter; // Type is more specialized, then tuple.
		else if( right_template_parameter.IsTemplateParam() )
			return ConversionsCompareResult::LeftIsBetter; // Tuple is more specialized, then template parameter.
		else if( const auto r_tuple= right_template_parameter.GetTuple() )
		{
			// Tuples with different size is incomparable.
			if( l_tuple->element_types.size() != r_tuple->element_types.size() )
				return ConversionsCompareResult::Incomparable;

			ConversionsCompareResult result= ConversionsCompareResult::Same;
			for( size_t i= 0u; i < l_tuple->element_types.size(); ++i )
			{
				const ConversionsCompareResult arg_result= TemplateSpecializationCompare( l_tuple->element_types[i], r_tuple->element_types[i] );
				if( arg_result == ConversionsCompareResult::Incomparable )
					return ConversionsCompareResult::Incomparable;

				if( arg_result == ConversionsCompareResult::Same )
				{}
				else if( result == ConversionsCompareResult::Same )
					result= arg_result;
				else if( result != arg_result )
					return ConversionsCompareResult::Incomparable;
			}
			return result;
		}
		else U_ASSERT(false);
	}
	else if( const auto l_raw_pointer= left_template_parameter.GetRawPointer() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::RightIsBetter; // Type is more specialized, then raw pointer.
		else if( right_template_parameter.IsTemplateParam() )
			return ConversionsCompareResult::LeftIsBetter; // Raw pointer is more specialized, then template parameter.
		else if( const auto r_raw_pointer= right_template_parameter.GetRawPointer() )
			return TemplateSpecializationCompare( *l_raw_pointer->element_type, *r_raw_pointer->element_type );
		else U_ASSERT(false);
	}
	else if( const auto l_function= left_template_parameter.GetFunction() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::RightIsBetter; // Type is more specialized, then function.
		else if( right_template_parameter.IsTemplateParam() )
			return ConversionsCompareResult::LeftIsBetter; // Function is more specialized, then template parameter.
		else if( const auto r_function= right_template_parameter.GetFunction() )
		{
			if( l_function->params.size() != r_function->params.size() )
				return ConversionsCompareResult::Incomparable; // TODO - it is possible?

			ConversionsCompareResult result= TemplateSpecializationCompare( *l_function->return_type, *r_function->return_type );
			for( size_t i= 0u; i < l_function->params.size(); ++i )
			{
				const ConversionsCompareResult arg_result= TemplateSpecializationCompare( *l_function->params[i].type, *r_function->params[i].type );
				if( arg_result == ConversionsCompareResult::Incomparable )
					return ConversionsCompareResult::Incomparable;

				if( arg_result == ConversionsCompareResult::Same )
				{}
				else if( result == ConversionsCompareResult::Same )
					result= arg_result;
				else if( result != arg_result )
					return ConversionsCompareResult::Incomparable;
			}
			return result;
		}
		else U_ASSERT(false);
	}
	else if( const auto l_coroutine= left_template_parameter.GetCoroutine() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::RightIsBetter; // Type is more specialized, then coroutine.
		else if( right_template_parameter.IsTemplateParam() )
			return ConversionsCompareResult::LeftIsBetter; // Coroutine is more specialized, then template.
		else if( const auto r_coroutine= right_template_parameter.GetCoroutine() )
		{
			if( l_coroutine->kind != r_coroutine->kind ||
				l_coroutine->return_value_type != r_coroutine->return_value_type ||
				l_coroutine->return_references != r_coroutine->return_references ||
				l_coroutine->return_inner_references != r_coroutine->return_inner_references ||
				l_coroutine->inner_references != r_coroutine->inner_references )
				return ConversionsCompareResult::Incomparable;;

			return TemplateSpecializationCompare( *l_coroutine->return_type, *r_coroutine->return_type );
		}
		else U_ASSERT(false);
	}
	else if( const auto l_template= left_template_parameter.GetTemplate() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::RightIsBetter; // Type is more specialized, then template.
		else if( right_template_parameter.IsTemplateParam() )
			return ConversionsCompareResult::LeftIsBetter; // Template is more specialized, then template parameter.
		else if( const auto r_template= right_template_parameter.GetTemplate() )
		{
			// Templates with different arg count is uncomparable.
			if( l_template->params.size() != r_template->params.size() )
				return ConversionsCompareResult::Incomparable;

			ConversionsCompareResult result= ConversionsCompareResult::Same;
			for( size_t i= 0u; i < l_template->params.size(); ++i )
			{
				const ConversionsCompareResult arg_result= TemplateSpecializationCompare( l_template->params[i], r_template->params[i] );
				if( arg_result == ConversionsCompareResult::Incomparable )
					return ConversionsCompareResult::Incomparable;

				if( arg_result == ConversionsCompareResult::Same )
				{}
				else if( result == ConversionsCompareResult::Same )
					result= arg_result;
				else if( result != arg_result )
					return ConversionsCompareResult::Incomparable;
			}
			return result;
		}
		else U_ASSERT(false);
	}
	else if( left_template_parameter.IsTemplateParam() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::RightIsBetter;  // Concrete type is better, then template parameter.
		else if( right_template_parameter.IsVariable() )
			return ConversionsCompareResult::RightIsBetter; // Value is more specialized, then template parameter.
		else if( right_template_parameter.IsTemplateParam() )
			return ConversionsCompareResult::Same;
		else if( right_template_parameter.GetArray() != nullptr )
			return ConversionsCompareResult::RightIsBetter; // Array is more specialized, then template parameter.
		else if( right_template_parameter.GetTuple() != nullptr )
			return ConversionsCompareResult::RightIsBetter; // Tuple is more specialized, then template parameter.
		else if( right_template_parameter.GetRawPointer() != nullptr )
			return ConversionsCompareResult::RightIsBetter; // Raw pointer is more specialized, then template parameter.
		else if( right_template_parameter.GetFunction() != nullptr )
			return ConversionsCompareResult::RightIsBetter; // Function is more specialized, then template parameter.
		else if( right_template_parameter.GetTemplate() != nullptr )
			return ConversionsCompareResult::RightIsBetter; // Template is more specialized, then template parameter.
		else U_ASSERT(false);
	}
	else U_ASSERT(false);

	return ConversionsCompareResult::Incomparable;
}

ConversionsCompareResult CompareConversions(
	const FunctionType::Param& src,
	const FunctionType::Param& dst_left,
	const FunctionType::Param& dst_right)
{
	const ConversionsCompareResult types_compare= CompareConversionsTypes( src.type, dst_left.type, dst_right.type );
	const ConversionsCompareResult mutability_compare= CompareConversionsMutability( src, dst_left, dst_right );

	if( types_compare == mutability_compare )
		return types_compare;

	if( types_compare == ConversionsCompareResult::Incomparable || mutability_compare == ConversionsCompareResult::Incomparable )
		return ConversionsCompareResult::Incomparable;

	if( types_compare == ConversionsCompareResult::Same )
		return mutability_compare;
	if( mutability_compare == ConversionsCompareResult::Same )
		return types_compare;

	return ConversionsCompareResult::Incomparable;
}

ConversionsCompareResult CompareConversions(
	const FunctionType::Param& src,
	const FunctionType::Param& dst_left,
	const FunctionType::Param& dst_right,
	const TemplateSignatureParam& dst_left_template_parameter,
	const TemplateSignatureParam& dst_right_template_parameter )
{
	const ConversionsCompareResult conversions_compare= CompareConversions( src, dst_left, dst_right );

	if( conversions_compare == ConversionsCompareResult::Incomparable )
		return ConversionsCompareResult::Incomparable;
	if( conversions_compare == ConversionsCompareResult::LeftIsBetter ||
		conversions_compare == ConversionsCompareResult::RightIsBetter )
		return conversions_compare;
	// Compare template specializations, only if type conversions are not same.
	return TemplateSpecializationCompare( dst_left_template_parameter, dst_right_template_parameter );
}

} // namespace

FunctionVariable* CodeBuilder::GetFunctionWithSameType(
	const FunctionType& function_type,
	OverloadedFunctionsSet& functions_set )
{
	for( FunctionVariable& function_variable : functions_set.functions )
	{
		if( function_variable.type == function_type )
			return &function_variable;
	}

	return nullptr;
}

bool CodeBuilder::ApplyOverloadedFunction(
	OverloadedFunctionsSet& functions_set,
	const FunctionVariable& function,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	if( functions_set.functions.empty() )
	{
		functions_set.functions.push_back(function);
		return true;
	}

	const FunctionType& function_type= function.type;

	/*
	Algorithm for overloading applying:
	If parameter count differs - overload function.
	If "ArgOverloadingClass" of one or more arguments differs - overload function.
	*/
	for( const FunctionVariable& set_function : functions_set.functions )
	{
		const FunctionType& set_function_type= set_function.type;

		// If argument count differs - allow overloading.
		// SPRACHE_TODO - handle default arguments.
		if( function_type.params.size() != set_function_type.params.size() )
			continue;

		uint32_t param_is_same_count= 0u;
		for( size_t i= 0u; i < function_type.params.size(); i++ )
		{
			const FunctionType::Param& param= function_type.params[i];
			const FunctionType::Param& set_param= set_function_type.params[i];

			if( param.type != set_param.type )
				continue;

			if( GetArgOverloadingClass( param ) == GetArgOverloadingClass( set_param ) )
				param_is_same_count++;
		} // For args.

		if( param_is_same_count == function_type.params.size() )
		{
			REPORT_ERROR( CouldNotOverloadFunction, errors_container, src_loc );
			return false;
		}
	} // For functions in set.

	// No error - add function to set.
	functions_set.functions.push_back(function);
	return true;
}

FunctionType::Param CodeBuilder::OverloadingResolutionItemGetParamExtendedType( const OverloadingResolutionItem& item, const size_t param_index )
{
	if( const auto function_variable= std::get_if<const FunctionVariable*>( &item ) )
	{
		U_ASSERT( param_index < (*function_variable)->type.params.size() );
		return (*function_variable)->type.params[ param_index ];
	}
	else if( const auto template_function_preparation_result= std::get_if<TemplateFunctionPreparationResult>( &item ) )
	{
		const auto& params= template_function_preparation_result->function_template->syntax_element->function->type.params;
		U_ASSERT( param_index < params.size() );
		const Synt::FunctionParam& param= params[ param_index ];

		FunctionType::Param result;
		if( param.reference_modifier == ReferenceModifier::Reference )
			result.value_type= param.mutability_modifier == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;
		else
			result.value_type= ValueType::Value;

		if( param.name == Keyword( Keywords::this_ ) )
		{
			const auto base_class= template_function_preparation_result->function_template->base_class;
			if( base_class != nullptr )
				result.type= base_class;
			else
				result.type= invalid_type_; // May be in case of error.
		}
		else
		{
			result.type= PrepareType( param.type, *template_function_preparation_result->template_args_namespace, *global_function_context_ );
			global_function_context_->args_preevaluation_cache.clear();
		}

		return result;
	}
	else
	{
		U_ASSERT(false);
		return FunctionType::Param();
	}
}

const TemplateSignatureParam& CodeBuilder::OverloadingResolutionItemGetTemplateSignatureParam( const OverloadingResolutionItem& item, const size_t param_index )
{
	if( std::get_if<const FunctionVariable*>( &item ) != nullptr )
		return g_dummy_template_signature_param;
	else if( const auto template_function_preparation_result= std::get_if<TemplateFunctionPreparationResult>( &item ) )
	{
		const auto& params= template_function_preparation_result->function_template->signature_params;
		U_ASSERT( param_index < params.size() );
		return params[ param_index ];
	}
	else
	{
		U_ASSERT(false);
		return g_dummy_template_signature_param;
	}
}

bool CodeBuilder::OverloadingResolutionItemIsThisCall( const OverloadingResolutionItem& item )
{
	if( const auto function_variable= std::get_if<const FunctionVariable*>( &item ) )
	{
		return (*function_variable)->is_this_call;
	}
	else if( const auto template_function_preparation_result= std::get_if<TemplateFunctionPreparationResult>( &item ) )
	{
		const auto& params= template_function_preparation_result->function_template->syntax_element->function->type.params;
		return !params.empty() && params.front().name == Keyword( Keywords::this_ );
	}
	else
	{
		U_ASSERT(false);
		return false;
	}
}

bool CodeBuilder::OverloadingResolutionItemIsConversionConstructor( const OverloadingResolutionItem& item )
{
	if( const auto function_variable= std::get_if<const FunctionVariable*>( &item ) )
		return (*function_variable)->is_conversion_constructor;
	else if( const auto template_function_preparation_result= std::get_if<TemplateFunctionPreparationResult>( &item ) )
		return template_function_preparation_result->function_template->syntax_element->function->is_conversion_constructor;
	else
	{
		U_ASSERT(false);
		return false;
	}
}

const FunctionVariable* CodeBuilder::FinalizeSelectedFunction(
	const OverloadingResolutionItem& item,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	if( const auto function_variable= std::get_if<const FunctionVariable*>( &item ) )
		return *function_variable;
	else if( const auto template_function_preparation_result= std::get_if<TemplateFunctionPreparationResult>( &item ) )
		return FinishTemplateFunctionGeneration( errors_container, src_loc, *template_function_preparation_result );
	else
	{
		U_ASSERT(false);
		return nullptr;
	}
}

void CodeBuilder::FetchMatchedOverloadedFunctions(
	const OverloadedFunctionsSet& functions_set,
	const llvm::ArrayRef<FunctionType::Param> actual_args,
	const bool first_actual_arg_is_this,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const bool enable_type_conversions,
	llvm::SmallVectorImpl<OverloadingResolutionItem>& out_match_functions )
{
	// First, found functions, compatible with given arguments.
	for( const FunctionVariable& function : functions_set.functions )
	{
		const FunctionType& function_type= function.type;

		size_t actial_arg_count;
		const FunctionType::Param* actual_args_begin;
		if( first_actual_arg_is_this && !function.is_this_call )
		{
			// In case of static function call via "this" compare actual args without "this".
			actual_args_begin= actual_args.data() + 1u;
			actial_arg_count= actual_args.size() - 1u;
		}
		else
		{
			actual_args_begin= actual_args.data();
			actial_arg_count= actual_args.size();
		}

		// SPRACHE_TODO - handle functions with default arguments.
		if( function_type.params.size() != actial_arg_count )
			continue;

		bool all_args_are_compatible= true;
		for( uint32_t i= 0u; i < actial_arg_count; i++ )
		{
			const ArgOverloadingClass arg_overloading_class= GetArgOverloadingClass( actual_args_begin[i] );
			const ArgOverloadingClass parameter_overloading_class= GetArgOverloadingClass( function_type.params[i] );

			if( actual_args_begin[i].type == function_type.params[i].type )
			{} // ok
			else
			{
				// We needs complete types for checking possible conversions.
				// We can not just skip this function, if types are incomplete, because it will break "template instantiation equality rule".
				if( !( EnsureTypeComplete( function_type.params[i].type ) && EnsureTypeComplete( actual_args_begin[i].type ) ) )
				{
					all_args_are_compatible= false;
					break;
				}

				if( ReferenceIsConvertible( actual_args_begin[i].type, function_type.params[i].type, errors_container, src_loc ) )
				{}
				// Enable type conversion only if argument is not mutable reference.
				else if(
					enable_type_conversions && parameter_overloading_class == ArgOverloadingClass::ImmutableReference &&
					!function.is_conversion_constructor && !( function.is_constructor && IsCopyConstructor( function_type, function_type.params.front().type ) ) && // Disable convesin constructors call for copy constructors and conversion constructors
					HasConversionConstructor( actual_args_begin[i].type, function_type.params[i].type, errors_container, src_loc ) )
				{}
				else
				{
					all_args_are_compatible= false;
					break;
				}
			}

			if( arg_overloading_class == parameter_overloading_class )
			{} // All ok, exact match
			else if( parameter_overloading_class == ArgOverloadingClass::MutalbeReference &&
				arg_overloading_class != ArgOverloadingClass::MutalbeReference )
			{
				// We can only bind nonconst-reference arg to nonconst-reference parameter.
				all_args_are_compatible= false;
				break;
			}
			else if( parameter_overloading_class == ArgOverloadingClass::ImmutableReference &&
				arg_overloading_class == ArgOverloadingClass::MutalbeReference )
			{} // Ok, mut to imut conversion.
			else U_ASSERT(false);

		} // for candidate function args.

		if( all_args_are_compatible )
			out_match_functions.push_back( &function );

	} // for functions

	// Try select also template functions.
	for( const FunctionTemplatePtr& function_template_ptr : functions_set.template_functions )
	{
		// Only prepare template function.
		// Finalize (and trigger its build) later - only if this function is selected.
		TemplateFunctionPreparationResult result= PrepareTemplateFunction( errors_container, src_loc, function_template_ptr, actual_args, first_actual_arg_is_this );
		if( result.function_template != nullptr )
			out_match_functions.push_back( std::move(result) );
	}
}

const CodeBuilder::OverloadingResolutionItem* CodeBuilder::SelectOverloadedFunction(
	llvm::ArrayRef<FunctionType::Param> actual_args,
	const bool first_actual_arg_is_this,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc,
	const llvm::ArrayRef<OverloadingResolutionItem> matched_functions )
{
	if( matched_functions.size() == 1u )
		return &matched_functions.front();

	llvm::SmallVector<bool, 16> best_functions( matched_functions.size(), true );

	// For each argument search functions, which is better, than another functions.
	// For NOT better (four current arg) functions set flags to false.
	for( size_t arg_n= 0; arg_n < actual_args.size(); ++arg_n )
	{
		for( const OverloadingResolutionItem& function_l : matched_functions )
		{
			size_t l_arg_n= arg_n;
			if( first_actual_arg_is_this && !OverloadingResolutionItemIsThisCall(function_l) )
			{
				if( arg_n == 0 )
					continue;
				l_arg_n= arg_n - 1u;
			}

			bool is_best_function_for_current_arg= true;
			for( const OverloadingResolutionItem& function_r : matched_functions )
			{
				size_t r_arg_n= arg_n;
				if( first_actual_arg_is_this && !OverloadingResolutionItemIsThisCall(function_r) )
				{
					if( arg_n == 0 )
						continue;
					r_arg_n= arg_n - 1u;
				}

				const ConversionsCompareResult comp=
					CompareConversions(
						actual_args[arg_n],
						OverloadingResolutionItemGetParamExtendedType( function_l, l_arg_n ),
						OverloadingResolutionItemGetParamExtendedType( function_r, r_arg_n ),
						OverloadingResolutionItemGetTemplateSignatureParam( function_l, l_arg_n ),
						OverloadingResolutionItemGetTemplateSignatureParam( function_r, r_arg_n ) );

				if( comp == ConversionsCompareResult::Same || comp == ConversionsCompareResult::LeftIsBetter )
					continue;

				is_best_function_for_current_arg= false;
				break;

			} // for functions right

			// Set best functions bits.
			if( is_best_function_for_current_arg )
			{
				for( size_t func_n= 0u; func_n < matched_functions.size(); ++func_n )
				{
					const OverloadingResolutionItem& function_r= matched_functions[func_n];
					size_t r_arg_n= arg_n;
					if( first_actual_arg_is_this && !OverloadingResolutionItemIsThisCall(function_r) )
					{
						if( arg_n == 0 )
							continue;
						r_arg_n= arg_n - 1u;
					}

					const ConversionsCompareResult comp=
						CompareConversions(
							actual_args[arg_n],
							OverloadingResolutionItemGetParamExtendedType( function_l, l_arg_n ),
							OverloadingResolutionItemGetParamExtendedType( function_r, r_arg_n ),
							OverloadingResolutionItemGetTemplateSignatureParam( function_l, l_arg_n ),
							OverloadingResolutionItemGetTemplateSignatureParam( function_r, r_arg_n ) );

					U_ASSERT( comp != ConversionsCompareResult::Incomparable && comp != ConversionsCompareResult::RightIsBetter );

					if( comp != ConversionsCompareResult::Same )
						best_functions[func_n]= false;
				}
			}
		} // for functions left
	} // for args

	// For succsess resolution we must get just one function with flag=true.
	const OverloadingResolutionItem* selected_function= nullptr;
	for( size_t func_n= 0u; func_n < matched_functions.size(); ++func_n )
	{
		if( best_functions[func_n] )
		{
			if( selected_function == nullptr )
				selected_function= &matched_functions[func_n];
			else
			{
				selected_function= nullptr;
				break;
			}
		}
	}

	if( selected_function == nullptr )
		REPORT_ERROR( TooManySuitableOverloadedFunctions, errors_container, src_loc, FunctionParamsToString(actual_args) );

	return selected_function;
}

const FunctionVariable* CodeBuilder::GetOverloadedFunction(
	const OverloadedFunctionsSet& functions_set,
	const llvm::ArrayRef<FunctionType::Param> actual_args,
	const bool first_actual_arg_is_this,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	U_ASSERT( !( first_actual_arg_is_this && actual_args.empty() ) );

	llvm::SmallVector<OverloadingResolutionItem, 8> matched_functions;
	FetchMatchedOverloadedFunctions( functions_set, actual_args, first_actual_arg_is_this, errors_container, src_loc, true, matched_functions );
	if( matched_functions.empty() )
	{
		REPORT_ERROR( CouldNotSelectOverloadedFunction, errors_container, src_loc, FunctionParamsToString(actual_args) );
		return nullptr;
	}

	if( const auto item= SelectOverloadedFunction( actual_args, first_actual_arg_is_this, errors_container, src_loc, matched_functions ) )
		return FinalizeSelectedFunction( *item, errors_container, src_loc );

	return nullptr;
}

const FunctionVariable* CodeBuilder::GetOverloadedOperator(
	const llvm::ArrayRef<FunctionType::Param> actual_args,
	const OverloadedOperator op,
	NamesScope& names_scope,
	const SrcLoc& src_loc )
{
	const std::string_view op_name= OverloadedOperatorToString( op );

	llvm::SmallVector<OverloadingResolutionItem, 8> matched_functions;
	llvm::SmallVector<ClassPtr, 8> processed_classes;
	llvm::SmallVector< std::pair<ClassPtr, ClassMemberVisibility>, 8> classes_visibility;
	for( const FunctionType::Param& arg : actual_args )
	{
		if( (op == OverloadedOperator::Indexing || op == OverloadedOperator::Call) && &arg != &actual_args.front() )
			break; // For indexing and call operators only check first argument.

		if( Class* const class_= arg.type.GetClassType() )
		{
			if( !EnsureTypeComplete( arg.type ) )
			{
				REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), src_loc, arg.type );
				return nullptr;
			}

			// Process each class exactly once - in order to fetch only single functions set for each class, even if arguments have same class type.
			bool already_processed= false;
			for( const ClassPtr prev_class : processed_classes )
			{
				if( class_ == prev_class )
				{
					already_processed= true;
					break;
				}
			}
			if( already_processed )
				continue;
			processed_classes.push_back( class_ );

			const auto value_in_class= ResolveClassValue( class_, op_name );
			if( value_in_class.first == nullptr )
				continue;

			const OverloadedFunctionsSetPtr operators_set= value_in_class.first->value.GetFunctionsSet();
			U_ASSERT( operators_set != nullptr ); // If we found something in names map with operator name, it must be operator.
			PrepareFunctionsSetAndBuildConstexprBodies( *class_->members, *operators_set ); // Make sure functions set is complete.

			const size_t prev_size= matched_functions.size();
			FetchMatchedOverloadedFunctions( *operators_set, actual_args, false, names_scope.GetErrors(), src_loc, true, matched_functions );
			const size_t new_size= matched_functions.size();

			for( size_t i= prev_size; i < new_size; ++i )
				classes_visibility.push_back( std::make_pair( class_, value_in_class.second ) );
		}
	}

	U_ASSERT( classes_visibility.size() == matched_functions.size() );

	if( !matched_functions.empty() )
	{
		if( const auto item= SelectOverloadedFunction( actual_args, false, names_scope.GetErrors(), src_loc, matched_functions ) )
		{
			const FunctionVariable* const func= FinalizeSelectedFunction( *item, names_scope.GetErrors(), src_loc );
			if( func != nullptr )
			{
				// Check access rights after function selection.
				const auto& visibility= classes_visibility[ size_t( item - matched_functions.data() ) ];
				const ClassPtr class_= visibility.first;
				if( names_scope.GetAccessFor( class_ ) < visibility.second )
					REPORT_ERROR( AccessingNonpublicClassMember, names_scope.GetErrors(), src_loc, op_name, class_->members->GetThisNamespaceName() );
				return func;
			}
		}
	}

	return nullptr;
}

OverloadedFunctionsSetPtr CodeBuilder::GetConstructors(
	const Type& type,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	if( !EnsureTypeComplete( type ) )
	{
		REPORT_ERROR( UsingIncompleteType, errors_container, src_loc, type );
		return nullptr;
	}
	const Class* const class_type= type.GetClassType();
	if( class_type == nullptr )
		return nullptr;

	const NamesScopeValue* const constructors_value= class_type->members->GetThisScopeValue( Keyword( Keywords::constructor_ ) );
	if( constructors_value == nullptr )
		return nullptr;

	auto constructors= constructors_value->value.GetFunctionsSet();
	if( constructors == nullptr )
		return nullptr;

	PrepareFunctionsSetAndBuildConstexprBodies( *class_type->members, *constructors );
	return constructors;
}

const FunctionVariable* CodeBuilder::GetConversionConstructor(
	const Type& src_type,
	const Type& dst_type,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	const OverloadedFunctionsSetPtr constructors= GetConstructors( dst_type, errors_container, src_loc );
	if( constructors == nullptr )
		return nullptr;

	// TODO - what if conversion constructor is private?

	FunctionType::Param actual_args[2];
	actual_args[0u].type= dst_type;
	actual_args[0u].value_type= ValueType::ReferenceMut;
	actual_args[1u].type= src_type;
	actual_args[1u].value_type= ValueType::ReferenceImut;

	llvm::SmallVector<OverloadingResolutionItem, 8> matched_functions;
	FetchMatchedOverloadedFunctions( *constructors, actual_args, false, errors_container, src_loc, false, matched_functions );

	if( !matched_functions.empty() )
	{
		if( const auto item= SelectOverloadedFunction( actual_args, true, errors_container, src_loc, matched_functions ) )
		{
			const auto func= FinalizeSelectedFunction( *item, errors_container, src_loc );
			if( func != nullptr && func->is_conversion_constructor )
				return func;
		}
	}

	return nullptr;
}

bool CodeBuilder::HasConversionConstructor(
	const Type& src_type,
	const Type& dst_type,
	CodeBuilderErrorsContainer& errors_container,
	const SrcLoc& src_loc )
{
	const OverloadedFunctionsSetPtr constructors= GetConstructors( dst_type, errors_container, src_loc );
	if( constructors == nullptr )
		return false;

	FunctionType::Param actual_args[2];
	actual_args[0u].type= dst_type;
	actual_args[0u].value_type= ValueType::ReferenceMut;
	actual_args[1u].type= src_type;
	actual_args[1u].value_type= ValueType::ReferenceImut;

	// Perform fetch of conversions constructors, but do not trigger its building.
	// This is needed, since check for conversion constructor existence may be requested without further conversion constructor usage.
	llvm::SmallVector<OverloadingResolutionItem, 8> matched_functions;
	FetchMatchedOverloadedFunctions( *constructors, actual_args, false, errors_container, src_loc, false, matched_functions );

	for( const OverloadingResolutionItem& item : matched_functions )
		if( OverloadingResolutionItemIsConversionConstructor( item ) )
			return true;

	return false;
}

const CodeBuilder::TemplateTypePreparationResult* CodeBuilder::SelectTemplateType(
	const llvm::ArrayRef<TemplateTypePreparationResult> candidate_templates,
	const size_t arg_count )
{
	if( candidate_templates.empty() )
		return nullptr;

	if( candidate_templates.size() == 1u )
		return &candidate_templates.front();

	llvm::SmallVector<bool, 16> best_templates( candidate_templates.size(), true );

	for( size_t i= 0u; i < arg_count; ++i )
	{
		for( const TemplateTypePreparationResult& candidate_l : candidate_templates )
		{
			bool is_best_template_for_current_arg= true;
			for( const TemplateTypePreparationResult& candidate_r : candidate_templates )
			{
				const ConversionsCompareResult comp=
					TemplateSpecializationCompare( candidate_l.type_template->signature_params[i], candidate_r.type_template->signature_params[i] );

				if( comp == ConversionsCompareResult::Incomparable || comp == ConversionsCompareResult::RightIsBetter )
				{
					is_best_template_for_current_arg= false;
					break;
				}
			}

			if( is_best_template_for_current_arg )
			{
				for( const TemplateTypePreparationResult& candidate_r : candidate_templates )
				{
					const ConversionsCompareResult comp=
						TemplateSpecializationCompare( candidate_l.type_template->signature_params[i], candidate_r.type_template->signature_params[i] );

					if( comp != ConversionsCompareResult::Same )
						best_templates[ size_t(&candidate_r - candidate_templates.data()) ]= false;
				}
			}
		}
	}

	const TemplateTypePreparationResult* selected_template= nullptr;
	for( size_t template_n= 0u; template_n < candidate_templates.size(); ++ template_n )
	{
		if( best_templates[template_n] )
		{
			if( selected_template == nullptr )
				selected_template= &candidate_templates[template_n];
			else
				return nullptr;
		}
	}

	return selected_template;
}

} // namespace U
