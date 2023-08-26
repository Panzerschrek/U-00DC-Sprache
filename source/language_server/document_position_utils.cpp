#include "document_position_utils.hpp"

namespace U
{

namespace LangServer
{

DocumentPosition SrcLocToDocumentPosition( const SrcLoc& src_loc )
{
	return DocumentPosition{ src_loc.GetLine(), src_loc.GetColumn() };
}

SrcLoc DocumentPositionToSrcLoc( const DocumentPosition& position )
{
	return SrcLoc( 0, position.line, position.column );
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
