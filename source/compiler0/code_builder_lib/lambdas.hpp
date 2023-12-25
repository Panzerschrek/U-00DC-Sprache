#pragma once
#include <unordered_set>
#include "names_scope.hpp"

namespace U
{

struct LambdaKey
{
	// Store here scope in order to distinguish lambdas in different template instantiations.
	NamesScope* parent_scope= nullptr;
	// SrcLoc of lambda syntax element.
	SrcLoc src_loc;

	size_t Hash() const;
};

bool operator==( const LambdaKey& l, const LambdaKey& r );
inline bool operator!=( const LambdaKey& l, const LambdaKey& r ) { return !( l == r ); }

struct LambdaKeyHasher
{
	size_t operator()( const LambdaKey& k ) const { return k.Hash(); }
};

struct LambdaClassData
{
	struct Capture
	{
		std::string captured_variable_name;
		ClassFieldPtr field;
	};

	std::vector<Capture> captures;
};

struct LambdaPreprocessingContext
{
	struct CapturedVariableData
	{
		VariablePtr source_variable;

		// Newly-created nodes inside the lambda.
		VariablePtr variable_node;
		VariablePtr reference_node;
		std::vector<VariablePtr> accessible_variables;
	};

	using ReferenceLink= std::variant< FunctionType::ParamReference, VariablePtr >;

	struct ReferencePollution
	{
		ReferenceLink dst;
		ReferenceLink src;
	};

public:

	// Inputs.
	std::unordered_set<VariablePtr> external_variables;
	ValueType lambda_this_value_type= ValueType::ReferenceImut;
	bool capture_by_value= false;

	// Outputs.
	std::unordered_map<std::string, CapturedVariableData> captured_external_variables;

	std::set<FunctionType::ParamReference> return_references;
	std::vector<std::set<FunctionType::ParamReference>> return_inner_references;
	std::vector<ReferencePollution> references_pollution;

	// Contains set of variables of the preprocessed lambda.
	std::set<VariablePtr> captured_variables_return_references;
	std::vector<std::set<VariablePtr>> captured_variables_return_inner_references;
};

} // namespace U
