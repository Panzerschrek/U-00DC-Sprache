#include "lex_utils.hpp"

namespace U
{

// Complexity is linear.
std::optional<SrcLoc> GetLexemSrcLocForPosition( const uint32_t line, const uint32_t column, const Lexems& lexems )
{
	// TODO - return none, if position is between lexems.

	const SrcLoc pos_loc( 0, line, column );
	auto it= lexems.begin();
	for( ; it < lexems.end(); ++it )
	{
		// Compare without file index and macro expansion context.
		const SrcLoc lexem_loc( 0, it->src_loc.GetLine(), it->src_loc.GetColumn() );
		if( pos_loc == lexem_loc )
			return lexem_loc;
		if( pos_loc < lexem_loc )
		{
			if( it != lexems.begin() )
				return std::prev(it)->src_loc;
		}
	}

	return std::nullopt;
}

// TODO - clear this ugly piece of shit.
SrcLoc GetLexemEnd( const uint32_t line, const uint32_t column, const Lexems& lexems )
{
	const SrcLoc pos_loc( 0, line, column );
	auto it= lexems.begin();
	for( ; it < lexems.end(); ++it )
	{
		// Compare without file index and macro expansion context.
		const SrcLoc lexem_loc( 0, it->src_loc.GetLine(), it->src_loc.GetColumn() );
		if( pos_loc == lexem_loc )
		{
			// TODO - fix this. This is wrong for anythyng except some identifiers.
			return SrcLoc( 0, line, column + uint32_t(it->text.size()) );
		}
	}

	return pos_loc;
}

} // namespace U
