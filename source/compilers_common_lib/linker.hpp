#pragma once

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Triple.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"


namespace U
{

void RunLinker( const char* argv0, const llvm::Triple& triple, const std::string& input_temp_file_path, const std::string& output_file_path );

void RunLinkerCOFF( const char* argv0, const llvm::Triple& triple,  const std::string& input_temp_file_path, const std::string& output_file_path );
void RunLinkerELF( const char* argv0, const llvm::Triple& triple,  const std::string& input_temp_file_path, const std::string& output_file_path );

} // namespace U
