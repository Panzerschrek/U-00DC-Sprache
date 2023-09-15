#include "../lex_synt_lib_common/assert.hpp"
#include "document_position_utils.hpp"
#include "program_string.hpp"

namespace U
{

namespace LangServer
{

std::optional<TextLinearPosition> DocumentPositionToLinearPosition( const DocumentPosition& pos, const std::string_view text, const LineToLinearPositionIndex& line_to_linear_position_index )
{
	if( pos.line >= line_to_linear_position_index.size() )
		return std::nullopt;

	const TextLinearPosition line_linear_pos= line_to_linear_position_index[ pos.line ];

	const auto column_offset= Utf16PositionToUtf8Position( text.substr( line_linear_pos ), pos.character );
	if( column_offset == std::nullopt )
		return std::nullopt;

	return line_linear_pos + *column_offset;
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
