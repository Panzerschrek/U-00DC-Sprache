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
	NamesScope( std::string name, const NamesScope* parent );

	NamesScope( const NamesScope&)= delete;
	NamesScope& operator=( const NamesScope&)= delete;

	const std::string& GetThisNamespaceName() const;

	// Get full name (with enclosing namespaces) un human-readable format.
	std::string ToString() const;

	// Returns nullptr, if name already exists in this scope.
	NamesScopeValue* AddName( std::string_view name, NamesScopeValue value );

	// Resolve simple name only in this scope.
	NamesScopeValue* GetThisScopeValue( std::string_view name );
	const NamesScopeValue* GetThisScopeValue( std::string_view name ) const;

	const NamesScope* GetParent() const;
	const NamesScope* GetRoot() const;

	const NamesScope* GetClosestNamedSpaceOrRoot() const;

	// Store class for namespaces of classes.
	void SetClass( ClassPtr in_class );
	ClassPtr GetClass() const;

	void AddAccessRightsFor( ClassPtr class_, ClassMemberVisibility visibility );
	ClassMemberVisibility GetAccessFor( ClassPtr class_ ) const;
	void CopyAccessRightsFrom( const NamesScope& src );

	static const std::string_view c_template_args_namespace_name;
	bool IsInsideTemplate() const;

	void SetErrors( std::shared_ptr<CodeBuilderErrorsContainer> errors );
	CodeBuilderErrorsContainer& GetErrors() const;

	template<class Func>
	void ForEachInThisScope( const Func& func )
	{
		++iterating_;
		for( auto& inserted_name : names_map_ )
			func( std::string_view( inserted_name.getKeyData(), inserted_name.getKeyLength() ), inserted_name.second );
		--iterating_;
	}

	template<class Func>
	void ForEachInThisScope( const Func& func ) const
	{
		++iterating_;
		for( const auto& inserted_name : names_map_ )
			func( std::string_view( inserted_name.getKeyData(), inserted_name.getKeyLength() ), inserted_name.second );
		--iterating_;
	}

	template<class Func>
	void ForEachValueInThisScope( const Func& func )
	{
		++iterating_;
		for( auto& inserted_name : names_map_ )
			func( inserted_name.second.value );
		--iterating_;
	}

	template<class Func>
	void ForEachValueInThisScope( const Func& func ) const
	{
		++iterating_;
		for( const auto& inserted_name : names_map_ )
			func( inserted_name.second.value );
		--iterating_;
	}

private:
	const std::string name_;
	const NamesScope* const parent_;

	ClassPtr class_= nullptr;

	llvm::StringMap< NamesScopeValue > names_map_;

	mutable size_t iterating_= 0u;
	std::unordered_map<ClassPtr, ClassMemberVisibility> access_rights_;

	std::shared_ptr<CodeBuilderErrorsContainer> errors_;
};

} // namespace U
