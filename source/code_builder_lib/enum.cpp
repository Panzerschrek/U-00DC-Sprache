#include "class.hpp"
#include "enum.hpp"


namespace U
{

namespace CodeBuilderPrivate
{

Enum::Enum( const std::string& in_name, NamesScope* const parent_scope )
	: members( in_name, parent_scope )
{}

} //namespace CodeBuilderPrivate

} // namespace U
