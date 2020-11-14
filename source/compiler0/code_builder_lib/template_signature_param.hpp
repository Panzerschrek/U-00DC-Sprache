#pragma once
#include <memory>
#include <variant>
#include <vector>
#include "value.hpp"


namespace U
{

namespace CodeBuilderPrivate
{


class TemplateSignatureParam
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

	struct TemplateParam
	{
		size_t index;
	};

	struct ArrayParam
	{
		std::unique_ptr<TemplateSignatureParam> size;
		std::unique_ptr<TemplateSignatureParam> type;

		ArrayParam()= default;
		ArrayParam(ArrayParam&&)= default;
		ArrayParam& operator=(ArrayParam&&)= default;

		ArrayParam( const ArrayParam& other );
		ArrayParam& operator=( const ArrayParam& other );
	};

	struct TupleParam
	{
		std::vector<TemplateSignatureParam> element_types;
	};

	struct FunctionParam
	{
		std::unique_ptr<TemplateSignatureParam> return_type;
		bool return_value_is_mutable;
		bool return_value_is_reference;

		bool is_unsafe;

		struct Param
		{
			std::unique_ptr<TemplateSignatureParam> type;
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
		std::vector<TemplateSignatureParam> params;
	};

public:
	TemplateSignatureParam( InvalidParam invalid= InvalidParam() );
	TemplateSignatureParam( TypeParam type );
	TemplateSignatureParam( VariableParam variable );
	TemplateSignatureParam( TemplateParam template_parameter );
	TemplateSignatureParam( ArrayParam array );
	TemplateSignatureParam( TupleParam tuple );
	TemplateSignatureParam( FunctionParam function );
	TemplateSignatureParam( SpecializedTemplateParam template_ );

	bool IsInvalid() const;
	bool IsType() const;
	bool IsVariable() const;
	bool IsTemplateParam() const;
	const TypeParam* GetType() const;
	const VariableParam* GetVariable() const;
	const TemplateParam* GetTemplateParam() const;
	const ArrayParam* GetArray() const;
	const TupleParam* GetTuple() const;
	const FunctionParam* GetFunction() const;
	const SpecializedTemplateParam* GetTemplate() const;

	template<class Func>
	auto Visit( const Func& func ) const
	{
		return std::visit( func, something_ );
	}

private:
	std::variant<
		InvalidParam,
		TypeParam,
		VariableParam,
		TemplateParam,
		ArrayParam,
		TupleParam,
		FunctionParam,
		SpecializedTemplateParam> something_;
};

} // namespace CodeBuilderPrivate

} // namespace U
