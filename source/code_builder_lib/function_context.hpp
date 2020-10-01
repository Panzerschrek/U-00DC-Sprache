#pragma once
#include "../compilers_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IRBuilder.h>
#include "../compilers_common/pop_llvm_warnings.hpp"
#include "value.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

using OverloadingResolutionCache=
	std::unordered_map< const Synt::SyntaxElementBase*, std::optional<FunctionVariable> >;

struct FunctionContext;

// Usage - create this struct on stack. FunctionContext::stack_variables_stack will be controlled automatically.
// But you still need call "CallDestructors" manually.
struct StackVariablesStorage final
{
public:
	using NodeAndVariable= std::pair< ReferencesGraphNodePtr, Variable >;

	StackVariablesStorage( FunctionContext& function_context );
	~StackVariablesStorage();

	void RegisterVariable( NodeAndVariable node_and_variable );

public:
	FunctionContext& function_context_;
	std::vector<NodeAndVariable> variables_;
};

struct LoopFrame final
{
	llvm::BasicBlock* block_for_break= nullptr;
	llvm::BasicBlock* block_for_continue= nullptr;
	// Number of stack variable storages at stack before loop block creation.
	size_t stack_variables_stack_size= 0u;

	// Populated during loop body building.
	std::vector<ReferencesGraph> break_variables_states;
	std::vector<ReferencesGraph> continue_variables_states;
};

struct FunctionContext
{
	FunctionContext(
		Function function_type,
		const std::optional<Type>& return_type,
		llvm::LLVMContext& llvm_context,
		llvm::Function* function );

	FunctionContext(const FunctionContext&)= delete;

	Function function_type;
	const std::optional<Type> return_type; // std::nullopt if type not known yet and must be deduced.
	std::optional<Type> deduced_return_type; // for functions with "auto" return type.

	// For reference checks.
	// arg variable node + optional inner reference variable node.
	ArgsVector< std::pair< ReferencesGraphNodePtr, ReferencesGraphNodePtr > > args_nodes;

	const Variable* this_= nullptr; // null for nonclass functions or static member functions.
	llvm::Value* s_ret_= nullptr; // Value for assignment for "sret" functions.

	std::unordered_set<std::string> uninitialized_this_fields;
	bool base_initialized= false;
	bool whole_this_is_unavailable= false; // May be true in constructor initializer list, in body of constructors and destructors of abstract classes.
	bool have_non_constexpr_operations_inside= false; // While building code, may set to "true".

	llvm::Function* const function;

	llvm::BasicBlock* const alloca_basic_block; // Block #0 in function. Contains all "alloca" instructions.
	llvm::IRBuilder<> alloca_ir_builder; // Use this builder for "alloca" instructions.

	llvm::BasicBlock* const function_basic_block; // Next block after all "alloca" instructions.
	llvm::IRBuilder<> llvm_ir_builder; // Use this builder for all instructions, except "alloca"

	std::vector<LoopFrame> loops_stack;
	bool is_in_unsafe_block= false;

	// Stack for stack variables.
	// First entry is set of function arguments.
	// Each block adds new storage for it`s variables.
	// Also, evaluation of some operators and expressions adds their variables storages.
	// Do not push/pop to this stack manually!
	std::vector<StackVariablesStorage*> stack_variables_stack;
	ReferencesGraph variables_state;

	OverloadingResolutionCache overloading_resolution_cache;

	llvm::BasicBlock* destructor_end_block= nullptr; // exists, if function is destructor

	llvm::DIScope* current_debug_info_scope= nullptr;
};

} // namespace CodeBuilderPrivate

} // namespace U
