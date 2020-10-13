#pragma once
#include <memory>
#include <variant>
#include <vector>


namespace U
{

namespace CodeBuilderPrivate
{

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

} // namespace CodeBuilderPrivate

} // namespace U
