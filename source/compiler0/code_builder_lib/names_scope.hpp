#pragma once
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/StringMap.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../../code_builder_lib_common/code_builder_errors.hpp"
#include "value.hpp"

namespace U
{

using Synt::ClassMemberVisibility;

class NamesScope final
{
public:
	NamesScope( std::string name, NamesScope* parent );

	NamesScope( const NamesScope&)= delete;
	NamesScope& operator=( const NamesScope&)= delete;

	bool IsAncestorFor( const NamesScope& other ) const;
	const std::string& GetThisNamespaceName() const;
	void SetThisNamespaceName( std::string name );

	// Get full name (with enclosing namespaces) un human-readable format.
	std::string ToString() const;

	// Returns nullptr, if name already exists in this scope.
	Value* AddName( const std::string& name, Value value );

	// Resolve simple name only in this scope.
	Value* GetThisScopeValue( const std::string& name );
	const Value* GetThisScopeValue( const std::string& name ) const;

	NamesScope* GetParent();
	const NamesScope* GetParent() const;
	NamesScope* GetRoot();
	const NamesScope* GetRoot() const;

	// Store class for namespaces of classes.
	void SetClass( ClassPtr in_class );
	ClassPtr GetClass() const;

	void AddAccessRightsFor( ClassPtr class_, ClassMemberVisibility visibility );
	ClassMemberVisibility GetAccessFor( ClassPtr class_ ) const;
	void CopyAccessRightsFrom( const NamesScope& src );

	static const std::string c_template_args_namespace_name;
	bool IsInsideTemplate() const;

	void SetErrors( CodeBuilderErrorsContainer& errors );
	CodeBuilderErrorsContainer& GetErrors() const;

	template<class Func>
	void ForEachInThisScope( const Func& func )
	{
		++iterating_;
		std::string name;
		name.reserve(max_key_size_);
		for( auto& inserted_name : names_map_ )
		{
			name.assign( inserted_name.getKeyData(), inserted_name.getKeyLength() );
			func( const_cast<const std::string&>(name), inserted_name.second );
		}
		--iterating_;
	}

	template<class Func>
	void ForEachInThisScope( const Func& func ) const
	{
		++iterating_;
		std::string name;
		name.reserve(max_key_size_);
		for( const auto& inserted_name : names_map_ )
		{
			name.assign( inserted_name.getKeyData(), inserted_name.getKeyLength() );
			func( const_cast<const std::string&>(name), inserted_name.second );
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
	std::string name_;
	NamesScope* parent_;

	ClassPtr class_= nullptr;

	llvm::StringMap< Value > names_map_;
	size_t max_key_size_= 0u;

	mutable size_t iterating_= 0u;
	std::unordered_map<ClassPtr, ClassMemberVisibility> access_rights_;

	CodeBuilderErrorsContainer* errors_= nullptr;
};

} // namespace U
