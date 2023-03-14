#pragma once
#include <iostream>
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../code_builder_lib_common/constexpr_function_evaluator.hpp"

namespace U
{

// Wrapper class over handmade execution engine, that is used for tests.

class ExecutionEngineException  final : public std::runtime_error
{
public:
	ExecutionEngineException()
		: std::runtime_error( "execution engine exception" )
	{}
};

class ExecutionEngine
{
public:
	explicit ExecutionEngine( std::unique_ptr<llvm::Module> module ): module_( std::move(module) )
		, evaluator_( module_->getDataLayout() )
	{}

	llvm::Function* FindFunctionNamed( const llvm::StringRef name )
	{
		return module_->getFunction( name );
	}

	llvm::GlobalVariable* FindGlobalVariableNamed( const llvm::StringRef name, const bool allow_internal= false )
	{
		return module_->getGlobalVariable( name, allow_internal );
	}

	llvm::GenericValue runFunction( llvm::Function* const function, const llvm::ArrayRef<llvm::GenericValue> args )
	{
		ConstexprFunctionEvaluator::ResultGeneric res= evaluator_.Evaluate( function, args );

		for (const std::string& error : res.errors )
			std::cout << error << std::endl;
		if( !res.errors.empty() )
			throw ExecutionEngineException();

		return std::move(res.result);
	}

	using CustomFunction= ConstexprFunctionEvaluator::CustomFunction;
	void RegisterCustomFunction( llvm::StringRef name, CustomFunction function )
	{
		evaluator_.RegisterCustomFunction( name, function );
	}

private:
	std::unique_ptr<llvm::Module> module_;
	ConstexprFunctionEvaluator evaluator_;
};

using EnginePtr= std::unique_ptr<ExecutionEngine>;

inline EnginePtr CreateEngine( std::unique_ptr<llvm::Module> module, const bool needs_dump= false )
{
	if( needs_dump )
	{
		llvm::raw_os_ostream stream(std::cout);
		module->print( stream, nullptr );
	}

	return std::make_unique<ExecutionEngine>( std::move(module) );
}

} // namespace U
