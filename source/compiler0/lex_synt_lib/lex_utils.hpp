#pragma once
#include <optional>
#include "lexical_analyzer.hpp"

namespace U
{

// Complexity is linear.
const Lexem* GetLexemForPosition( const uint32_t line, const uint32_t column, const Lexems& lexems );

} // namespace U
