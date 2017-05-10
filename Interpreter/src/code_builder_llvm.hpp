#pragma once
#include <vector>

#include <llvm/IR/Module.h>

#include "syntax_elements.hpp"

namespace Interpreter
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
	} types_;

	std::unique_ptr<llvm::Module> module_;
	std::vector<std::string> error_messages_;
};

} // namespace Interpreter
