#pragma once

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../compiler0/code_builder_lib/code_builder_errors.hpp"

#include "funcs_registrator.hpp"

namespace U
{

// Main tests assertion handler. Aborts test.
#define U_TEST_ASSERT(x) \
	if( !(x) )\
	{\
		throw TestException( #x );\
	}

#define DISABLE_TEST throw DisableTestException()

// Utility tests functions.

#define ASSERT_NEAR( x, y, eps ) U_TEST_ASSERT( std::abs( (x) - (y) ) <= (eps) )

using EnginePtr= std::unique_ptr<llvm::ExecutionEngine>;

struct ErrorTestBuildResult
{
	std::vector<CodeBuilderError> errors;
};

std::unique_ptr<llvm::Module> BuildProgram( const char* text );
ErrorTestBuildResult BuildProgramWithErrors( const char* text );

struct SourceEntry
{
	std::string file_path;
	const char* text;
};

std::unique_ptr<llvm::Module> BuildMultisourceProgram( std::vector<SourceEntry> sources, const std::string& root_file_path );
ErrorTestBuildResult BuildMultisourceProgramWithErrors( std::vector<SourceEntry> sources, const std::string& root_file_path );

EnginePtr CreateEngine( std::unique_ptr<llvm::Module> module, bool needs_dump= false );

bool HaveError( const std::vector<CodeBuilderError>& errors, CodeBuilderErrorCode code, unsigned int line );

} // namespace U
