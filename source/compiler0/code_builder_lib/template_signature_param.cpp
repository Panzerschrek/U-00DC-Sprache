#include "../../lex_synt_lib_common/assert.hpp"
#include "template_signature_param.hpp"

namespace U
{

bool TemplateSignatureParam::Type::operator==( const Type& other ) const
{
	return this->t == other.t;
}

bool TemplateSignatureParam::Variable::operator==( const Variable& other ) const
{
	return
		this->type == other.type &&
		// LLVM constants are deduplicated, so, comparing pointers should work.
		this->constexpr_value == other.constexpr_value;
}

bool TemplateSignatureParam::TemplateParam::operator==( const TemplateParam& other ) const
{
	return this->index == other.index && this->kind_index == other.kind_index;
}

bool TemplateSignatureParam::TypeTemplate::operator==( const TypeTemplate& other ) const
{
	return this->type_template == other.type_template;
}

bool TemplateSignatureParam::Array::operator==( const Array& other ) const
{
	return *this->element_type == *other.element_type && *this->element_count == *other.element_count;
}

bool TemplateSignatureParam::Tuple::operator==( const Tuple& other ) const
{
	return this->element_types == other.element_types;
}

bool TemplateSignatureParam::RawPointer::operator==( const RawPointer& other ) const
{
	return *this->element_type == *other.element_type;
}

bool TemplateSignatureParam::Function::Param::operator==( const Param& other ) const
{
	return
		*this->type == *other.type &&
		this->value_type == other.value_type;
}

bool TemplateSignatureParam::Function::operator==( const Function& other ) const
{
	if( this->params.size() != other.params.size() )
		return false;

	for( size_t i= 0; i < this->params.size(); ++i )
		if( !( this->params[i] == other.params[i] ) )
			return false;

	return
		*this->return_type == *other.return_type &&
		this->return_value_type == other.return_value_type &&
		this->is_unsafe == other.is_unsafe &&
		this->calling_convention == other.calling_convention;
}

bool TemplateSignatureParam::Coroutine::operator==( const Coroutine& other ) const
{
	return
		this->kind == other.kind &&
		*this->return_type == *other.return_type &&
		this->return_value_type == other.return_value_type &&
		this->return_references == other.return_references &&
		this->return_inner_references == other.return_inner_references &&
		this->inner_references == other.inner_references &&
		this->non_sync == other.non_sync;
}

bool TemplateSignatureParam::SpecializedTemplate::operator==( const SpecializedTemplate& other ) const
{
	return
		this->type_templates == other.type_templates &&
		this->params == other.params;
}

TemplateSignatureParam::TemplateSignatureParam( Type type )
{
	something_= std::move(type);
}

TemplateSignatureParam::TemplateSignatureParam( Variable variable )
{
	something_= std::move(variable);
}

TemplateSignatureParam::TemplateSignatureParam( TypeTemplate type_template_param )
{
	something_= std::move(type_template_param);
}

TemplateSignatureParam::TemplateSignatureParam( TemplateParam template_parameter )
{
	something_= std::move(template_parameter);
}

TemplateSignatureParam::TemplateSignatureParam( Array array )
{
	something_= std::move(array);
}

TemplateSignatureParam::TemplateSignatureParam( RawPointer raw_pointer )
{
	something_= std::move(raw_pointer);
}

TemplateSignatureParam::TemplateSignatureParam( Tuple tuple )
{
	something_= std::move(tuple);
}

TemplateSignatureParam::TemplateSignatureParam( Function function )
{
	something_= std::move(function);
}

TemplateSignatureParam::TemplateSignatureParam( Coroutine coroutine )
{
	something_= std::move(coroutine);
}

TemplateSignatureParam::TemplateSignatureParam( SpecializedTemplate template_ )
{
	something_= std::move(template_);
}

bool TemplateSignatureParam::IsType() const
{
	return std::get_if<Type>( &something_ ) != nullptr;
}

bool TemplateSignatureParam::IsVariable() const
{
	return std::get_if<Variable>( &something_ ) != nullptr;
}

bool TemplateSignatureParam::IsTemplateParam() const
{
	return std::get_if<TemplateParam>( &something_ ) != nullptr;
}

const TemplateSignatureParam::Type* TemplateSignatureParam::GetType() const
{
	return std::get_if<Type>( &something_ );
}

const TemplateSignatureParam::Variable* TemplateSignatureParam::GetVariable() const
{
	return std::get_if<Variable>( &something_ );
}

const TemplateSignatureParam::TypeTemplate* TemplateSignatureParam::GetTypeTemplate() const
{
	return std::get_if<TypeTemplate>( &something_ );
}

const TemplateSignatureParam::TemplateParam* TemplateSignatureParam::GetTemplateParam() const
{
	return std::get_if<TemplateParam>( &something_ );
}

const TemplateSignatureParam::Array* TemplateSignatureParam::GetArray() const
{
	return std::get_if<Array>( &something_ );
}

const TemplateSignatureParam::Tuple* TemplateSignatureParam::GetTuple() const
{
	return std::get_if<Tuple>( &something_ );
}

const TemplateSignatureParam::RawPointer* TemplateSignatureParam::GetRawPointer() const
{
	return std::get_if<RawPointer>( &something_ );
}

const TemplateSignatureParam::Function* TemplateSignatureParam::GetFunction() const
{
	return std::get_if<Function>( &something_ );
}

const TemplateSignatureParam::Coroutine* TemplateSignatureParam::GetCoroutine() const
{
	return std::get_if<Coroutine>( &something_ );
}

const TemplateSignatureParam::SpecializedTemplate* TemplateSignatureParam::GetSpecializedTemplate() const
{
	return std::get_if<SpecializedTemplate>( &something_ );
}

bool TemplateSignatureParam::operator==( const TemplateSignatureParam& other ) const
{
	return this->something_ == other.something_;
}

namespace
{

TemplateSignatureParam MapTemplateParamsToSignatureParamsImpl(
	const TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam::Type& param )
{
	// Return type params as is.
	U_UNUSED(mapping);
	return param;
}

TemplateSignatureParam MapTemplateParamsToSignatureParamsImpl(
	const TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam::Variable& param )
{
	// Return variable params as is.
	U_UNUSED(mapping);
	return param;
}

TemplateSignatureParam MapTemplateParamsToSignatureParamsImpl(
	const TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam::TypeTemplate& param )
{
	// Return type template params as is.
	U_UNUSED(mapping);
	return param;
}

TemplateSignatureParam MapTemplateParamsToSignatureParamsImpl(
	const TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam::TemplateParam& param )
{
	// Map this template param.
	U_ASSERT( param.index < mapping.size() );
	return mapping[param.index];
}

TemplateSignatureParam MapTemplateParamsToSignatureParamsImpl(
	const TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam::Array& param )
{
	TemplateSignatureParam::Array out_param;
	out_param.element_count= std::make_shared<TemplateSignatureParam>( MapTemplateParamsToSignatureParams( mapping, *param.element_count ) );
	out_param.element_type= std::make_shared<TemplateSignatureParam>( MapTemplateParamsToSignatureParams( mapping, *param.element_type ) );
	return out_param;
}

TemplateSignatureParam MapTemplateParamsToSignatureParamsImpl(
	const TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam::Tuple& param )
{
	TemplateSignatureParam::Tuple out_param;
	out_param.element_types.reserve( param.element_types.size() );

	for( const TemplateSignatureParam& element_type_param : param.element_types )
		out_param.element_types.push_back( MapTemplateParamsToSignatureParams( mapping, element_type_param ) );

	return out_param;
}

TemplateSignatureParam MapTemplateParamsToSignatureParamsImpl(
	const TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam::RawPointer& param )
{
	TemplateSignatureParam::RawPointer out_param;
	out_param.element_type= std::make_shared<TemplateSignatureParam>( MapTemplateParamsToSignatureParams( mapping, *param.element_type ) );
	return out_param;
}

TemplateSignatureParam MapTemplateParamsToSignatureParamsImpl(
	const TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam::Function& param )
{
	TemplateSignatureParam::Function out_param= param;
	out_param.return_type= std::make_shared<TemplateSignatureParam>( MapTemplateParamsToSignatureParams( mapping, *param.return_type ) );

	for( TemplateSignatureParam::Function::Param& function_param : out_param.params )
		function_param.type= std::make_shared<TemplateSignatureParam>( MapTemplateParamsToSignatureParams( mapping, *function_param.type ) );

	return out_param;
}

TemplateSignatureParam MapTemplateParamsToSignatureParamsImpl(
	const TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam::Coroutine& param )
{
	TemplateSignatureParam::Coroutine out_param= param;
	out_param.return_type= std::make_shared<TemplateSignatureParam>( MapTemplateParamsToSignatureParams( mapping, *param.return_type ) );
	return out_param;
}

TemplateSignatureParam MapTemplateParamsToSignatureParamsImpl(
	const TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam::SpecializedTemplate& param )
{
	TemplateSignatureParam::SpecializedTemplate out_param;

	out_param.type_templates.reserve( param.type_templates.size() );
	for( const TemplateSignatureParam& type_template : param.type_templates )
		out_param.type_templates.push_back( MapTemplateParamsToSignatureParams( mapping, type_template ) );

	out_param.params.reserve( param.params.size() );
	for( const TemplateSignatureParam& template_param : param.params )
		out_param.params.push_back( MapTemplateParamsToSignatureParams( mapping, template_param ) );

	return out_param;
}

} // namespace

TemplateSignatureParam MapTemplateParamsToSignatureParams(
	const TemplateParamsToSignatureParamsMappingRef mapping,
	const TemplateSignatureParam& param )
{
	return param.Visit( [&]( const auto& el ) { return MapTemplateParamsToSignatureParamsImpl( mapping, el ); } );
}

} // namespace U
