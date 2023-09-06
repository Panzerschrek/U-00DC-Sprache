#include "../lex_synt_lib_common/assert.hpp"
#include "text_change.hpp"

namespace U
{

namespace LangServer
{

std::optional<uint32_t> MapNewPositionToOldPosition( const TextChangesSequence& changes, uint32_t position )
{
	uint32_t current_position= position;
	for( auto it= changes.rbegin(); it != changes.rend(); ++it ) // Iterate in reverse order.
	{
		const TextChange& change= *it;
		if( current_position < change.range_start )
		{
			// Change was done after current position - perserve it.
			continue;
		}
		const uint32_t new_position_of_change_end= change.range_start + change.new_count;
		if( current_position >= new_position_of_change_end )
		{
			// Change was done before current possition.
			current_position= current_position - new_position_of_change_end + change.range_end;
		}
		else
		{
			// Current position inside change - can't perform mapping.
			return std::nullopt;
		}
	}
	return current_position;
}

std::optional<uint32_t> MapOldPositionToNewPosition( const TextChangesSequence& changes, const uint32_t position )
{
	uint32_t current_position= position;
	for( const TextChange& change : changes )
	{
		U_ASSERT( change.range_start <= change.range_end );
		if( current_position < change.range_start )
		{
			// Leave current position - change was done after it.
		}
		else if( current_position >= change.range_end )
		{
			// Position after change.
			const uint32_t position_relative_change_end= current_position - change.range_end;
			const uint32_t new_position_of_change_end= change.range_start + change.new_count;
			current_position= position_relative_change_end + new_position_of_change_end;
		}
		else
		{
			// This text was replaced with new one - mapping is impossible.
			return std::nullopt;
		}
	}

	return current_position;
}

} // namespace LangServer

} // namespace U
