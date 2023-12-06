#pragma once
#include "value.hpp"

namespace U
{

struct TypeinfoListElement
{
	std::string name_for_ordering;
	llvm::Constant* initializer= nullptr;
	ClassPtr type= nullptr;
};

struct TypeinfoCacheElement
{
	VariableMutPtr variable; // Variable - result of typeinfo operator call.

	// Various typeinfo lists. They are created lazily.
	VariablePtr elements_list= nullptr; // For enums and tuples.
	VariablePtr fields_list= nullptr;
	VariablePtr types_list= nullptr;
	VariablePtr functions_list= nullptr;
	VariablePtr parents_list= nullptr;
	VariablePtr params_list= nullptr;
};

struct TypeinfoClassDescription
{
	Type source_type; // Type for which this typeinfo class was generated.
	bool is_main_class= false; // Main class - a class produced by typeinfo operator. Non-main class - some other typeinfo class, like list node class.
};

} // namespace U
