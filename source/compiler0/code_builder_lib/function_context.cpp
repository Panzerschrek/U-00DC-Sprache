#include "template_signature_param.hpp"
#include "function_context.hpp"

namespace U
{

StackVariablesStorage::StackVariablesStorage( FunctionContext& in_function_context )
	: function_context_(in_function_context)
{
	function_context_.stack_variables_stack.push_back(this);
}

StackVariablesStorage::~StackVariablesStorage()
{
	for( const VariablePtr& variable : variables_ )
		function_context_.variables_state.RemoveNode( variable );
	function_context_.stack_variables_stack.pop_back();
}

void StackVariablesStorage::RegisterVariable( VariablePtr variable )
{
	variables_.push_back( std::move(variable) );
}

FunctionContext::FunctionContext(
	FunctionType in_function_type,
	llvm::LLVMContext& llvm_context,
	llvm::Function* const in_function )
	: function_type(std::move(in_function_type))
	, function(in_function)
	, alloca_ir_builder( llvm::BasicBlock::Create( llvm_context, "allocations", function ) )
	, function_basic_block( llvm::BasicBlock::Create( llvm_context, "func_code", function ) )
	, llvm_ir_builder( function_basic_block )
	, current_debug_info_scope( function->getSubprogram() )
{
}

} // namespace U
