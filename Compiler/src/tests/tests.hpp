#pragma once

#include "../push_disable_llvm_warnings.hpp"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include "../pop_llvm_warnings.hpp"

#include "../code_builder.hpp"

namespace U
{

std::unique_ptr<llvm::Module> BuildProgram( const char* text );
CodeBuilder::BuildResult BuildProgramWithErrors( const char* text );
llvm::ExecutionEngine* CreateEngine( std::unique_ptr<llvm::Module> module, bool needs_dump= false );

} // namespace U
