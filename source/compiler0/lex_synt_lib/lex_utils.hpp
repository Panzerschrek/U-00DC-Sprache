#pragma once
#include <optional>
#include "lexical_analyzer.hpp"

namespace U
{

// Complexity is linear.
std::optional<SrcLoc> GetLexemSrcLocForPosition( const uint32_t line, const uint32_t column, const Lexems& lexems );

SrcLoc GetLexemEnd( const uint32_t line, const uint32_t column, const Lexems& lexems );

} // namespace U
