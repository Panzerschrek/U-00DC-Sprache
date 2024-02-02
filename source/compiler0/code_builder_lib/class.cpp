#include "../../code_builder_lib_common/string_ref.hpp"
#include "template_signature_param.hpp"
#include "class.hpp"

namespace U
{

const std::string_view Class::c_template_class_name= "_";

Class::Class( std::string in_name, const NamesScope* const parent_scope )
	: members( std::make_shared<NamesScope>( std::move(in_name), parent_scope ) )
	, members_initial(members)
{}

ClassMemberVisibility Class::GetMemberVisibility( const std::string_view member_name ) const
{
	const auto it= members_visibility.find( StringViewToStringRef(member_name) );
	if( it == members_visibility.end() )
		return ClassMemberVisibility::Public;
	return it->second;
}

void Class::SetMemberVisibility( const std::string_view member_name, const ClassMemberVisibility visibility )
{
	if( visibility == ClassMemberVisibility::Public )
		return;
	members_visibility[ StringViewToStringRef(member_name) ]= visibility;
}

bool Class::HaveAncestor( const ClassPtr class_ ) const
{
	for( const auto& parent : parents )
	{
		if( parent.class_ == class_ )
			return true;
		if( parent.class_->HaveAncestor( class_ ) )
			return true;
	}
	return false;
}

} // namespace U
