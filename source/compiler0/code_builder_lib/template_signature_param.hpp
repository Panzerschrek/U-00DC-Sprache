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

	// HACK!
	// use shared_ptr to avoid manual copy constructors creation.

	struct ArrayParam
	{
		std::shared_ptr<const TemplateSignatureParam> size;
		std::shared_ptr<const TemplateSignatureParam> type;
	};

	struct TupleParam
	{
		std::vector<TemplateSignatureParam> element_types;
	};

	struct RawPointerParam
	{
		std::shared_ptr<const TemplateSignatureParam> type;
	};

	struct FunctionParam
	{
		std::shared_ptr<const TemplateSignatureParam> return_type;
		bool return_value_is_mutable;
		bool return_value_is_reference;

		bool is_unsafe;

		struct Param
		{
			std::shared_ptr<const TemplateSignatureParam> type;
			bool is_mutable;
			bool is_reference;
		};
		std::vector<Param> params;
	};

	struct SpecializedTemplateParam
	{
		std::vector<TypeTemplatePtr> type_templates;
		std::vector<TemplateSignatureParam> params;
	};

public:
	TemplateSignatureParam( TypeParam type= TypeParam() );
	TemplateSignatureParam( VariableParam variable );
	TemplateSignatureParam( TemplateParam template_parameter );
	TemplateSignatureParam( ArrayParam array );
	TemplateSignatureParam( TupleParam tuple );
	TemplateSignatureParam( RawPointerParam raw_pointer );
	TemplateSignatureParam( FunctionParam function );
	TemplateSignatureParam( SpecializedTemplateParam template_ );

	bool IsType() const;
	bool IsVariable() const;
	bool IsTemplateParam() const;
	const TypeParam* GetType() const;
	const VariableParam* GetVariable() const;
	const TemplateParam* GetTemplateParam() const;
	const ArrayParam* GetArray() const;
	const TupleParam* GetTuple() const;
	const RawPointerParam* GetRawPointer() const;
	const FunctionParam* GetFunction() const;
	const SpecializedTemplateParam* GetTemplate() const;

	template<class Func>
	auto Visit( const Func& func ) const
	{
		return std::visit( func, something_ );
	}

private:
	std::variant<
		TypeParam,
		VariableParam,
		TemplateParam,
		ArrayParam,
		TupleParam,
		RawPointerParam,
		FunctionParam,
		SpecializedTemplateParam> something_;
};

} // namespace CodeBuilderPrivate

} // namespace U
