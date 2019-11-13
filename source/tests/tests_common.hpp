#pragma once
#include <string>

#include "../lex_synt_lib/push_disable_boost_warnings.hpp"
#include <boost/predef/detail/endian_compat.h>
#include "../lex_synt_lib/pop_boost_warnings.hpp"

namespace U
{

const std::string GetTestsDataLayout()
{
	std::string result;

#ifdef BOOST_BIG_ENDIAN
		result+= "E";
	else
#else
		result+= "e";
#endif

	const bool is_32_bit= sizeof(void*) <= 4u;
	result+= is_32_bit ? "-p:32:32" : "-p:64:64";
	result+= is_32_bit ? "-n8:16:32" : "-n8:16:32:64";
	result+= "-i8:8-i16:16-i32:32-i64:64";
	result+= "-f32:32-f64:64";
	result+= "-S128";

	return result;
}

} // namespace U
