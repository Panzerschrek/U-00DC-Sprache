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
	struct Type
	{
		U::Type t;

		bool operator==( const Type& other ) const;
	};

	struct Variable
	{
		U::Type type;
		llvm::Constant* constexpr_value= nullptr;

		bool operator==( const Variable& other ) const;
	};

	struct TypeTemplate
	{
		TypeTemplatePtr type_template;

		bool operator==( const TypeTemplate& other ) const;
	};

	struct TemplateParam
	{
		size_t index= ~0u;

		// See TemplateBase::TemplateParameter::kind_data.
		size_t kind_index= ~0u;

		bool operator==( const TemplateParam& other ) const;
	};

	// use shared_ptr to avoid manual copy constructors creation.

	struct Array
	{
		std::shared_ptr<const TemplateSignatureParam> element_count;
		std::shared_ptr<const TemplateSignatureParam> element_type;

		bool operator==( const Array& other ) const;
	};

	struct Tuple
	{
		std::vector<TemplateSignatureParam> element_types;

		bool operator==( const Tuple& other ) const;
	};

	struct RawPointer
	{
		std::shared_ptr<const TemplateSignatureParam> element_type;

		bool operator==( const RawPointer& other ) const;
	};

	struct Function
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

		bool operator==( const Function& other ) const;
	};

	struct Coroutine
	{
		CoroutineKind kind= CoroutineKind::Generator;
		std::shared_ptr<const TemplateSignatureParam> return_type;
		ValueType return_value_type= ValueType::Value;

		FunctionType::ReturnReferences return_references;
		FunctionType::ReturnInnerReferences return_inner_references;

		llvm::SmallVector<InnerReferenceKind, 4> inner_references;
		bool non_sync= false;

		bool operator==( const Coroutine& other ) const;
	};

	struct SpecializedTemplate
	{
		std::vector<TemplateSignatureParam> type_templates;
		std::vector<TemplateSignatureParam> params;

		bool operator==( const SpecializedTemplate& other ) const;
	};

public:
	TemplateSignatureParam( Type type= Type() );
	TemplateSignatureParam( Variable variable );
	TemplateSignatureParam( TypeTemplate type_template_param );
	TemplateSignatureParam( TemplateParam template_parameter );
	TemplateSignatureParam( Array array );
	TemplateSignatureParam( Tuple tuple );
	TemplateSignatureParam( RawPointer raw_pointer );
	TemplateSignatureParam( Function function );
	TemplateSignatureParam( Coroutine coroutine );
	TemplateSignatureParam( SpecializedTemplate template_ );

	bool IsType() const;
	bool IsVariable() const;
	bool IsTemplateParam() const;
	const Type* GetType() const;
	const Variable* GetVariable() const;
	const TypeTemplate* GetTypeTemplate() const;
	const TemplateParam* GetTemplateParam() const;
	const Array* GetArray() const;
	const Tuple* GetTuple() const;
	const RawPointer* GetRawPointer() const;
	const Function* GetFunction() const;
	const Coroutine* GetCoroutine() const;
	const SpecializedTemplate* GetSpecializedTemplate() const;

	template<class Func>
	auto Visit( const Func& func ) const
	{
		return std::visit( func, something_ );
	}

	bool operator==( const TemplateSignatureParam& other ) const;
	bool operator!=( const TemplateSignatureParam& other ) const { return !(*this == other); }

private:
	std::variant<
		Type,
		Variable,
		TypeTemplate,
		TemplateParam,
		Array,
		Tuple,
		RawPointer,
		Function,
		Coroutine,
		SpecializedTemplate> something_;
};

// Mapping of template params 0-N to signature params.
using TemplateParamsToSignatureParamsMappingRef= llvm::ArrayRef<TemplateSignatureParam>;

// Replace all template params with given signature params.
TemplateSignatureParam MapTemplateParamsToSignatureParams(
	TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam& param );

} // namespace U
