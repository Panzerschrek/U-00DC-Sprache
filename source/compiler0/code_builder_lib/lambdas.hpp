#pragma once
#include <unordered_map>
#include <unordered_set>
#include "names_scope.hpp"
#include "template_types.hpp"

namespace U
{

struct LambdaKey
{
	// Store here scope in order to distinguish lambdas in different template instantiations.
	NamesScope* parent_scope= nullptr;
	// SrcLoc of lambda syntax element.
	SrcLoc src_loc;

	// We need to create distinct lambdas on each iteration of tuple-for.
	std::vector<uint32_t> tuple_for_indices;

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

	std::vector<Capture> captures; // Should be in fields order.
	std::vector<TemplateArg> template_args; // For some lambdas (like in template functions).
};

struct LambdaPreprocessingContext
{
	struct ExplicitCapture
	{
		bool capture_by_reference= false;
	};

	using ExplicitCaptures= std::unordered_map<VariablePtr, ExplicitCapture>;

	struct CapturedVariableData
	{
		// Variable from parent function.
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

	// Inputs (filled before lambda preprocessing).
	LambdaPreprocessingContext* parent= nullptr;
	std::unordered_set<VariablePtr> external_variables;
	std::optional<ExplicitCaptures> explicit_captures; // If none - all variables are allowed for capture.
	bool capture_by_reference= false;
	bool lambda_this_is_mutable= false;

	// Outputs (filled during lambda preprocessing).

	std::unordered_map<std::string, CapturedVariableData> captured_external_variables;

	std::vector<ReferencePollution> references_pollution;

	// Contains set of variables of the preprocessed lambda.
	std::set<VariablePtr> captured_variables_return_references;
	std::vector<std::set<VariablePtr>> captured_variables_return_inner_references;

	bool has_preprocessing_errors= false;
};

} // namespace U
