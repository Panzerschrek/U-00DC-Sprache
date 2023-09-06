#pragma once
#include <optional>
#include <vector>

namespace U
{

namespace LangServer
{

// Represent change in text - replacing of range with new one.
struct TextChange
{
	uint32_t range_start= 0;
	uint32_t range_end= 0;
	uint32_t new_count= 0;
};

// Changes in chronological order.
using TextChangesSequence= std::vector<TextChange>;

// Mapping functions. Return nullopt, if can't perform mapping (position in newly inserted text, deleted text).
std::optional<uint32_t> MapNewPositionToOldPosition( const TextChangesSequence& changes, uint32_t position );
std::optional<uint32_t> MapOldPositionToNewPosition( const TextChangesSequence& changes, uint32_t position );

} // namespace LangServer

} // namespace U
