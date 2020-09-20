#include <cstdint>

extern "C" uint32_t U1_ReferencesGraphGetNextUniqueId()
{
	static uint32_t next_id= 1;
	const auto result= next_id;
	++next_id;
	return result;
}
