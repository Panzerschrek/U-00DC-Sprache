#pragma once
#include <vector>

#include <llvm/IR/Module.h>

#include "code_builder_llvm_types.hpp"
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
	Type PrepareType( const TypeName& type_name );

	void BuildFuncCode( Variable& func, const ProgramString& func_name );

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

		llvm::IntegerType* f32;
		llvm::IntegerType* f64;

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
