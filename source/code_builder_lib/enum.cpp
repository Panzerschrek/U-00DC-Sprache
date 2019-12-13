#include "class.hpp"
#include "enum.hpp"


namespace U
{

namespace CodeBuilderPrivate
{

Enum::Enum( std::string in_name, NamesScope* const parent_scope )
	: members( std::move(in_name), parent_scope )
{}

} //namespace CodeBuilderPrivate

} // namespace U
