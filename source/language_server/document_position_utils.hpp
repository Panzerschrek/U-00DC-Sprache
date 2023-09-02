#pragma once
#include "../compiler0/lex_synt_lib/lexical_analyzer.hpp"
#include "document_position.hpp"

namespace U
{

namespace LangServer
{

// Complexity is linear.
std::optional<TextLinearPosition> DocumentPositionToLinearPosition( const DocumentPosition& pos, std::string_view text );

DocumentPosition SrcLocToDocumentPosition( const SrcLoc& src_loc );
SrcLoc DocumentPositionToSrcLoc( const DocumentPosition& position );

std::optional<SrcLoc> GetIdentifierStartSrcLoc( const SrcLoc& src_loc, const std::string_view program_text, const LineToLinearPositionIndex& line_to_linear_position_index );
std::optional<SrcLoc> GetIdentifierEndSrcLoc( const SrcLoc& src_loc, const std::string_view program_text, const LineToLinearPositionIndex& line_to_linear_position_index );

} //namespace LangServer

} // namespace U
