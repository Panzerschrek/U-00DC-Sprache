// If this file changed, "constexpr_function_evaluator_bindings.uh" must be changed too!

#include "../../compilers_common/constexpr_function_evaluator.hpp"

extern "C" U::ConstexprFunctionEvaluator* U1_ConstexprFunctionEvaluatorCreate( const llvm::DataLayout& data_layout )
{
	return new U::ConstexprFunctionEvaluator( data_layout );
}

extern "C" void U1_ConstexprFunctionEvaluatorDestroy( U::ConstexprFunctionEvaluator* const constexpr_function_evaluator )
{
	delete constexpr_function_evaluator;
}

using ConstexprFunctionEvaluatorErrorHandlerFunc= void(*)( void* user_data, const char* error_text_start, size_t error_text_size );

extern "C" LLVMValueRef U1_ConstexprFunctionEvaluatorEvaluate(
	U::ConstexprFunctionEvaluator& constexpr_function_evaluator,
	const LLVMValueRef function,
	const LLVMValueRef* const args_start,
	const size_t arg_count,
	const ConstexprFunctionEvaluatorErrorHandlerFunc error_handler_func,
	void* const user_data )
{
	U::ArgsVector<llvm::Constant*> args;
	args.reserve( arg_count );
	for( size_t i= 0u; i < arg_count; ++i )
	{
		if( args_start[i] == nullptr )
			args.push_back( nullptr );
		else
			args.push_back( llvm::dyn_cast<llvm::Constant>( llvm::unwrap( args_start[i] ) ) );
	}

	const auto res= constexpr_function_evaluator.Evaluate( llvm::dyn_cast<llvm::Function>(llvm::unwrap(function)), args );
	for( const std::string& err : res.errors )
		error_handler_func( user_data, err.data(), err.size() );

	return llvm::wrap(res.result_constant);
}
