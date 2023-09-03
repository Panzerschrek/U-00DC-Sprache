#include "document_position_utils.hpp"
#include "program_string.hpp"

namespace U
{

namespace LangServer
{

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

std::optional<TextLinearPosition> GetUtf8LineStartPosition( const std::string_view text, const uint32_t line )
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

std::optional<TextLinearPosition> Utf16PositionToUtf8Position( const std::string_view text, const TextLinearPosition position )
{
	// Extract from UTF-8 string code points and count number of UTF-16 words.
	TextLinearPosition current_utf16_position= 0;
	const char* s= text.data();
	const char* const s_end= s + text.size();
	while( s < s_end && current_utf16_position < position )
	{
		const sprache_char code_point= ReadNextUTF8Char( s, s_end );
		if( code_point <= 0xFFFFu )
		{
			// Code points in range [0;0xFFFF] are encoded as singe UTF-16 word.
			++current_utf16_position;
		}
		else
		{
			// Code points abowe 0xFFFF are encoded as two UTF-16 words.
			current_utf16_position += 2;
		}
	}

	if( current_utf16_position != position )
	{
		// Something went wrong. Maybe utf-16 position is too big?
		return std::nullopt;
	}

	return TextLinearPosition( s - text.data() );
}

std::optional<TextLinearPosition> Utf8PositionToUtf16Position( const std::string_view text, const TextLinearPosition position )
{
	TextLinearPosition current_utf16_position= 0;
	const char* s= text.data();
	const char* const s_end= s + text.size();
	while( s < s_end && TextLinearPosition(s - text.data()) < position )
	{
		const sprache_char code_point= ReadNextUTF8Char( s, s_end );
		if( code_point <= 0xFFFFu )
		{
			// Code points in range [0;0xFFFF] are encoded as singe UTF-16 word.
			++current_utf16_position;
		}
		else
		{
			// Code points abowe 0xFFFF are encoded as two UTF-16 words.
			current_utf16_position += 2;
		}
	}

	if( TextLinearPosition(s - text.data()) != position )
		return std::nullopt;

	return current_utf16_position;
}

std::optional<TextLinearPosition> Utf8PositionToUtf32Position( const std::string_view text, const TextLinearPosition position )
{
	TextLinearPosition current_utf32_position= 0;
	const char* s= text.data();
	const char* const s_end= s + text.size();
	while( s < s_end && TextLinearPosition(s - text.data()) < position )
	{
		ReadNextUTF8Char( s, s_end );
		++current_utf32_position;
	}

	if( TextLinearPosition(s - text.data()) != position )
		return std::nullopt;

	return current_utf32_position;
}

std::optional<TextLinearPosition> Utf32PositionToUtf8Position( std::string_view text, const TextLinearPosition position )
{
	TextLinearPosition current_utf32_position= 0;
	const char* s= text.data();
	const char* const s_end= s + text.size();
	while( s < s_end && current_utf32_position < position )
	{
		ReadNextUTF8Char( s, s_end );
		++current_utf32_position;
	}

	if( current_utf32_position != position )
		return std::nullopt;

	return TextLinearPosition(s - text.data());
}

std::optional<TextLinearPosition> Utf32PositionToUtf16Position( const std::string_view text, const TextLinearPosition position )
{
	TextLinearPosition current_utf32_position= 0;
	TextLinearPosition current_utf16_position= 0;
	const char* s= text.data();
	const char* const s_end= s + text.size();
	while( s < s_end && current_utf32_position < position )
	{
		const sprache_char code_point= ReadNextUTF8Char( s, s_end );
		if( code_point <= 0xFFFFu )
		{
			// Code points in range [0;0xFFFF] are encoded as singe UTF-16 word.
			++current_utf16_position;
		}
		else
		{
			// Code points abowe 0xFFFF are encoded as two UTF-16 words.
			current_utf16_position += 2;
		}
		++current_utf32_position;
	}

	if( current_utf32_position != position )
		return std::nullopt;

	return current_utf16_position;
}

TextLinearPosition GetLineStartUtf8Position( const std::string_view text, const TextLinearPosition position )
{
	TextLinearPosition line_start_position= position;
	while( line_start_position >= 1 )
	{
		if( text[ line_start_position - 1 ] == '\n' )
			break;
		--line_start_position;
	}

	return line_start_position;
}

std::optional<DocumentRange> SrcLocToDocumentIdentifierRange( const SrcLoc& src_loc, const std::string_view program_text, const LineToLinearPositionIndex& line_to_linear_position_index )
{
	const uint32_t line= src_loc.GetLine();
	if( line >= line_to_linear_position_index.size() )
		return std::nullopt;

	const std::string_view line_text= program_text.substr( line_to_linear_position_index[ line ] );

	const std::optional<TextLinearPosition> utf8_column= Utf32PositionToUtf8Position( line_text, src_loc.GetColumn() );
	if( utf8_column == std::nullopt )
		return std::nullopt;

	const std::optional<TextLinearPosition> utf8_column_end= GetIdentifierEndForPosition( line_text, *utf8_column );
	if( utf8_column_end == std::nullopt )
		return std::nullopt;

	const std::optional<TextLinearPosition> utf16_column    = Utf8PositionToUtf16Position( line_text, *utf8_column     );
	const std::optional<TextLinearPosition> utf16_column_end= Utf8PositionToUtf16Position( line_text, *utf8_column_end );
	if( utf16_column == std::nullopt || utf16_column_end == std::nullopt )
		return std::nullopt;

	return DocumentRange{ { line, *utf16_column }, { line, *utf16_column_end } };
}

std::optional<DocumentPosition> GetIdentifierEndPosition( const DocumentPosition& position, const std::string_view program_text, const LineToLinearPositionIndex& line_to_linear_position_index )
{
	if( position.line >= line_to_linear_position_index.size() )
		return std::nullopt;
	const std::string_view line_text= program_text.substr( line_to_linear_position_index[ position.line ] );

	const auto utf8_column= Utf16PositionToUtf8Position( line_text, position.character );
	if( utf8_column == std::nullopt )
		return std::nullopt;

	const auto utf8_column_corrected= GetIdentifierEndForPosition( line_text, *utf8_column );
	if( utf8_column_corrected == std::nullopt )
		return std::nullopt;

	const auto utf16_column_corrected= Utf8PositionToUtf16Position( line_text, *utf8_column_corrected );
	if( utf16_column_corrected == std::nullopt )
		return std::nullopt;

	return DocumentPosition{ position.line, *utf16_column_corrected };
}

} //namespace LangServer

} // namespace U
