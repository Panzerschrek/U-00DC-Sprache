#pragma once
#include <memory>
#include <variant>
#include <vector>
#include "coroutine.hpp"
#include "value.hpp"


namespace U
{

class TemplateSignatureParam
{
public:
	struct TypeParam
	{
		Type t;

		bool operator==( const TypeParam& other ) const;
	};

	struct VariableParam
	{
		Type type;
		llvm::Constant* constexpr_value= nullptr;

		bool operator==( const VariableParam& other ) const;
	};

	struct TemplateParam
	{
		size_t index;

		bool operator==( const TemplateParam& other ) const;
	};

	// use shared_ptr to avoid manual copy constructors creation.

	struct ArrayParam
	{
		std::shared_ptr<const TemplateSignatureParam> element_count;
		std::shared_ptr<const TemplateSignatureParam> element_type;

		bool operator==( const ArrayParam& other ) const;
	};

	struct TupleParam
	{
		std::vector<TemplateSignatureParam> element_types;

		bool operator==( const TupleParam& other ) const;
	};

	struct RawPointerParam
	{
		std::shared_ptr<const TemplateSignatureParam> element_type;

		bool operator==( const RawPointerParam& other ) const;
	};

	struct FunctionParam
	{
		std::shared_ptr<const TemplateSignatureParam> return_type;
		ValueType return_value_type= ValueType::Value;

		bool is_unsafe= false;
		llvm::CallingConv::ID calling_convention= llvm::CallingConv::C;

		struct Param
		{
			std::shared_ptr<const TemplateSignatureParam> type;
			ValueType value_type= ValueType::Value;

			bool operator==( const Param& other ) const;
		};
		std::vector<Param> params;

		bool operator==( const FunctionParam& other ) const;
	};

	struct CoroutineParam
	{
		CoroutineKind kind= CoroutineKind::Generator;
		std::shared_ptr<const TemplateSignatureParam> return_type;
		ValueType return_value_type= ValueType::Value;

		InnerReferenceType inner_reference_type= InnerReferenceType::None;

		bool operator==( const CoroutineParam& other ) const;
	};

	struct SpecializedTemplateParam
	{
		std::vector<TypeTemplatePtr> type_templates;
		std::vector<TemplateSignatureParam> params;

		bool operator==( const SpecializedTemplateParam& other ) const;
	};

public:
	TemplateSignatureParam( TypeParam type= TypeParam() );
	TemplateSignatureParam( VariableParam variable );
	TemplateSignatureParam( TemplateParam template_parameter );
	TemplateSignatureParam( ArrayParam array );
	TemplateSignatureParam( TupleParam tuple );
	TemplateSignatureParam( RawPointerParam raw_pointer );
	TemplateSignatureParam( FunctionParam function );
	TemplateSignatureParam( CoroutineParam coroutine );
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
	const CoroutineParam* GetCoroutine() const;
	const SpecializedTemplateParam* GetTemplate() const;

	template<class Func>
	auto Visit( const Func& func ) const
	{
		return std::visit( func, something_ );
	}

	bool operator==( const TemplateSignatureParam& other )const;

private:
	std::variant<
		TypeParam,
		VariableParam,
		TemplateParam,
		ArrayParam,
		TupleParam,
		RawPointerParam,
		FunctionParam,
		CoroutineParam,
		SpecializedTemplateParam> something_;
};

} // namespace U
