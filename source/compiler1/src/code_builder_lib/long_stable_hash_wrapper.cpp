#include "../../../code_builder_lib_common/long_stable_hash.hpp"

// If contents of this file changed, long_stable_hash_wrapper.uh must be changed too!

using UserHandle= size_t;

using U1_CalculateLongStableHashCallback= void(*)( UserHandle data, const char* hash_data, size_t hash_size );

extern "C" void U1_CalculateLongStableHash(
	const char* const data,
	const size_t size,
	const U1_CalculateLongStableHashCallback callback,
	const UserHandle callback_data )
{
	const std::string hash= U::CalculateLongStableHash( std::string_view( data, size ) );
	callback( callback_data, hash.data(), hash.size() );
}
