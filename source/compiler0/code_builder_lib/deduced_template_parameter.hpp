#pragma once
#include <memory>
#include <variant>
#include <vector>
#include "value.hpp"


namespace U
{

namespace CodeBuilderPrivate
{


class DeducedTemplateParameter
{
public:
	struct InvalidParam{};

	struct TypeParam
	{
		Type t;
	};

	struct VariableParam
	{
		Variable v;
	};

	struct TemplateParameter
	{
		size_t index;
	};

	struct ArrayParam
	{
		std::unique_ptr<DeducedTemplateParameter> size;
		std::unique_ptr<DeducedTemplateParameter> type;

		ArrayParam()= default;
		ArrayParam(ArrayParam&&)= default;
		ArrayParam& operator=(ArrayParam&&)= default;

		ArrayParam( const ArrayParam& other );
		ArrayParam& operator=( const ArrayParam& other );
	};

	struct TupleParam
	{
		std::vector<DeducedTemplateParameter> element_types;
	};

	struct FunctionParam
	{
		std::unique_ptr<DeducedTemplateParameter> return_type;
		bool return_value_is_mutable;
		bool return_value_is_reference;

		bool is_unsafe;

		struct Param
		{
			std::unique_ptr<DeducedTemplateParameter> type;
			bool is_mutable;
			bool is_reference;
		};
		std::vector<Param> params;

		FunctionParam()= default;
		FunctionParam(FunctionParam&&)= default;
		FunctionParam& operator=(FunctionParam&&)= default;

		FunctionParam( const FunctionParam& other );
		FunctionParam& operator=( const FunctionParam& other );
	};

	struct SpecializedTemplateParam
	{
		std::vector<TypeTemplatePtr> type_templates;
		std::vector<DeducedTemplateParameter> params;
	};

public:
	DeducedTemplateParameter( InvalidParam invalid= InvalidParam() );
	DeducedTemplateParameter( TypeParam type );
	DeducedTemplateParameter( VariableParam variable );
	DeducedTemplateParameter( TemplateParameter template_parameter );
	DeducedTemplateParameter( ArrayParam array );
	DeducedTemplateParameter( TupleParam tuple );
	DeducedTemplateParameter( FunctionParam function );
	DeducedTemplateParameter( SpecializedTemplateParam template_ );

	bool IsInvalid() const;
	bool IsType() const;
	bool IsVariable() const;
	bool IsTemplateParameter() const;
	const ArrayParam* GetArray() const;
	const TupleParam* GetTuple() const;
	const FunctionParam* GetFunction() const;
	const SpecializedTemplateParam* GetTemplate() const;

private:
	std::variant<
		InvalidParam,
		TypeParam,
		VariableParam,
		TemplateParameter,
		ArrayParam,
		TupleParam,
		FunctionParam,
		SpecializedTemplateParam> something_;
};

} // namespace CodeBuilderPrivate

} // namespace U
