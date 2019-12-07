#include "function_context.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

StackVariablesStorage::StackVariablesStorage( FunctionContext& in_function_context )
	: function_context_(in_function_context)
{
	function_context_.stack_variables_stack.push_back(this);
}

StackVariablesStorage::~StackVariablesStorage()
{
	for( const NodeAndVariable& node_and_variable : variables_ )
		function_context_.variables_state.RemoveNode( node_and_variable.first );
	function_context_.stack_variables_stack.pop_back();
}

void StackVariablesStorage::RegisterVariable( NodeAndVariable node_and_variable )
{
	function_context_.variables_state.AddNode( node_and_variable.first );
	variables_.push_back( std::move(node_and_variable) );
}

FunctionContext::FunctionContext(
	const std::optional<Type>& in_return_type,
	const bool in_return_value_is_mutable,
	const bool in_return_value_is_reference,
	llvm::LLVMContext& llvm_context,
	llvm::Function* const in_function )
	: return_type(in_return_type)
	, return_value_is_mutable(in_return_value_is_mutable)
	, return_value_is_reference(in_return_value_is_reference)
	, function(in_function)
	, alloca_basic_block( llvm::BasicBlock::Create( llvm_context, "allocations", function ) )
	, alloca_ir_builder( alloca_basic_block )
	, function_basic_block( llvm::BasicBlock::Create( llvm_context, "func_code", function ) )
	, llvm_ir_builder( function_basic_block )
{
}

} // namespace CodeBuilderPrivate

} // namespace U
