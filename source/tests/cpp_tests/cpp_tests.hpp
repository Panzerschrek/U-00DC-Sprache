#pragma once

#include "../../code_builder_lib_common/code_builder_errors.hpp"
#include "../execution_engine.hpp"

#include "../tests_lib/funcs_registrator.hpp"
#include "../tests_lib/tests.hpp"

namespace U
{

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
