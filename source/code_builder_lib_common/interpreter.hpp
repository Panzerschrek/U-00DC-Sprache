#pragma once
#include <cstddef>
#include <unordered_map>
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

struct InterpreterOptions
{
	// Default limits intended to be used in constexpr evaluator.
	size_t max_call_stack_depth= 512;
	uint64_t max_instructions_executed= uint64_t(1) << 24;
};

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

class Interpreter
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
	explicit Interpreter( const llvm::DataLayout& data_layout, InterpreterOptions options= InterpreterOptions() );
	Interpreter( const Interpreter& ) = delete;
	Interpreter& operator=( const Interpreter& )= delete;

	// Evaluate result of "constexpr" call.
	// it suports value and pointer args, it can return value args.
	// Provided args should be equal to expected param types or compatible with it (have same size).
	ResultConstexpr EvaluateConstexpr( const llvm::Function& llvm_function, llvm::ArrayRef<const llvm::Constant*> args, llvm::Type& return_type );

	// Evaluate any other call.
	// Pointer args are not supported.
	// Also globals state is preserved after this call.
	ResultGeneric EvaluateGeneric( const llvm::Function& llvm_function, llvm::ArrayRef<llvm::GenericValue> args );

	void RegisterCustomFunction( llvm::StringRef name, CustomFunction function );

	// Read data from address space of execution engine.
	void ReadExecutinEngineData( void* dst, uint64_t address, size_t size ) const;

private:
	using InstructionsMap= llvm::DenseMap< const llvm::Value*, llvm::GenericValue >;

	struct CoroutineData;

	struct CallFrame
	{
		InstructionsMap instructions_map;
		std::shared_ptr<CoroutineData> coroutine_data;
		bool is_coroutine= false;
	};

	struct CoroutineData
	{
		InstructionsMap instructions_map; // Save here all instruction in case of suspend.
		llvm::GenericValue promise;
		const llvm::CallInst* suspend_instruction= nullptr;
		bool done= false;
	};

	struct MemoryRegionView
	{
		std::byte* ptr= nullptr;
		size_t size= 0;
	};

private:
	ResultConstexpr PrepareResultAndClear();

	llvm::GenericValue CallFunction( const llvm::Function& llvm_function );
	llvm::GenericValue ExecuteFunctionBody( const llvm::Instruction* instruction );

	// Returns offset
	size_t MoveConstantToStack( const llvm::Constant& constant );
	void CopyConstantToStack( const llvm::Constant& constant, size_t stack_offset );

	llvm::Constant* ReadConstantFromStack( llvm::Type* type, size_t value_ptr );

	llvm::GenericValue BuildGEP( const llvm::User* instruction );

	llvm::GenericValue GetVal( const llvm::Value* val );
	void ProcessAlloca( const llvm::Instruction* instruction );

	MemoryRegionView GetMemoryForVirtualAddress( size_t offset );

	void ProcessLoad( const llvm::Instruction* instruction );
	llvm::GenericValue DoLoad( MemoryRegionView memory_region, llvm::Type* t );

	void ProcessStore( const llvm::Instruction* instruction );
	void DoStore( MemoryRegionView memory_region, const llvm::GenericValue& val, llvm::Type* t );

	void ProcessGEP( const llvm::Instruction* instruction );
	void ProcessCall( const llvm::CallInst* instruction );
	void ProcessMemmove( const llvm::CallInst* instruction );
	void ProcessMalloc( const llvm::CallInst* instruction );
	void ProcessRealloc( const llvm::CallInst* instruction );
	void ProcessFree( const llvm::CallInst* instruction );

	void ProcessCoroId( const llvm::CallInst* instruction );
	void ProcessCoroAlloc( const llvm::CallInst* instruction );
	void ProcessCoroFree( const llvm::CallInst* instruction );
	void ProcessCoroSize( const llvm::CallInst* instruction );
	void ProcessCoroBegin( const llvm::CallInst* instruction );
	void ProcessCoroEnd( const llvm::CallInst* instruction );
	void ProcessCoroSuspend( const llvm::CallInst* instruction );
	void ProcessCoroResume( const llvm::CallInst* instruction );
	void ProcessCoroDestroy( const llvm::CallInst* instruction );
	void ProcessCoroDone( const llvm::CallInst* instruction );
	void ProcessCoroPromise( const llvm::CallInst* instruction );

	void ProcessSAddWithOverflow( const llvm::CallInst* instruction );
	void ProcessUAddWithOverflow( const llvm::CallInst* instruction );
	void ProcessSSubWithOverflow( const llvm::CallInst* instruction );
	void ProcessUSubWithOverflow( const llvm::CallInst* instruction );
	void ProcessSMulWithOverflow( const llvm::CallInst* instruction );
	void ProcessUMulWithOverflow( const llvm::CallInst* instruction );

	void ProcessStacksave( const llvm::CallInst* instruction );
	void ProcessStackrestore( const llvm::CallInst* instruction );

	void ProcessThreadLocalAddress( const llvm::CallInst* instruction );

	void ResumeCoroutine( const llvm::CallInst* instruction, bool destroy );

	void ProcessUnaryArithmeticInstruction( const llvm::Instruction* instruction );
	void ProcessBinaryArithmeticInstruction( const llvm::Instruction* instruction );

	size_t GetFunctionIndex( const llvm::Function* function );

	void ReportDataStackOverflow();
	void ReportGlobalsStackOverflow();

	void ReportError(std::string_view text, const llvm::Instruction& instruction );
	void ReportError(std::string_view text );
	std::string GetCurrentCallStackDescription();

private:
	const llvm::DataLayout data_layout_;
	const uint32_t pointer_size_in_bits_;
	const InterpreterOptions options_;

	CallFrame current_function_frame_;

	std::vector<std::byte> stack_;
	std::vector<std::byte> globals_stack_;
	std::vector<std::byte> heap_;

	// Allocate coroutine data here and provide user code only handle value.
	uint32_t next_coroutine_id_= 1u;
	llvm::DenseMap<uint32_t, std::shared_ptr<CoroutineData>> coroutines_data_;

	llvm::DenseMap<const llvm::Constant*, size_t> external_constant_mapping_;

	// Store just sequential integers for values for function pointers.
	// Use this container to obtain functions for such integers.
	// Doing so we can make sure, that no arbitrary address can be casted to llvm::Function*.
	llvm::DenseMap<const llvm::Function*, size_t> function_pointers_map_;
	llvm::SmallVector<const llvm::Function*> function_pointers_mapping_vec_;

	llvm::StringMap<CustomFunction> custom_functions_;

	llvm::SmallVector<const llvm::CallInst*, 8> call_stack_;
	uint64_t instructions_executed_= 0;

	std::vector<std::string> errors_;
};

} // namespace U
