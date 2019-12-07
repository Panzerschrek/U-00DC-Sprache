#pragma once
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include "pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"
#include "class.hpp"
#include "enum.hpp"
#include "small_types.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

using Synt::ClassMemberVisibility;

// "Class" of function argument in terms of overloading.
enum class ArgOverloadingClass
{
	// Value-args (both mutable and immutable), immutable references.
	ImmutableReference,
	// Mutable references.
	MutalbeReference,
	// SPRACHE_TODO - add class for move-references here
};

ArgOverloadingClass GetArgOverloadingClass( bool is_reference, bool is_mutable );
ArgOverloadingClass GetArgOverloadingClass( ValueType value_type );
ArgOverloadingClass GetArgOverloadingClass( const Function::Arg& arg );

struct TemplateBase
{
	virtual ~TemplateBase()= default;

	struct TemplateParameter
	{
		ProgramString name;
		const Synt::ComplexName* type_name= nullptr; // Exists for value parameters.
	};

	std::vector< TemplateParameter > template_parameters;

	NamesScope* parent_namespace= nullptr; // NamesScope, where defined. NOT changed after import.

	FilePos file_pos;
};

struct TypeTemplate final : TemplateBase
{
	std::vector< const Synt::Expression* > signature_arguments;
	std::vector< const Synt::Expression* > default_signature_arguments;
	size_t first_optional_signature_argument= ~0u;

	enum class Kind
	{
		Class,
		Typedef,
	};

	Kind kind= Kind::Class;
	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::TypeTemplateBase* syntax_element= nullptr;
};

using DeducibleTemplateParameter= std::variant< int, Type, Variable >; // int means not deduced
using DeducibleTemplateParameters= std::vector<DeducibleTemplateParameter>;

struct FunctionTemplate final : public TemplateBase
{
	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::FunctionTemplate* syntax_element= nullptr;

	ClassProxyPtr base_class;

	std::vector< std::pair< ProgramString, Value > > known_template_parameters;
};

class DeducedTemplateParameter
{
public:
	struct Invalid{};
	struct Type{};
	struct Variable{};
	struct TemplateParameter{};

	struct Array
	{
		std::unique_ptr<DeducedTemplateParameter> size;
		std::unique_ptr<DeducedTemplateParameter> type;

		Array()= default;
		Array(Array&&)= default;
		Array& operator=(Array&&)= default;

		Array( const Array& other );
		Array& operator=( const Array& other );
	};

	struct Tuple
	{
		std::vector<DeducedTemplateParameter> element_types;
	};

	struct Function
	{
		std::unique_ptr<DeducedTemplateParameter> return_type;
		std::vector<DeducedTemplateParameter> argument_types;

		Function()= default;
		Function(Function&&)= default;
		Function& operator=(Function&&)= default;

		Function( const Function& other );
		Function& operator=( const Function& other );
	};

	struct Template
	{
		std::vector<DeducedTemplateParameter> args;
	};

public:
	DeducedTemplateParameter( Invalid invalid= Invalid() );
	DeducedTemplateParameter( Type type );
	DeducedTemplateParameter( Variable variable );
	DeducedTemplateParameter( TemplateParameter template_parameter );
	DeducedTemplateParameter( Array array );
	DeducedTemplateParameter( Tuple tuple );
	DeducedTemplateParameter( Function function );
	DeducedTemplateParameter( Template template_ );

	bool IsInvalid() const;
	bool IsType() const;
	bool IsVariable() const;
	bool IsTemplateParameter() const;
	const Array* GetArray() const;
	const Tuple* GetTuple() const;
	const Function* GetFunction() const;
	const Template* GetTemplate() const;

private:
	std::variant<
		Invalid,
		Type,
		Variable,
		TemplateParameter,
		Array,
		Tuple,
		Function,
		Template> something_;
};

} //namespace CodeBuilderLLVMPrivate

} // namespace U
