#pragma once
#include "push_disable_llvm_warnings.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instructions.h>
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

	struct ResultGeneric
	{
		llvm::GenericValue result;
		std::vector<std::string> errors;
	};

	using CustomFunction= llvm::GenericValue (*)( llvm::FunctionType*, llvm::ArrayRef<llvm::GenericValue> );

	ConstexprFunctionEvaluator( const llvm::DataLayout& data_layout );

	Result Evaluate(
		llvm::Function* const llvm_function,
		llvm::ArrayRef<const llvm::Constant*> args );

	ResultGeneric Evaluate(
		llvm::Function* const llvm_function,
		llvm::ArrayRef<llvm::GenericValue> args );

	void RegisterCustomFunction( llvm::StringRef name, CustomFunction function );

private:
	Result PrepareResultAndClear();

	llvm::GenericValue CallFunction( const llvm::Function& llvm_function, size_t stack_depth );

	// Returns offset
	size_t MoveConstantToStack( const llvm::Constant& constant );
	void CopyConstantToStack( const llvm::Constant& constant, size_t stack_offset );

	llvm::Constant* ReadConstantFromStack( llvm::Type* type, size_t value_ptr );

	llvm::GenericValue BuildGEP( const llvm::User* instruction );

	llvm::GenericValue GetVal( const llvm::Value* val );
	void ProcessAlloca( const llvm::Instruction* instruction );

	void ProcessLoad( const llvm::Instruction* instruction );
	llvm::GenericValue DoLoad( const void* ptr, llvm::Type* t );

	void ProcessStore( const llvm::Instruction* instruction );
	void DoStore( void* ptr, const llvm::GenericValue& val, llvm::Type* t );

	void ProcessGEP( const llvm::Instruction* instruction );
	void ProcessCall( const llvm::CallInst* instruction, size_t stack_depth );
	void ProcessMemmove( const llvm::Instruction* instruction );

	void ProcessUnaryArithmeticInstruction( const llvm::Instruction* instruction );
	void ProcessBinaryArithmeticInstruction( const llvm::Instruction* instruction );

	void ReportDataStackOverflow();
	void ReportGlobalsStackOverflow();

private:
	using InstructionsMap= llvm::DenseMap< const llvm::Value*, llvm::GenericValue >;

	const llvm::DataLayout data_layout_;
	const uint32_t pointer_size_in_bits_;

	InstructionsMap instructions_map_;
	std::vector<unsigned char> stack_;
	std::vector<unsigned char> globals_stack_;

	llvm::DenseMap<const llvm::Constant*, size_t> external_constant_mapping_;

	llvm::StringMap<CustomFunction> custom_functions_;

	std::vector<std::string> errors_;
};

} // namespace U
