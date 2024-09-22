#pragma once
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IRBuilder.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "lambdas.hpp"
#include "references_graph.hpp"

namespace U
{

struct FunctionContext;

struct AllocaInfo
{
	llvm::Value* is_stack_allocation= nullptr; // A boolean value indicating stack or heap allocation.
	llvm::Value* ptr_for_free= nullptr; // Pointer for free or for llvm.stackrestore.
};

// Usage - create this struct on stack. FunctionContext::stack_variables_stack will be controlled automatically.
// But you still need call "CallDestructors" manually.
struct StackVariablesStorage
{
public:
	StackVariablesStorage( FunctionContext& function_context );
	~StackVariablesStorage();

	void RegisterVariable( VariablePtr variable );

public:
	FunctionContext& function_context_;
	std::vector<VariablePtr> variables_;
	std::vector<AllocaInfo> allocas_;
};

struct LoopFrame
{
	llvm::BasicBlock* block_for_break= nullptr;
	llvm::BasicBlock* block_for_continue= nullptr;
	// Number of stack variable storages at stack before loop block creation.
	size_t stack_variables_stack_size= 0u;

	std::string name; // Contains label or is empty for non-labeled loops.

	// Populated during loop body building.
	std::vector<ReferencesGraph> break_variables_states;
	std::vector<ReferencesGraph> continue_variables_states;
};

struct ReturnTypeDeductionContext
{
	// Filled during function preprocessing.
	std::optional<Type> return_type;
};

struct ReferenceNotationDeductionContext
{
	// Filled during function preprocessing.
	FunctionType::ReturnReferences return_references;
	FunctionType::ReturnInnerReferences return_inner_references;
	FunctionType::ReferencesPollution references_pollution;
};

struct FunctionContext
{
public:
	FunctionContext(
		FunctionType function_type,
		llvm::LLVMContext& llvm_context,
		llvm::Function* function );

	FunctionContext(const FunctionContext&)= delete;

public:
	const FunctionType function_type;

	llvm::Function* const function;

	llvm::IRBuilder<> alloca_ir_builder; // Use this builder for "alloca" instructions.

	llvm::BasicBlock* const function_basic_block; // Next block after all "alloca" instructions.
	llvm::IRBuilder<> llvm_ir_builder; // Use this builder for all instructions, except "alloca"

	// For reference checks.
	// arg variable node + inner reference nodes.
	ArgsVector< std::pair< VariablePtr, llvm::SmallVector< VariablePtr, 1 > > > args_nodes;

	llvm::Value* s_ret= nullptr; // Value for assignment for "sret" functions. Also it is a promise for coroutines.

	// Specific for coroutines data.
	llvm::BasicBlock* coro_final_suspend_bb= nullptr; // Used to jump from "return" operator.
	llvm::BasicBlock* coro_suspend_bb= nullptr; // Used as final destination for "yield" and "return".
	llvm::BasicBlock* coro_cleanup_bb= nullptr; // Used as final destination after suspention destruction blocks.

	VariablePtr this_; // null for non-class functions or static member functions.
	ClassFieldsVector<bool> initialized_this_fields; // Non-empty in constructors. element is true, if field is already initialized.

	llvm::BasicBlock* destructor_end_block= nullptr; // exists, if function is destructor

	std::vector<LoopFrame> loops_stack;

	// Contains pointers to stack variables with pointers needed to be freed.
	std::vector<llvm::Value*> heap_allocations_to_free_at_return;

	// Stack for stack variables.
	// First entry is set of function arguments.
	// Each block adds new storage for it`s variables.
	// Also, evaluation of some operators and expressions adds their variables storages.
	// Do not push/pop to this stack manually!
	std::vector<StackVariablesStorage*> stack_variables_stack;
	ReferencesGraph variables_state;

	// Cache result of arguments pre-evaluation for selection of overloaded functions and operators.
	// This needed for reducing exponential expression evaluation complexity.
	std::unordered_map< const Synt::Expression*, FunctionType::Param > args_preevaluation_cache;

	llvm::DIScope* current_debug_info_scope= nullptr;

	// Observer pointers. May be null.
	ReturnTypeDeductionContext* return_type_deduction_context= nullptr; // Non-null if preprocessing auto-return function.
	ReferenceNotationDeductionContext* reference_notation_deduction_context= nullptr; // Non-null if performing reference notation deduction in preprocessing.
	LambdaPreprocessingContext* lambda_preprocessing_context= nullptr; // Non-null if this is preprocessed lambda.

	// Keep "bool" fields last in order to reduce gaps between fields.

	bool base_initialized= false;
	bool whole_this_is_unavailable= false; // May be true in constructor initializer list, in body of constructors and destructors of abstract classes.
	bool has_non_constexpr_operations_inside= false; // While building code, may set to "true".
	bool is_in_unsafe_block= false;
	bool is_functionless_context= false; // True for global function context or for function context, used for args preevaluation or typeof operator.
};

} // namespace U
