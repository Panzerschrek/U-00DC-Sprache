#pragma once
#include <string>
#include <string_view>

namespace U
{

// This function should produce deterministic hash undependent on target architecture/compiler!
// Result shoud be also long enough to minimine possible hash collisions.
// For now it returns MD5 hash of given string.
std::string CalculateLongStableHash( std::string_view contents );

} // namespace U
