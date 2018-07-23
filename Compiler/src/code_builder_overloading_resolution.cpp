#include "assert.hpp"
#include "keywords.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

enum class ConversionsCompareResult
{
	Same,
	LeftIsBetter,
	RightIsBetter,
	Incomparable
};

static ConversionsCompareResult CompareConversionsTypes(
	const Type& src,
	const Type& dst_left,
	const Type& dst_right)
{
	U_ASSERT( src.ReferenceIsConvertibleTo( dst_left  ) );
	U_ASSERT( src.ReferenceIsConvertibleTo( dst_right ) );

	if( dst_left == dst_right )
		return ConversionsCompareResult::Same;

	const Type void_type( FundamentalType( U_FundamentalType::Void, nullptr ) );

	// If one of types are void - other is better (because it is not void ).
	if( dst_left  == void_type )
	{
		U_ASSERT( dst_right != void_type );
		return ConversionsCompareResult::RightIsBetter;
	}
	if( dst_right == void_type )
	{
		U_ASSERT( dst_left  != void_type );
		return ConversionsCompareResult::LeftIsBetter;
	}

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

static ConversionsCompareResult CompareConversionsMutability(
	const Function::Arg& src,
	const Function::Arg& dst_left,
	const Function::Arg& dst_right)
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

static ConversionsCompareResult TemplateSpecializationCompare(
	const DeducedTemplateParameter& left_template_parameter,
	const DeducedTemplateParameter& right_template_parameter )
{
	U_ASSERT( ! left_template_parameter.IsInvalid() );
	U_ASSERT( !right_template_parameter.IsInvalid() );

	if( left_template_parameter.IsType() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::Same;
		else if( right_template_parameter.IsTemplateParameter() )
			return ConversionsCompareResult::LeftIsBetter; // Concrete type is better, then template parameter.
		else if( right_template_parameter.GetArray() != nullptr )
			return ConversionsCompareResult::RightIsBetter; // Array is more specialized, then type.
		if( right_template_parameter.GetFunction() != nullptr )
			return ConversionsCompareResult::RightIsBetter; // Function is more specialized, then type.
		if( right_template_parameter.GetTemplate() != nullptr )
			return ConversionsCompareResult::RightIsBetter; // Template is more specialized, then type.
		else U_ASSERT(false);
	}
	else if( left_template_parameter.IsVariable() )
	{
		if( right_template_parameter.IsVariable() )
			return ConversionsCompareResult::Same;
		else if( right_template_parameter.IsTemplateParameter() )
			return ConversionsCompareResult::LeftIsBetter; // Value is more specialized, then template parameter.
		else U_ASSERT(false);
	}
	else if( const auto l_array= left_template_parameter.GetArray() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::LeftIsBetter; // Array is more specialized, then type.
		else if( right_template_parameter.IsTemplateParameter() )
			return ConversionsCompareResult::LeftIsBetter; // Array is more specialized, then template parameter.
		else if( const auto r_array= right_template_parameter.GetArray() )
		{
			const ConversionsCompareResult size_compare_result= TemplateSpecializationCompare( *l_array->size, *r_array->size );
			const ConversionsCompareResult type_compare_result= TemplateSpecializationCompare( *l_array->type, *r_array->type );
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
	else if( const auto l_function= left_template_parameter.GetFunction() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::LeftIsBetter; // Function is more specialized, then type.
		else if( right_template_parameter.IsTemplateParameter() )
			return ConversionsCompareResult::LeftIsBetter; // Function is more specialized, then template parameter.
		else if( const auto r_function= right_template_parameter.GetFunction() )
		{
			if( l_function->argument_types.size() != r_function->argument_types.size() )
				return ConversionsCompareResult::Incomparable; // TODO - it is possible?

			ConversionsCompareResult result= TemplateSpecializationCompare( *l_function->return_type, *r_function->return_type );
			for( size_t i= 0u; i < l_function->argument_types.size(); ++i )
			{
				const ConversionsCompareResult arg_result= TemplateSpecializationCompare( l_function->argument_types[i], r_function->argument_types[i] );
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
	else if( const auto l_template= left_template_parameter.GetTemplate() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::LeftIsBetter; // Template is more specialized, then type.
		else if( right_template_parameter.IsTemplateParameter() )
			return ConversionsCompareResult::LeftIsBetter; // Template is more specialized, then template parameter.
		else if( const auto r_template= right_template_parameter.GetTemplate() )
		{
			// Templates with different arg count is uncomparable.
			if( l_template->args.size() != r_template->args.size() )
				return ConversionsCompareResult::Incomparable;

			ConversionsCompareResult result= ConversionsCompareResult::Same;
			for( size_t i= 0u; i < l_template->args.size(); ++i )
			{
				const ConversionsCompareResult arg_result= TemplateSpecializationCompare( l_template->args[i], r_template->args[i] );
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
	else if( left_template_parameter.IsTemplateParameter() )
	{
		if( right_template_parameter.IsType() )
			return ConversionsCompareResult::RightIsBetter;  // Concrete type is better, then template parameter.
		else if( right_template_parameter.IsVariable() )
			return ConversionsCompareResult::RightIsBetter; // Value is more specialized, then template parameter.
		else if( right_template_parameter.IsTemplateParameter() )
			return ConversionsCompareResult::Same;
		else if( right_template_parameter.GetArray() != nullptr )
			return ConversionsCompareResult::RightIsBetter; // Array is more specialized, then template parameter.
		else if( right_template_parameter.GetFunction() != nullptr )
			return ConversionsCompareResult::RightIsBetter; // Function is more specialized, then template parameter.
		else if( right_template_parameter.GetTemplate() != nullptr )
			return ConversionsCompareResult::RightIsBetter; // Template is more specialized, then template parameter.
		else U_ASSERT(false);
	}
	else U_ASSERT(false);

	return ConversionsCompareResult::Incomparable;
}

static ConversionsCompareResult CompareConversions(
	const Function::Arg& src,
	const Function::Arg& dst_left,
	const Function::Arg& dst_right)
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

static ConversionsCompareResult CompareConversions(
	const Function::Arg& src,
	const Function::Arg& dst_left,
	const Function::Arg& dst_right,
	const DeducedTemplateParameter& dst_left_template_parameter,
	const DeducedTemplateParameter& dst_right_template_parameter )
{
	const ConversionsCompareResult conversions_compare= CompareConversions( src, dst_left, dst_right );
	const ConversionsCompareResult template_specialization_compare= TemplateSpecializationCompare( dst_left_template_parameter, dst_right_template_parameter );

	if( conversions_compare == template_specialization_compare )
		return conversions_compare;

	if( conversions_compare == ConversionsCompareResult::Incomparable || template_specialization_compare == ConversionsCompareResult::Incomparable )
		return ConversionsCompareResult::Incomparable;

	if( conversions_compare == ConversionsCompareResult::Same )
		return template_specialization_compare;
	if( template_specialization_compare == ConversionsCompareResult::Same )
		return conversions_compare;

	return ConversionsCompareResult::Incomparable;
}

FunctionVariable* CodeBuilder::GetFunctionWithSameType(
	const Function& function_type,
	OverloadedFunctionsSet& functions_set )
{
	for( FunctionVariable& function_varaible : functions_set.functions )
	{
		if( *function_varaible.type.GetFunctionType() == function_type )
			return &function_varaible;
	}

	return nullptr;
}

bool CodeBuilder::ApplyOverloadedFunction(
	OverloadedFunctionsSet& functions_set,
	const FunctionVariable& function,
	const FilePos& file_pos )
{
	if( functions_set.functions.empty() )
	{
		functions_set.functions.push_back(function);
		return true;
	}

	const Function* function_type= function.type.GetFunctionType();
	U_ASSERT(function_type);

	/*
	Algorithm for overloading applying:
	If parameter count differs - overload function.
	If "ArgOverloadingClass" of one or more arguments differs - overload function.
	*/
	for( const FunctionVariable& set_function : functions_set.functions )
	{
		const Function& set_function_type= *set_function.type.GetFunctionType(); // Must be function type 100 %

		// If argument count differs - allow overloading.
		// SPRACHE_TODO - handle default arguments.
		if( function_type->args.size() != set_function_type.args.size() )
			continue;

		unsigned int arg_is_same_count= 0u;
		for( size_t i= 0u; i < function_type->args.size(); i++ )
		{
			const Function::Arg& arg= function_type->args[i];
			const Function::Arg& set_arg= set_function_type.args[i];

			if( arg.type != set_arg.type )
				continue;

			if( GetArgOverloadingClass( arg ) == GetArgOverloadingClass( set_arg ) )
				arg_is_same_count++;
		} // For args.

		if( arg_is_same_count == function_type->args.size() )
		{
			errors_.push_back( ReportCouldNotOverloadFunction(file_pos) );
			return false;
		}
	} // For functions in set.

	// No error - add function to set.
	functions_set.functions.push_back(function);
	return true;
}

const FunctionVariable* CodeBuilder::GetOverloadedFunction(
	const OverloadedFunctionsSet& functions_set,
	const std::vector<Function::Arg>& actual_args,
	const bool first_actual_arg_is_this,
	const FilePos& file_pos )
{
	U_ASSERT( !functions_set.functions.empty() || ! functions_set.template_functions.empty() );
	U_ASSERT( !( first_actual_arg_is_this && actual_args.empty() ) );

	std::vector<const FunctionVariable*> match_functions;

	// First, found functions, compatible with given arguments.
	for( const FunctionVariable& function : functions_set.functions )
	{
		const Function& function_type= *function.type.GetFunctionType();

		size_t actial_arg_count;
		const Function::Arg* actual_args_begin;
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
		if( function_type.args.size() != actial_arg_count )
			continue;

		bool all_args_is_compatible= true;
		for( unsigned int i= 0u; i < actial_arg_count; i++ )
		{
			const bool types_are_same= actual_args_begin[i].type == function_type.args[i].type;
			if( !types_are_same )
			{
				// We needs complete types for checking possible conversions.
				// We can not just skip this function, if types are incomplete, because it will break "template instantiation equality rule".
				if( function_type.args[i].type != void_type_ && actual_args_begin[i].type != void_type_ &&
					( actual_args_begin[i].type.IsIncomplete() || function_type.args[i].type.IsIncomplete() ) )
				{
					errors_.push_back( ReportCouldNotSelectOverloadedFunction( file_pos ) );
					all_args_is_compatible= false;
					break;
				};
			}

			const bool types_are_compatible= actual_args_begin[i].type.ReferenceIsConvertibleTo( function_type.args[i].type );
			// SPRACHE_TODO - support type-casting for function call.
			if( !types_are_compatible )
			{
				all_args_is_compatible= false;
				break;
			}

			const ArgOverloadingClass arg_overloading_class= GetArgOverloadingClass( actual_args_begin[i] );
			const ArgOverloadingClass parameter_overloading_class= GetArgOverloadingClass( function_type.args[i] );
			if( arg_overloading_class == parameter_overloading_class )
			{} // All ok, exact match
			else if( parameter_overloading_class == ArgOverloadingClass::MutalbeReference &&
				arg_overloading_class != ArgOverloadingClass::MutalbeReference )
			{
				// We can only bind nonconst-reference arg to nonconst-reference parameter.
				all_args_is_compatible= false;
				break;
			}
			else if( parameter_overloading_class == ArgOverloadingClass::ImmutableReference &&
				arg_overloading_class == ArgOverloadingClass::MutalbeReference )
			{} // Ok, mut to imut conversion.
			else
				U_ASSERT(false);

		} // for candidate function args.


		if( all_args_is_compatible )
			match_functions.push_back( &function );

	} // for functions

	if( match_functions.empty() )
	{
		// Not found any function - try select template function.
		for( const FunctionTemplatePtr& function_template_ptr : functions_set.template_functions )
		{
			const FunctionVariable* const generated_function=
				GenTemplateFunction( file_pos, function_template_ptr, *function_template_ptr->parent_namespace, actual_args, first_actual_arg_is_this );
			if( generated_function != nullptr )
				match_functions.push_back( generated_function );
		}
	}

	if( match_functions.empty() )
	{
		errors_.push_back( ReportCouldNotSelectOverloadedFunction( file_pos ) );
		return nullptr;
	}
	else if( match_functions.size() == 1u )
		return match_functions.front();

	std::vector<bool> best_functions( match_functions.size(), true );

	// For each argument search functions, which is better, than another functions.
	// For NOT better (four current arg) functions set flags to false.
	for( size_t arg_n= 0; arg_n < actual_args.size(); ++arg_n )
	{
		for( const FunctionVariable* const function_l : match_functions )
		{
			size_t l_arg_n= arg_n;
			if( first_actual_arg_is_this && !function_l->is_this_call )
			{
				if( arg_n == 0 )
					continue;
				l_arg_n= arg_n - 1u;
			}

			const Function& l_type=* function_l->type.GetFunctionType();

			bool is_best_function_for_current_arg= true;
			for( const FunctionVariable* const function_r : match_functions )
			{
				size_t r_arg_n= arg_n;
				if( first_actual_arg_is_this && !function_r->is_this_call )
				{
					if( arg_n == 0 )
						continue;
					r_arg_n= arg_n - 1u;
				}

				const Function& r_type=* function_r->type.GetFunctionType();

				ConversionsCompareResult comp;
					if( !function_r->deduced_temlpate_parameters.empty() )
						comp=
							CompareConversions(
								actual_args[arg_n],
								l_type.args[l_arg_n], r_type.args[r_arg_n],
								function_l->deduced_temlpate_parameters[l_arg_n], function_r->deduced_temlpate_parameters[r_arg_n] );
					else
						comp=
							CompareConversions(
								actual_args[arg_n],
								l_type.args[l_arg_n], r_type.args[r_arg_n] );

				if( comp == ConversionsCompareResult::Same || comp == ConversionsCompareResult::LeftIsBetter )
					continue;

				is_best_function_for_current_arg= false;
				break;

			} // for functions right

			// Set best functions bits.
			if( is_best_function_for_current_arg )
			{
				for( size_t func_n= 0u; func_n < match_functions.size(); ++func_n )
				{
					const FunctionVariable function_r= *match_functions[func_n];
					size_t r_arg_n= arg_n;
					if( first_actual_arg_is_this && !function_r.is_this_call )
					{
						if( arg_n == 0 )
							continue;
						r_arg_n= arg_n - 1u;
					}

					const Function& r_type=* function_r.type.GetFunctionType();
					ConversionsCompareResult comp;
					if( !function_r.deduced_temlpate_parameters.empty() )
						comp=
							CompareConversions(
								actual_args[arg_n],
								l_type.args[l_arg_n], r_type.args[r_arg_n],
								function_l->deduced_temlpate_parameters[l_arg_n], function_r.deduced_temlpate_parameters[r_arg_n] );
					else
						comp=
							CompareConversions(
								actual_args[arg_n],
								l_type.args[l_arg_n], r_type.args[r_arg_n] );

					U_ASSERT( comp != ConversionsCompareResult::Incomparable && comp != ConversionsCompareResult::RightIsBetter );

					if( comp != ConversionsCompareResult::Same )
						best_functions[func_n]= false;
				}
			}
		} // for functions left
	} // for args

	// For succsess resolution we must get just one function with flag=true.
	const FunctionVariable* selected_function= nullptr;
	for( size_t func_n= 0u; func_n < match_functions.size(); ++func_n )
	{
		if( best_functions[func_n] )
		{
			if( selected_function == nullptr )
				selected_function= match_functions[func_n];
			else
			{
				selected_function= nullptr;
				break;
			}
		}
	}

	if( selected_function == nullptr )
		errors_.push_back( ReportTooManySuitableOverloadedFunctions( file_pos ) );

	return selected_function;
}

const FunctionVariable* CodeBuilder::GetOverloadedOperator(
	const std::vector<Function::Arg>& actual_args,
	OverloadedOperator op,
	const FilePos& file_pos )
{
	const ProgramString op_name= OverloadedOperatorToString( op );

	const size_t errors_before= errors_.size();

	for( const Function::Arg& arg : actual_args )
	{
		if( op == OverloadedOperator::Indexing && &arg != &actual_args.front() )
			break; // For indexing operator only check first argument.

		if( const Class* const class_= arg.type.GetClassType() )
		{
			if( class_->completeness != TypeCompleteness::Complete )
			{
				errors_.push_back( ReportUsingIncompleteType( file_pos, arg.type.ToString() ) );
				return nullptr;
			}

			const NamesScope::InsertedName* const name_in_class= class_->members.GetThisScopeName( op_name );
			if( name_in_class == nullptr )
				continue;

			const OverloadedFunctionsSet* const operators_set= name_in_class->second.GetFunctionsSet();
			U_ASSERT( operators_set != nullptr ); // If we found something in names map with operator name, it must be operator.

			const FunctionVariable* const func= GetOverloadedFunction( *operators_set, actual_args, false, file_pos );
			if( func != nullptr )
			{
				errors_.resize( errors_before ); // Clear potential errors only in case of success.
				return func;
			}
		}
	}

	return nullptr;
}

const CodeBuilder::TemplateTypeGenerationResult* CodeBuilder::SelectTemplateType(
	const std::vector<TemplateTypeGenerationResult>& candidate_templates,
	const size_t arg_count )
{
	if( candidate_templates.empty() )
		return nullptr;

	if( candidate_templates.size() == 1u )
		return &candidate_templates.front();

	std::vector<bool> best_templates( candidate_templates.size(), true );

	for( const TemplateTypeGenerationResult& template_ : candidate_templates )
	{
		U_UNUSED( template_ );
		U_ASSERT( template_.deduced_template_parameters.size() >= arg_count );
	}

	for( size_t arg_n= 0u; arg_n < arg_count; ++arg_n )
	{
		for( const TemplateTypeGenerationResult& candidate_l : candidate_templates )
		{
			bool is_best_template_for_current_arg= true;
			for( const TemplateTypeGenerationResult& candidate_r : candidate_templates )
			{
				const ConversionsCompareResult comp=
					TemplateSpecializationCompare( candidate_l.deduced_template_parameters[arg_n], candidate_r.deduced_template_parameters[arg_n] );

				if( comp == ConversionsCompareResult::Incomparable || comp == ConversionsCompareResult::RightIsBetter )
				{
					is_best_template_for_current_arg= false;
					break;
				}
			}

			if( is_best_template_for_current_arg )
			{
				for( const TemplateTypeGenerationResult& candidate_r : candidate_templates )
				{
					const ConversionsCompareResult comp=
						TemplateSpecializationCompare( candidate_l.deduced_template_parameters[arg_n], candidate_r.deduced_template_parameters[arg_n] );

					if( comp != ConversionsCompareResult::Same )
						best_templates[ &candidate_r - candidate_templates.data() ]= false;
				}
			}
		}
	}

	const TemplateTypeGenerationResult* selected_template= nullptr;
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

} // namespace CodeBuilderPrivate

} // namespace U
