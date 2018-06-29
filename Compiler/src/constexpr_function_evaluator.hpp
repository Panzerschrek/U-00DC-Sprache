#pragma once
#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include "pop_llvm_warnings.hpp"

#include "code_builder_types.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

class ConstexprFunctionEvaluator final
{
public:
	struct Result
	{
		llvm::Constant* result_constant= nullptr;
	};

	ConstexprFunctionEvaluator( const llvm::DataLayout& data_layout );

	Result Evaluate(
		const Function& function_type,
		llvm::Function* const llvm_function,
		const std::vector<llvm::Constant*>& args );


private:
	llvm::GenericValue CallFunction( const llvm::Function& llvm_function );

	// Returns offset
	size_t MoveConstantToStack( const llvm::Constant& constant );

	llvm::GenericValue GetVal( const llvm::Value* val );
	void ProcessAlloca( const llvm::Instruction* instruction );
	void ProcessLoad( const llvm::Instruction* instruction );
	void ProcessStore( const llvm::Instruction* instruction );
	void ProcessGEP( const llvm::Instruction* instruction );
	void ProcessCall( const llvm::Instruction* instruction );

	void ProcessUnaryArithmeticInstruction( const llvm::Instruction* instruction );
	void ProcessBinaryArithmeticInstruction( const llvm::Instruction* instruction );

private:
	using InstructionsMap= std::unordered_map< const llvm::Value*, llvm::GenericValue >;

	const llvm::DataLayout data_layout_;

	InstructionsMap instructions_map_;
	std::vector<unsigned char> stack_;

};

} // namespace CodeBuilderPrivate

} // namespace U
