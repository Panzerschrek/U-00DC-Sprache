#include "class.hpp"
#include "template_signature_param.hpp"
#include "enum.hpp"


namespace U
{

Enum::Enum( std::string in_name, NamesScope* const parent_scope )
	: members( std::move(in_name), parent_scope )
{}

} // namespace U
