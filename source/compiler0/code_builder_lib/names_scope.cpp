#include "../../lex_synt_lib_common/assert.hpp"
#include "class.hpp"
#include "template_signature_param.hpp"
#include "enum.hpp"
#include "names_scope.hpp"

namespace U
{

NamesScope::NamesScope( std::string name, NamesScope* const parent )
	: name_(std::move(name) )
	, parent_(parent)
{}

const std::string& NamesScope::GetThisNamespaceName() const
{
	return name_;
}

std::string NamesScope::ToString() const
{
	if( parent_ == nullptr ) // Global namespace has no name.
		return "";
	if( parent_->parent_ == nullptr )
		return name_;

	// Process template class namespaces specially.
	if( class_ != nullptr && std::holds_alternative<Class::BaseTemplate>(class_->generated_class_data) )
		return Type(class_).ToString();

	return parent_->ToString() + "::" + name_;
}

NamesScopeValue* NamesScope::AddName( const std::string_view name, NamesScopeValue value )
{
	U_ASSERT( iterating_ == 0u );
	auto it_bool_pair= names_map_.insert( std::make_pair( llvm::StringRef(name), std::move( value ) ) );

	if( it_bool_pair.second )
		return &it_bool_pair.first->second;

	return nullptr;
}

NamesScopeValue* NamesScope::GetThisScopeValue( const std::string_view name )
{
	const auto it= names_map_.find( name );
	if( it != names_map_.end() )
		return &it->second;
	return nullptr;
}

const NamesScopeValue* NamesScope::GetThisScopeValue( const std::string_view name ) const
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

NamesScope* NamesScope::GetClosestNamedSpaceOrRoot()
{
	NamesScope* current= this;
	while( current->name_.empty() )
	{
		if( current->parent_ == nullptr )
			return current; // Reached the root.
		current= current->parent_;
	}
	return current;
}

void NamesScope::SetClass(const ClassPtr in_class )
{
	this->class_= in_class;
}

ClassPtr NamesScope::GetClass() const
{
	return class_;
}

void NamesScope::AddAccessRightsFor( const ClassPtr class_, const ClassMemberVisibility visibility )
{
	access_rights_[class_]= visibility;
}

ClassMemberVisibility NamesScope::GetAccessFor( const ClassPtr class_ ) const
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

const std::string_view NamesScope::c_template_args_namespace_name= "_tp_ns";

bool NamesScope::IsInsideTemplate() const
{
	if( name_ == Class::c_template_class_name || name_ == c_template_args_namespace_name )
		return true;
	if( parent_ != nullptr )
		return parent_->IsInsideTemplate();
	return false;
}

void NamesScope::SetErrors( std::shared_ptr<CodeBuilderErrorsContainer> errors )
{
	errors_= std::move(errors);
}

CodeBuilderErrorsContainer& NamesScope::GetErrors() const
{
	if( errors_ != nullptr )
		return *errors_;
	return parent_->GetErrors();
}

} // namespace U
