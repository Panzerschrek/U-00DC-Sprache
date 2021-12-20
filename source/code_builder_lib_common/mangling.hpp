#pragma once
#include <cstdint>

namespace U
{

enum class ManglingScheme : uint8_t
{
	ItaniumABI,
	MSVC,
};

} // namespace
