#pragma once
#include <string_view>
#include <llvm/ADT/StringRef.h>

namespace U
{

// HACK! Shitty MSVC fails to convert string_view to llvm::StringRef. So, do this with such helper function.
// TODO - remove it, when this problem will be fixed in MSVC or in LLVM.
inline llvm::StringRef StringViewToStringRef( const std::string_view s )
{
	return llvm::StringRef( s.data(), s.size() );
}

} // namespace U
