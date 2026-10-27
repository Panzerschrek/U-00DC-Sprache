#pragma once
#include <memory>
#include <variant>
#include <vector>
#include "../../lex_synt_lib_common/assert.hpp"
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

		// See TemplateParameter::kind_data.
		size_t kind_index= ~0u;

		bool operator==( const TemplateParam& other ) const;
	};

	struct Array;

	struct Tuple
	{
		std::vector<TemplateSignatureParam> element_types;

		bool operator==( const Tuple& other ) const;
	};

	struct RawPointer;
	struct Function;
	struct Coroutine;

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
		if( const auto type= GetType() )
			return func( *type );
		else if( const auto variable= GetVariable() )
			return func( *variable );
		else if( const auto type_template= GetTypeTemplate() )
			return func( *type_template );
		else if( const auto template_param= GetTemplateParam() )
			return func( *template_param );
		else if( const auto array= GetArray() )
			return func( *array );
		else if( const auto tuple= GetTuple() )
			return func( *tuple );
		else if( const auto raw_pointer= GetRawPointer() )
			return func( *raw_pointer );
		else if( const auto function= GetFunction() )
			return func( *function );
		else if( const auto coroutine= GetCoroutine() )
			return func( *coroutine );
		else if( const auto specialized_template= GetSpecializedTemplate() )
			return func( *specialized_template );
		else
		{
			U_ASSERT( false );
			return func( *GetSpecializedTemplate() );
		}
	}

	bool operator==( const TemplateSignatureParam& other ) const;
	bool operator!=( const TemplateSignatureParam& other ) const { return !(*this == other); }

private:
	// Use RcPtr to allow cheap copying.
	std::variant<
		Type,
		Variable,
		TypeTemplate,
		TemplateParam,
		RcPtr<Array>,
		Tuple,
		RcPtr<RawPointer>,
		RcPtr<Function>,
		RcPtr<Coroutine>,
		SpecializedTemplate> something_;
};

// Declare it after "TemplateSignatureParam", since we require it to be complete.

struct TemplateSignatureParam::Array
{
	TemplateSignatureParam element_count;
	TemplateSignatureParam element_type;

	bool operator==( const Array& other ) const;
};

struct TemplateSignatureParam::RawPointer
{
	TemplateSignatureParam element_type;

	bool operator==( const RawPointer& other ) const;
};

struct TemplateSignatureParam::Function
{
	TemplateSignatureParam return_type;
	ValueType return_value_type= ValueType::Value;

	bool is_unsafe= false;
	bool is_variadic= false;
	CallingConvention calling_convention= CallingConvention::Default;

	struct Param
	{
		TemplateSignatureParam type;
		ValueType value_type= ValueType::Value;

		bool operator==( const Param& other ) const;
	};

	std::vector<Param> params;

	bool operator==( const Function& other ) const;
};

struct TemplateSignatureParam::Coroutine
{
	TemplateSignatureParam return_type;

	FunctionType::ReturnReferences return_references;
	FunctionType::ReturnInnerReferences return_inner_references;

	llvm::SmallVector<InnerReferenceKind, 4> inner_references;

	CoroutineKind kind= CoroutineKind::Generator;
	ValueType return_value_type= ValueType::Value;
	bool non_sync= false;

	bool operator==( const Coroutine& other ) const;
};

// Mapping of template params 0-N to signature params.
using TemplateParamsToSignatureParamsMappingRef= llvm::ArrayRef<TemplateSignatureParam>;

// Replace all template params with given signature params.
TemplateSignatureParam MapTemplateParamsToSignatureParams(
	TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam& param );

} // namespace U
