#include "../lex_synt_lib/assert.hpp"
#include "class.hpp"
#include "enum.hpp"
#include "names_scope.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

NamesScope::NamesScope( ProgramString name, NamesScope* const parent )
	: name_(std::move(name) )
	, parent_(parent)
{}

bool NamesScope::IsAncestorFor( const NamesScope& other ) const
{
	const NamesScope* n= other.parent_;
	while( n != nullptr )
	{
		if( this == n )
			return true;
		n= n->parent_;
	}

	return false;
}

const ProgramString& NamesScope::GetThisNamespaceName() const
{
	return name_;
}

void NamesScope::SetThisNamespaceName( ProgramString name )
{
	name_= std::move(name);
}

ProgramString NamesScope::ToString() const
{
	if( parent_ == nullptr ) // Global namespace have no name.
		return "";
	if( parent_->parent_ == nullptr )
		return name_;
	return parent_->ToString() + "::" + name_;
}

Value* NamesScope::AddName(
	const ProgramString& name,
	Value value )
{
	U_ASSERT( iterating_ == 0u );
	auto it_bool_pair=
		names_map_.insert(
			std::make_pair(
				name,
				std::move( value ) ) );

	if( it_bool_pair.second )
	{
		max_key_size_= std::max( max_key_size_, name.size() );
		return &it_bool_pair.first->second;
	}

	return nullptr;
}

Value* NamesScope::GetThisScopeValue( const ProgramString& name )
{
	const auto it= names_map_.find( name );
	if( it != names_map_.end() )
		return &it->second;
	return nullptr;
}

const Value* NamesScope::GetThisScopeValue( const ProgramString& name ) const
{
	return const_cast<NamesScope*>(this)->GetThisScopeValue( name );
}

NamesScope* NamesScope::GetParent()
{
	return parent_;
}

const NamesScope* NamesScope::GetParent() const
{
	return parent_;
}

NamesScope* NamesScope::GetRoot()
{
	NamesScope* root= this;
	while( root->parent_ != nullptr )
		root= root->parent_;
	return root;
}

const NamesScope* NamesScope::GetRoot() const
{
	const NamesScope* root= this;
	while( root->parent_ != nullptr )
		root= root->parent_;
	return root;
}

void NamesScope::SetParent( NamesScope* const parent )
{
	parent_= parent;
}

void NamesScope::AddAccessRightsFor( const ClassProxyPtr& class_, const ClassMemberVisibility visibility )
{
	access_rights_[class_]= visibility;
}

ClassMemberVisibility NamesScope::GetAccessFor( const ClassProxyPtr& class_ ) const
{
	const auto it= access_rights_.find(class_);
	const auto this_rights= it == access_rights_.end() ? ClassMemberVisibility::Public : it->second;
	const auto parent_rights= parent_ == nullptr ? ClassMemberVisibility::Public : parent_->GetAccessFor( class_ );
	return std::max( this_rights, parent_rights );
}

void NamesScope::CopyAccessRightsFrom( const NamesScope& src )
{
	access_rights_= src.access_rights_;
}

void NamesScope::SetErrors( CodeBuilderErrorsContainer& errors )
{
	errors_= &errors;
}

CodeBuilderErrorsContainer& NamesScope::GetErrors() const
{
	if( errors_ != nullptr )
		return *errors_;
	return parent_->GetErrors();
}

} //namespace CodeBuilderPrivate

} // namespace U
