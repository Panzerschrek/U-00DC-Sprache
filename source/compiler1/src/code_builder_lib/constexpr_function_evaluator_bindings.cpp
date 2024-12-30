// If this file changed, "constexpr_function_evaluator_bindings.uh" must be changed too!

#include "../../../code_builder_lib_common/interpreter.hpp"

extern "C"
{

U::Interpreter* U1_ConstexprFunctionEvaluatorCreate( const llvm::DataLayout& data_layout )
{
	return new U::Interpreter( data_layout );
}

void U1_ConstexprFunctionEvaluatorDestroy( U::Interpreter* const constexpr_function_evaluator )
{
	delete constexpr_function_evaluator;
}

using ConstexprFunctionEvaluatorErrorHandlerFunc= void(*)( void* user_data, const char* error_text_start, size_t error_text_size );

LLVMValueRef U1_ConstexprFunctionEvaluatorEvaluate(
	U::Interpreter& constexpr_function_evaluator,
	const LLVMValueRef function,
	const LLVMValueRef* const args_start,
	const size_t arg_count,
	const ConstexprFunctionEvaluatorErrorHandlerFunc error_handler_func,
	void* const user_data )
{
	const auto res=
		constexpr_function_evaluator.EvaluateConstexpr(
			llvm::dyn_cast<llvm::Function>(llvm::unwrap(function)),
			llvm::ArrayRef<const llvm::Constant*>( reinterpret_cast<const llvm::Constant* const*>(args_start), arg_count ) );

	for( const std::string& err : res.errors )
		error_handler_func( user_data, err.data(), err.size() );

	return llvm::wrap(res.result_constant);
}

} // extern "C"
