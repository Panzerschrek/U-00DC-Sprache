#include "class.hpp"


namespace U
{

namespace CodeBuilderPrivate
{

//
// Class
//

const std::string Class::c_template_class_name= "_";

Class::Class( std::string in_name, NamesScope* const parent_scope )
	: members( std::move(in_name), parent_scope )
{}

ClassMemberVisibility Class::GetMemberVisibility( const std::string& member_name ) const
{
	const auto it= members_visibility.find( member_name );
	if( it == members_visibility.end() )
		return ClassMemberVisibility::Public;
	return it->second;
}

void Class::SetMemberVisibility( const std::string& member_name, const ClassMemberVisibility visibility )
{
	if( visibility == ClassMemberVisibility::Public )
		return;
	members_visibility[member_name]= visibility;
}

} //namespace CodeBuilderPrivate

} // namespace U
