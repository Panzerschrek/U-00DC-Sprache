#include "document_position_utils.hpp"
#include "program_string.hpp"

namespace U
{

namespace LangServer
{

std::optional<TextLinearPosition> DocumentPositionToLinearPosition( const DocumentPosition& pos, const std::string_view text )
{
	// Find linear position for given line.
	uint32_t line= 1; // Count lines from one.
	TextLinearPosition linear_pos= 0;
	while( linear_pos < text.size() )
	{
		if( line == pos.line )
			break; // Found target line.

		if( text[linear_pos] == '\n' )
			++line;
		++linear_pos;
	}

	if( line != pos.line )
		return std::nullopt;

	// Iterate over text starting from line start. Extract from UTF-8 string code points and count number of UTF-16 words.
	uint32_t column= 0;
	const char* const char_pos_start= text.data() + linear_pos;
	const char* char_pos= char_pos_start;
	const char* const char_end= text.data() + text.size();
	while( column < pos.character && char_pos < char_end )
	{
		const sprache_char code_point= ReadNextUTF8Char( char_pos, char_end );
		if( code_point <= 0xFFFFu )
		{
			// Code points in range [0;0xFFFF] are encoded as singe UTF-16 word.
			++column;
		}
		else
		{
			// Code points abowe 0xFFFF are encoded as two UTF-16 words.
			column += 2;
		}
	}

	return linear_pos + uint32_t( char_pos - char_pos_start );
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
