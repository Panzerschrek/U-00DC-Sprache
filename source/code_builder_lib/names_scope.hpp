#pragma once
#include "value.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

using Synt::ClassMemberVisibility;

class NamesScope final
{
public:
	NamesScope( ProgramString name, NamesScope* parent );

	NamesScope( const NamesScope&)= delete;
	NamesScope& operator=( const NamesScope&)= delete;

	bool IsAncestorFor( const NamesScope& other ) const;
	const ProgramString& GetThisNamespaceName() const;
	void SetThisNamespaceName( ProgramString name );

	// Get full name (with enclosing namespaces) un human-readable format.
	ProgramString ToString() const;

	// Returns nullptr, if name already exists in this scope.
	Value* AddName( const ProgramString& name, Value value );

	// Resolve simple name only in this scope.
	Value* GetThisScopeValue( const ProgramString& name );
	const Value* GetThisScopeValue( const ProgramString& name ) const;

	NamesScope* GetParent();
	const NamesScope* GetParent() const;
	NamesScope* GetRoot();
	const NamesScope* GetRoot() const;
	void SetParent( NamesScope* parent );

	void AddAccessRightsFor( const ClassProxyPtr& class_, ClassMemberVisibility visibility );
	ClassMemberVisibility GetAccessFor( const ClassProxyPtr& class_ ) const;
	void CopyAccessRightsFrom( const NamesScope& src );

	void SetErrors( CodeBuilderErrorsContainer& errors );
	CodeBuilderErrorsContainer& GetErrors() const;

	template<class Func>
	void ForEachInThisScope( const Func& func )
	{
		++iterating_;
		ProgramString name;
		name.reserve(max_key_size_);
		for( auto& inserted_name : names_map_ )
		{
			name.assign( inserted_name.getKeyData(), inserted_name.getKeyLength() );
			func( const_cast<const ProgramString&>(name), inserted_name.second );
		}
		--iterating_;
	}

	template<class Func>
	void ForEachInThisScope( const Func& func ) const
	{
		++iterating_;
		ProgramString name;
		name.reserve(max_key_size_);
		for( const auto& inserted_name : names_map_ )
		{
			name.assign( inserted_name.getKeyData(), inserted_name.getKeyLength() );
			func( const_cast<const ProgramString&>(name), inserted_name.second );
		}
		--iterating_;
	}

	template<class Func>
	void ForEachValueInThisScope( const Func& func )
	{
		++iterating_;
		for( auto& inserted_name : names_map_ )
			func( inserted_name.second );
		--iterating_;
	}

	template<class Func>
	void ForEachValueInThisScope( const Func& func ) const
	{
		++iterating_;
		for( const auto& inserted_name : names_map_ )
			func( inserted_name.second );
		--iterating_;
	}

private:
	ProgramString name_;
	NamesScope* parent_;

	// Use StringMap here, with "const char*" key.
	// interpritate ProgramString bytes as chars.
	// TODO - maybe replace "ProgramString" with UTF-8 std::string?
	llvm::StringMap< Value > names_map_;
	size_t max_key_size_= 0u;

	mutable size_t iterating_= 0u;
	std::unordered_map<ClassProxyPtr, ClassMemberVisibility> access_rights_;

	CodeBuilderErrorsContainer* errors_= nullptr;
};


} //namespace CodeBuilderPrivate

} // namespace U
