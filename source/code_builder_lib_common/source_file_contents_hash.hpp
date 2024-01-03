#pragma once
#include <string>
#include <string_view>

namespace U
{

// This function should produce deterministic hash undependent on target architecture/compiler!
// For now it returns MD5 hash of given string.
std::string CalculateSourceFileContentsHash( std::string_view contents );

} // namespace U
