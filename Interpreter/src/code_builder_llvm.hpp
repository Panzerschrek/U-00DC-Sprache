#pragma once
#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include "code_builder_llvm_types.hpp"
#include "inverse_polish_notation.hpp"
#include "syntax_elements.hpp"

namespace Interpreter
{

namespace CodeBuilderLLVMPrivate
{

class CodeBuilderLLVM final
{
public:
	CodeBuilderLLVM();
	~CodeBuilderLLVM();

	struct BuildResult
	{
		std::vector<std::string> error_messages;
		std::unique_ptr<llvm::Module> module;
	};

	BuildResult BuildProgram( const ProgramElements& program_elements );

private:
	struct FunctionContext
	{
		FunctionContext(
			llvm::LLVMContext& llvm_context,
			llvm::Function* function );

		llvm::BasicBlock* function_basic_block;
		llvm::IRBuilder<> llvm_ir_builder;
	};

private:
	Type PrepareType( const TypeName& type_name );

	void BuildFuncCode(
		Variable& func,
		const ProgramString& func_name,
		const std::vector<ProgramString>& arg_names,
		const Block& block );

	void BuildBlockCode(
		const Block& block,
		const NamesScope& names,
		FunctionContext& function_context );

	Variable BuildExpressionCode(
		const BinaryOperatorsChain& expression,
		const NamesScope& names,
		FunctionContext& function_context );

	Variable BuildExpressionCode_r(
		const InversePolishNotation& ipn,
		unsigned int ipn_index,
		const NamesScope& names,
		FunctionContext& function_context );

	void BuildReturnOperatorCode(
		const ReturnOperator& return_operator,
		const NamesScope& names,
		FunctionContext& function_context );

private:
	llvm::LLVMContext& llvm_context_;

	struct
	{
		llvm::IntegerType*  i8;
		llvm::IntegerType*  u8;
		llvm::IntegerType* i16;
		llvm::IntegerType* u16;
		llvm::IntegerType* i32;
		llvm::IntegerType* u32;
		llvm::IntegerType* i64;
		llvm::IntegerType* u64;

		llvm::Type* f32;
		llvm::Type* f64;

		llvm::Type* void_;
		llvm::IntegerType* bool_;
	} fundamental_llvm_types_;

	std::unique_ptr<llvm::Module> module_;
	std::vector<std::string> error_messages_;
	unsigned int error_count_= 0u;

	NamesScope global_names_;
};

} // namespace CodeBuilderLLVMPrivate

} // namespace Interpreter
