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

DocumentPosition SrcLocToDocumentPosition( const SrcLoc& src_loc )
{
	return DocumentPosition{ src_loc.GetLine(), src_loc.GetColumn() };
}

SrcLoc DocumentPositionToSrcLoc( const DocumentPosition& position )
{
	return SrcLoc( 0, position.line, position.character );
}

std::optional<SrcLoc> GetIdentifierStartSrcLoc( const SrcLoc& src_loc, const std::string_view program_text, const LineToLinearPositionIndex& line_to_linear_position_index )
{
	const uint32_t line= src_loc.GetLine();
	if( line >= line_to_linear_position_index.size() )
		return std::nullopt;

	const TextLinearPosition linear_position= line_to_linear_position_index[ line ] + src_loc.GetColumn();
	const std::optional<TextLinearPosition> linear_position_corrected= GetIdentifierStartForPosition( program_text, linear_position );
	if( linear_position_corrected == std::nullopt )
		return std::nullopt;

	SrcLoc result= LinearPositionToSrcLoc( line_to_linear_position_index, *linear_position_corrected );
	result.SetFileIndex( src_loc.GetFileIndex() );
	result.SetMacroExpansionIndex( src_loc.GetMacroExpansionIndex() );
	return result;
}

std::optional<SrcLoc> GetIdentifierEndSrcLoc( const SrcLoc& src_loc, const std::string_view program_text, const LineToLinearPositionIndex& line_to_linear_position_index )
{
	const uint32_t line= src_loc.GetLine();
	if( line >= line_to_linear_position_index.size() )
		return std::nullopt;

	const TextLinearPosition linear_position= line_to_linear_position_index[ line ] + src_loc.GetColumn();
	const std::optional<TextLinearPosition> linear_position_corrected= GetIdentifierEndForPosition( program_text, linear_position );
	if( linear_position_corrected == std::nullopt )
		return std::nullopt;

	SrcLoc result= LinearPositionToSrcLoc( line_to_linear_position_index, *linear_position_corrected );
	result.SetFileIndex( src_loc.GetFileIndex() );
	result.SetMacroExpansionIndex( src_loc.GetMacroExpansionIndex() );
	return result;
}

} //namespace LangServer

} // namespace U
