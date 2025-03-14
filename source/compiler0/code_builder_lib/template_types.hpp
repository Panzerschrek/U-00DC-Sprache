#pragma once

#include "../lex_synt_lib/syntax_elements.hpp"
#include "names_scope.hpp"
#include "template_signature_param.hpp"

namespace U
{

struct TemplateVariableArg
{
	Type type;
	llvm::Constant* constexpr_value= nullptr;

	TemplateVariableArg()= default;
	explicit TemplateVariableArg( const Variable& v )
		: type( v.type ), constexpr_value( v.constexpr_value )
	{
		// U_ASSERT( constexpr_value != nullptr );
	}

	TemplateVariableArg( const TemplateVariableArg& )= default;
	TemplateVariableArg( TemplateVariableArg&& )= default;
	TemplateVariableArg& operator=( const TemplateVariableArg& )= default;
	TemplateVariableArg& operator=( TemplateVariableArg&& )= default;
};

bool operator==( const TemplateVariableArg& l, const TemplateVariableArg& r );
inline bool operator!=( const TemplateVariableArg& l, const TemplateVariableArg& r ) { return !(l == r); }

using TemplateArg= std::variant< TemplateVariableArg, Type, TypeTemplatePtr >;
using TemplateArgs= llvm::SmallVector<TemplateArg, 2>;

struct TemplateParameter
{
	struct TypeParamData{};

	struct TypeTemplateParamData{};

	struct VariableParamData
	{
		// Type of variable param.
		TemplateSignatureParam type;
	};

	SrcLoc src_loc;
	std::string name;
	std::variant<TypeParamData, TypeTemplateParamData, VariableParamData> kind_data;
};

struct TemplateBase
{
	virtual ~TemplateBase()= default;

	std::vector<TemplateParameter> template_params;
	std::vector<TemplateSignatureParam> signature_params; // Function params for function templates.

	// NamesScope, where defined. NOT changed after import or inheritance.
	NamesScope* parent_namespace= nullptr;

	SrcLoc src_loc;
};

struct TypeTemplate final : TemplateBase
{
	size_t first_optional_signature_param= ~0u;

	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::TypeTemplate* syntax_element= nullptr;

	// Set to "true" if this type template was instantiated at least once.
	mutable bool used= false;

	// Convert type template name to human-readable format.
	std::string ToString() const;
};

struct FunctionTemplate final : public TemplateBase
{
	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::FunctionTemplate* syntax_element= nullptr;

	ClassPtr base_class= nullptr;

	// In case of manual parameters specifying, like foo</A, B, C/> we create new template and store known arguments and reference to base template.
	TemplateArgs known_template_args;
	FunctionTemplatePtr parent;
};

struct TemplateKey
{
	// Type or function template.
	std::shared_ptr<const TemplateBase> template_;

	// Signature args for type templates, template args for function templates.
	TemplateArgs args;

	size_t Hash() const;
};

bool operator==( const TemplateKey& l, const TemplateKey& r );
inline bool operator!=( const TemplateKey& l, const TemplateKey& r ) { return !( l == r ); }

struct TemplateKeyHasher
{
	size_t operator()( const TemplateKey& k ) const { return k.Hash(); }
};

struct ParameterizedFunctionTemplateKey
{
	OverloadedFunctionsSetConstPtr functions_set;
	TemplateArgs args;

	size_t Hash() const;
};

bool operator==( const ParameterizedFunctionTemplateKey& l, const ParameterizedFunctionTemplateKey& r );
inline bool operator!=( const ParameterizedFunctionTemplateKey& l, const ParameterizedFunctionTemplateKey& r ) { return !( l == r ); }

struct ParameterizedFunctionTemplateKeyHasher
{
	size_t operator()( const ParameterizedFunctionTemplateKey& k ) const { return k.Hash(); }
};

} // namespace U
