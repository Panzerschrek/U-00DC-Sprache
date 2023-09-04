#pragma once
#include "../compiler0/lex_synt_lib/lexical_analyzer.hpp"
#include "document_position.hpp"

namespace U
{

namespace LangServer
{

// Complexity is linear.
std::optional<TextLinearPosition> DocumentPositionToLinearPosition( const DocumentPosition& pos, std::string_view text );

std::optional<TextLinearPosition> GetUtf8LineStartPosition( std::string_view text, uint32_t line /* from 1*/ );
std::optional<TextLinearPosition> Utf16PositionToUtf8Position( std::string_view text, TextLinearPosition position );
std::optional<TextLinearPosition> Utf8PositionToUtf16Position( std::string_view text, TextLinearPosition position );
std::optional<TextLinearPosition> Utf8PositionToUtf32Position( std::string_view text, TextLinearPosition position );
std::optional<TextLinearPosition> Utf32PositionToUtf8Position( std::string_view text, TextLinearPosition position );
std::optional<TextLinearPosition> Utf32PositionToUtf16Position( std::string_view text, TextLinearPosition position );

TextLinearPosition GetLineStartUtf8Position( std::string_view text, TextLinearPosition position );

std::optional<SrcLoc> GetSrcLocForIndentifierStartPoisitionInText( std::string_view text, const DocumentPosition& position );

std::optional<DocumentRange> SrcLocToDocumentIdentifierRange( const SrcLoc& src_loc, std::string_view program_text, const LineToLinearPositionIndex& line_to_linear_position_index );

std::optional<DocumentPosition> GetIdentifierEndPosition( const DocumentPosition& position, const std::string_view program_text, const LineToLinearPositionIndex& line_to_linear_position_index );

} //namespace LangServer

} // namespace U
