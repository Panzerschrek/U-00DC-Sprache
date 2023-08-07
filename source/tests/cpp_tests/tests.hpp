#pragma once

#include "../../code_builder_lib_common/code_builder_errors.hpp"
#include "../execution_engine.hpp"

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

std::unique_ptr<llvm::Module> BuildMultisourceProgram( std::vector<SourceEntry> sources, const std::string& root_file_path, bool report_about_unused_names= false );
ErrorTestBuildResult BuildMultisourceProgramWithErrors( std::vector<SourceEntry> sources, const std::string& root_file_path );

std::unique_ptr<llvm::Module> BuildProgramForLifetimesTest( const char* text );
std::unique_ptr<llvm::Module> BuildProgramForMSVCManglingTest( const char* text );

bool HaveError( const std::vector<CodeBuilderError>& errors, CodeBuilderErrorCode code, uint32_t line );

} // namespace U
