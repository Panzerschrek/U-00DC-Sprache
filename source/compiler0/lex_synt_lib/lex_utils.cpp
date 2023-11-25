#include "lex_utils.hpp"

namespace U
{

// Complexity is linear.
const Lexem* GetLexemForPosition( const uint32_t line, const uint32_t column, const Lexems& lexems )
{
	// TODO - return none, if position is between lexems.

	const SrcLoc pos_loc( 0, line, column );
	auto it= lexems.begin();
	for( ; it < lexems.end(); ++it )
	{
		// Compare without file index and macro expansion context.
		const SrcLoc lexem_loc( 0, it->src_loc.GetLine(), it->src_loc.GetColumn() );
		if( pos_loc == lexem_loc )
			return &*it;
		if( pos_loc < lexem_loc )
		{
			if( it != lexems.begin() )
				return &*std::prev(it);
		}
	}

	return nullptr;
}

} // namespace U
