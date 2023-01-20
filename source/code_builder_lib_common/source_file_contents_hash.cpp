#include "push_disable_llvm_warnings.hpp"
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/MD5.h>
#include "pop_llvm_warnings.hpp"

#include "source_file_contents_hash.hpp"

namespace U
{

std::string CalculateSourceFileContentsHash( const std::string_view contents )
{
	llvm::MD5 hash;
	hash.update( llvm::StringRef( contents.data(), contents.size() ) );
	llvm::MD5::MD5Result result;
	hash.final( result );

	llvm::SmallString<32> str_result;
	llvm::MD5::stringifyResult( result, str_result );
	return str_result.str().str();
}

} // namespace U
