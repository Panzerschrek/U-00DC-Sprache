#include <iostream>
#include "push_disable_llvm_warnings.hpp"
#include <llvm/Support/raw_os_ostream.h>
#include "pop_llvm_warnings.hpp"

#include "execution_engine.hpp"

namespace U
{

ExecutionEngine::ExecutionEngine( std::unique_ptr<llvm::Module> module )
	: module_( std::move(module) )
	, evaluator_( module_->getDataLayout() )
{
}

llvm::Function* ExecutionEngine::FindFunctionNamed( const llvm::StringRef name )
{
	return module_->getFunction( name );
}

llvm::GlobalVariable* ExecutionEngine::FindGlobalVariableNamed( const llvm::StringRef name, const bool allow_internal )
{
	return module_->getGlobalVariable( name, allow_internal );
}

llvm::GenericValue ExecutionEngine::runFunction(
	llvm::Function* const function,
	const llvm::ArrayRef<llvm::GenericValue> args )
{
	ConstexprFunctionEvaluator::ResultGeneric res= evaluator_.Evaluate( function, args );

	for (const std::string& error : res.errors )
		std::cout << error << std::endl;

	return std::move(res.result);
}

void ExecutionEngine::RegisterCustomFunction( const llvm::StringRef name, const CustomFunction function )
{
	evaluator_.RegisterCustomFunction( name, function );
}

EnginePtr CreateEngine( std::unique_ptr<llvm::Module> module, const bool needs_dump )
{
	if( needs_dump )
	{
		llvm::raw_os_ostream stream(std::cout);
		module->print( stream, nullptr );
	}

	return std::make_unique<ExecutionEngine>( std::move(module) );
}

} // namespace U
