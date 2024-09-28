#pragma once
#include <iostream>
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../code_builder_lib_common/interpreter.hpp"

namespace U
{

class HaltException final : public std::exception
{
public:
	virtual const char* what() const noexcept override
	{
		return "Halt exception";
	}
};

class ExecutionEngineException  final : public std::runtime_error
{
public:
	ExecutionEngineException(std::vector<std::string> in_errors)
		: std::runtime_error( "execution engine exception" ), errors(std::move(in_errors))
	{}

public:
	std::vector<std::string> errors;
};

// Wrapper class over handmade execution engine, that is used for tests.

class ExecutionEngine
{
public:
	explicit ExecutionEngine( std::unique_ptr<llvm::Module> module ): module_( std::move(module) )
		, interpreter_( module_->getDataLayout(), GetInterpreterOptions() )
	{
		RegisterCustomFunction( "__U_halt", HaltCalled );
	}

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
		Interpreter::ResultGeneric res= interpreter_.EvaluateGeneric( function, args );

		if( !res.errors.empty() )
			throw ExecutionEngineException( std::move(res.errors) );

		return std::move(res.result);
	}

	using CustomFunction= Interpreter::CustomFunction;
	void RegisterCustomFunction( llvm::StringRef name, CustomFunction function )
	{
		interpreter_.RegisterCustomFunction( name, function );
	}

	// Read data from address space of execution engine.
	void ReadExecutinEngineData( void* const dst, const uint64_t address, const size_t size ) const
	{
		return interpreter_.ReadExecutinEngineData( dst, address, size );
	}

private:
	static llvm::GenericValue HaltCalled( llvm::FunctionType*, llvm::ArrayRef<llvm::GenericValue> )
	{
		// Return from interpreter, using native exception.
		throw HaltException();
	}

	static InterpreterOptions GetInterpreterOptions()
	{
		InterpreterOptions options;
		// Slightly increase execution limit compared to default.
		options.max_instructions_executed= uint64_t(1) << 40;
		return options;
	}

private:
	const std::unique_ptr<llvm::Module> module_;
	Interpreter interpreter_;
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
