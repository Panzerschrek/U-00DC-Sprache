#pragma once
#include <cstddef>
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
	It has some limitations and supports subset ob instructions, using in Ü compiler.

	It supports calls of constexpr functions with no side effects,
	with possibility of scalar type return or return of composite types via "sret".
	Pointers return values are not supported.

	It also supports calls of any other functions, but only with scalar types arguments and return value.
	For such calls state of global variables is preserved.

	This Interpreter has its own virtual address space consisting of two stacks - for local and for global data.
	External memory access is not supported.
*/

class Interpreter final
{
public:
	struct ResultConstexpr
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

public:
	Interpreter( const llvm::DataLayout& data_layout );
	Interpreter( const Interpreter& ) = delete;
	Interpreter& operator=( const Interpreter& )= delete;

	// Evaluate result of "constexpr" call.
	ResultConstexpr EvaluateConstexpr( llvm::Function* llvm_function, llvm::ArrayRef<const llvm::Constant*> args );

	// Evaluate any other call.
	// Pointer args are not supported.
	// Also globals state is preserved after this call.
	ResultGeneric EvaluateGeneric( llvm::Function* llvm_function, llvm::ArrayRef<llvm::GenericValue> args );

	void RegisterCustomFunction( llvm::StringRef name, CustomFunction function );

	// Read data from address space of execution engine.
	void ReadExecutinEngineData( void* dst, uint64_t address, size_t size ) const;

private:
	ResultConstexpr PrepareResultAndClear();

	llvm::GenericValue CallFunction( const llvm::Function& llvm_function, size_t stack_depth );

	// Returns offset
	size_t MoveConstantToStack( const llvm::Constant& constant );
	void CopyConstantToStack( const llvm::Constant& constant, size_t stack_offset );

	llvm::Constant* ReadConstantFromStack( llvm::Type* type, size_t value_ptr );

	llvm::GenericValue BuildGEP( const llvm::User* instruction );

	llvm::GenericValue GetVal( const llvm::Value* val );
	void ProcessAlloca( const llvm::Instruction* instruction );

	std::byte* GetMemoryForVirtualAddress( size_t offset );

	void ProcessLoad( const llvm::Instruction* instruction );
	llvm::GenericValue DoLoad( const std::byte* ptr, llvm::Type* t );

	void ProcessStore( const llvm::Instruction* instruction );
	void DoStore( std::byte* ptr, const llvm::GenericValue& val, llvm::Type* t );

	void ProcessGEP( const llvm::Instruction* instruction );
	void ProcessCall( const llvm::CallInst* instruction, size_t stack_depth );
	void ProcessMemmove( const llvm::Instruction* instruction );
	void ProcessMalloc( const llvm::CallInst* instruction );
	void ProcessFree( const llvm::CallInst* instruction );

	void ProcessUnaryArithmeticInstruction( const llvm::Instruction* instruction );
	void ProcessBinaryArithmeticInstruction( const llvm::Instruction* instruction );

	void ReportDataStackOverflow();
	void ReportGlobalsStackOverflow();

private:
	using InstructionsMap= llvm::DenseMap< const llvm::Value*, llvm::GenericValue >;

	const llvm::DataLayout data_layout_;
	const uint32_t pointer_size_in_bits_;

	InstructionsMap instructions_map_;
	std::vector<std::byte> stack_;
	std::vector<std::byte> globals_stack_;
	std::vector<std::byte> heap_;

	llvm::DenseMap<const llvm::Constant*, size_t> external_constant_mapping_;

	llvm::StringMap<CustomFunction> custom_functions_;

	std::vector<std::string> errors_;
};

} // namespace U
