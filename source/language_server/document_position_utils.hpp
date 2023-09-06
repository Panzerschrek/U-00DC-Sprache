#pragma once
#include "../compiler0/lex_synt_lib/lexical_analyzer.hpp"
#include "document_position.hpp"

namespace U
{

namespace LangServer
{

// Get position withing string (for insertion, for example).
// Complexity is linear.
std::optional<TextLinearPosition> DocumentPositionToLinearPosition( const DocumentPosition& pos, std::string_view text );

std::optional<SrcLoc> GetSrcLocForIndentifierStartPoisitionInText( std::string_view text, uint32_t line, TextLinearPosition linear_position );

std::optional<DocumentRange> SrcLocToDocumentIdentifierRange( const SrcLoc& src_loc, std::string_view program_text, const LineToLinearPositionIndex& line_to_linear_position_index );

} //namespace LangServer

} // namespace U
