#pragma once
#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include "pop_llvm_warnings.hpp"
#include "small_types.hpp"

namespace U
{

/*
	This class is simple virtual machine for llvm functions executing.
	It have some limitations and supports subset ob instructions, using in Ü compiler.

	Supported calls of functions with no side effects.
	Supported returning of scalar types and composite types via "sret".
	Returning of pointers not supported.
*/

class ConstexprFunctionEvaluator final
{
public:
	struct Result
	{
		llvm::Constant* result_constant= nullptr;
		std::vector<std::string> errors;
	};

	ConstexprFunctionEvaluator( const llvm::DataLayout& data_layout );

	Result Evaluate(
		llvm::Function* const llvm_function,
		llvm::ArrayRef<const llvm::Constant*> args );

private:
	llvm::GenericValue CallFunction( const llvm::Function& llvm_function, size_t stack_depth );

	// Returns offset
	size_t MoveConstantToStack( const llvm::Constant& constant );
	void CopyConstantToStack( const llvm::Constant& constant, size_t stack_offset );

	llvm::Constant* CreateInitializerForStructElement( llvm::Type* type, size_t element_ptr );

	llvm::GenericValue GetVal( const llvm::Value* val );
	void ProcessAlloca( const llvm::Instruction* instruction );
	void ProcessLoad( const llvm::Instruction* instruction );
	void ProcessStore( const llvm::Instruction* instruction );
	void ProcessGEP( const llvm::Instruction* instruction );
	void ProcessCall( const llvm::Instruction* instruction, size_t stack_depth );
	void ProcessMemmove( const llvm::Instruction* instruction );

	void ProcessUnaryArithmeticInstruction( const llvm::Instruction* instruction );
	void ProcessBinaryArithmeticInstruction( const llvm::Instruction* instruction );

	void ReportDataStackOverflow();
	void ReportConstantsStackOverflow();

private:
	using InstructionsMap= std::unordered_map< const llvm::Value*, llvm::GenericValue >;

	const llvm::DataLayout data_layout_;

	InstructionsMap instructions_map_;
	std::vector<unsigned char> stack_;
	std::vector<unsigned char> constants_stack_;

	std::unordered_map<const llvm::Constant*, size_t> external_constant_mapping_;

	std::vector<std::string> errors_;
};

} // namespace U
