#pragma once
#include <cstdint>

namespace U
{

// This enum must match same enum in Compiler1 code!
enum class ManglingScheme : uint8_t
{
	ItaniumABI,
	MSVC, // Auto-select 32-bit or 64-bit scheme.
	MSVC32,
	MSVC64,
};

} // namespace
