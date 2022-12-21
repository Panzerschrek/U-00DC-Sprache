#pragma once
#include <string>
#include <string_view>

namespace U
{

std::string CalculateSourceFileContentsHash( std::string_view contents );

} // namespace U
