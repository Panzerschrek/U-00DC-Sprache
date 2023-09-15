#include "../lex_synt_lib_common/assert.hpp"
#include "document_position_utils.hpp"
#include "program_string.hpp"

namespace U
{

namespace LangServer
{

namespace
{

// Get linear position for given line.
// Complexity is linear.
std::optional<TextLinearPosition> GetUtf8LineStartPosition( const std::string_view text, const uint32_t line /* from 1 */ )
{
	uint32_t current_line= 1; // Count lines from one.
	TextLinearPosition line_linear_pos= 0;

	if( current_line >= line ) // Handle line 1 and (even if it is wrong) line 0
		return line_linear_pos;

	while( line_linear_pos < text.size() )
	{
		if( text[line_linear_pos] == '\n' )
		{
			++current_line;
			if( current_line == line )
				return line_linear_pos + 1;
		}
		++line_linear_pos;
	}

	// Reached text end without reaching target line.
	return std::nullopt;
}

} // namespace

std::optional<TextLinearPosition> DocumentPositionToLinearPosition( const DocumentPosition& pos, const std::string_view text )
{
	const auto line_linear_pos= GetUtf8LineStartPosition( text, pos.line );
	if( line_linear_pos == std::nullopt )
		return std::nullopt;

	const auto column_offset= Utf16PositionToUtf8Position( text.substr( *line_linear_pos ), pos.character );
	if( column_offset == std::nullopt )
		return std::nullopt;

	return *line_linear_pos + *column_offset;
}

std::optional<DocumentRange> SrcLocToDocumentIdentifierRange( const SrcLoc& src_loc, const std::string_view program_text, const LineToLinearPositionIndex& line_to_linear_position_index )
{
	const uint32_t line= src_loc.GetLine();
	if( line >= line_to_linear_position_index.size() )
		return std::nullopt;

	const std::string_view line_text= program_text.substr( line_to_linear_position_index[ line ] );

	const auto utf8_column= Utf32PositionToUtf8Position( line_text, src_loc.GetColumn() );
	if( utf8_column == std::nullopt )
		return std::nullopt;

	const std::optional<TextLinearPosition> utf8_column_end= GetIdentifierEndForPosition( line_text, *utf8_column );
	if( utf8_column_end == std::nullopt )
		return std::nullopt;

	const auto utf16_column    = Utf8PositionToUtf16Position( line_text, *utf8_column     );
	const auto utf16_column_end= Utf8PositionToUtf16Position( line_text, *utf8_column_end );
	if( utf16_column == std::nullopt || utf16_column_end == std::nullopt )
		return std::nullopt;

	return DocumentRange{ { line, *utf16_column }, { line, *utf16_column_end } };
}

} //namespace LangServer

} // namespace U
