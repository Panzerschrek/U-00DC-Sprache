#include "../../code_builder_lib_common/source_file_contents_hash.hpp"
#include "funcs_c.hpp"

// If contents of this file changed, source_file_contents_hash_wrapper.uh must be changed too!

using U1_CalculateSourceFileContentsHashCallback= void(*)( U1_UserHandle data, const U1_StringView& hash );

extern "C" void U1_CalculateSourceFileContentsHash(
	const U1_StringView& contents,
	const U1_CalculateSourceFileContentsHashCallback callback,
	const U1_UserHandle callback_data )
{
	const std::string hash= U::CalculateSourceFileContentsHash( std::string_view( contents.data, contents.size ) );
	callback( callback_data, U1_StringView{ hash.data(), hash.size() } );
}
