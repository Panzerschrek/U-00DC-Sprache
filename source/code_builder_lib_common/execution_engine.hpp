#pragma once
#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Module.h>
#include "pop_llvm_warnings.hpp"

#include "constexpr_function_evaluator.hpp"

namespace U
{

class ExecutionEngine
{
public:
	explicit ExecutionEngine( std::unique_ptr<llvm::Module> module );

	llvm::Function* FindFunctionNamed( llvm::StringRef name );
	llvm::GlobalVariable* FindGlobalVariableNamed( llvm::StringRef name, bool allow_internal= false );

	llvm::GenericValue runFunction( llvm::Function* function, llvm::ArrayRef<llvm::GenericValue> args );

private:
	std::unique_ptr<llvm::Module> module_;
	ConstexprFunctionEvaluator evaluator_;
};

using EnginePtr= std::unique_ptr<ExecutionEngine>;

EnginePtr CreateEngine( std::unique_ptr<llvm::Module> module, bool needs_dump= false );

} // namespace U
