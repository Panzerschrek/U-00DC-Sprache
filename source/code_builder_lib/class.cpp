#include "code_builder_types.hpp"
#include "class.hpp"


namespace U
{

namespace CodeBuilderPrivate
{

//
// Class
//

Class::Class( const ProgramString& in_name, NamesScope* const parent_scope )
	: members( in_name, parent_scope )
{}

Class::~Class()
{}

ClassMemberVisibility Class::GetMemberVisibility( const ProgramString& member_name ) const
{
	const auto it= members_visibility.find( member_name );
	if( it == members_visibility.end() )
		return ClassMemberVisibility::Public;
	return it->second;
}

void Class::SetMemberVisibility( const ProgramString& member_name, const ClassMemberVisibility visibility )
{
	if( visibility == ClassMemberVisibility::Public )
		return;
	members_visibility[member_name]= visibility;
}

} //namespace CodeBuilderPrivate

} // namespace U
