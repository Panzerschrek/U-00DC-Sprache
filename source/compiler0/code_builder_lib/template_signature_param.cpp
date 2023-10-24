#include "template_signature_param.hpp"

namespace U
{

bool TemplateSignatureParam::TypeParam::operator==( const TypeParam& other ) const
{
	return this->t == other.t;
}

bool TemplateSignatureParam::VariableParam::operator==( const VariableParam& other ) const
{
	return
		this->type == other.type &&
		this->constexpr_value->getUniqueInteger().getLimitedValue() == other.constexpr_value->getUniqueInteger().getLimitedValue();
}

bool TemplateSignatureParam::TemplateParam::operator==( const TemplateParam& other ) const
{
	return this->index == other.index;
}

bool TemplateSignatureParam::ArrayParam::operator==( const ArrayParam& other ) const
{
	return *this->element_type == *other.element_type && *this->element_count == *other.element_count;
}

bool TemplateSignatureParam::TupleParam::operator==( const TupleParam& other ) const
{
	return this->element_types == other.element_types;
}

bool TemplateSignatureParam::RawPointerParam::operator==( const RawPointerParam& other ) const
{
	return *this->element_type == *other.element_type;
}

bool TemplateSignatureParam::FunctionParam::Param::operator==( const Param& other ) const
{
	return
		*this->type == *other.type &&
		this->value_type == other.value_type;
}

bool TemplateSignatureParam::FunctionParam::operator==( const FunctionParam& other ) const
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

bool TemplateSignatureParam::CoroutineParam::operator==( const CoroutineParam& other ) const
{
	return
		this->kind == other.kind &&
		*this->return_type == *other.return_type &&
		this->return_value_type == other.return_value_type &&
		this->inner_references == other.inner_references &&
		this->non_sync == other.non_sync;
}

bool TemplateSignatureParam::SpecializedTemplateParam::operator==( const SpecializedTemplateParam& other ) const
{
	return
		this->type_templates == other.type_templates &&
		this->params == other.params;
}

TemplateSignatureParam::TemplateSignatureParam( TypeParam type )
{
	something_= std::move(type);
}

TemplateSignatureParam::TemplateSignatureParam( VariableParam variable )
{
	something_= std::move(variable);
}

TemplateSignatureParam::TemplateSignatureParam( TemplateParam template_parameter )
{
	something_= std::move(template_parameter);
}

TemplateSignatureParam::TemplateSignatureParam( ArrayParam array )
{
	something_= std::move(array);
}

TemplateSignatureParam::TemplateSignatureParam( RawPointerParam raw_pointer )
{
	something_= std::move(raw_pointer);
}

TemplateSignatureParam::TemplateSignatureParam( TupleParam tuple )
{
	something_= std::move(tuple);
}

TemplateSignatureParam::TemplateSignatureParam( FunctionParam function )
{
	something_= std::move(function);
}

TemplateSignatureParam::TemplateSignatureParam( CoroutineParam coroutine )
{
	something_= std::move(coroutine);
}

TemplateSignatureParam::TemplateSignatureParam( SpecializedTemplateParam template_ )
{
	something_= std::move(template_);
}

bool TemplateSignatureParam::IsType() const
{
	return std::get_if<TypeParam>( &something_ ) != nullptr;
}

bool TemplateSignatureParam::IsVariable() const
{
	return std::get_if<VariableParam>( &something_ ) != nullptr;
}

bool TemplateSignatureParam::IsTemplateParam() const
{
	return std::get_if<TemplateParam>( &something_ ) != nullptr;
}

const TemplateSignatureParam::TypeParam* TemplateSignatureParam::GetType() const
{
	return std::get_if<TypeParam>( &something_ );
}

const TemplateSignatureParam::VariableParam* TemplateSignatureParam::GetVariable() const
{
	return std::get_if<VariableParam>( &something_ );
}

const TemplateSignatureParam::TemplateParam* TemplateSignatureParam::GetTemplateParam() const
{
	return std::get_if<TemplateParam>( &something_ );
}

const TemplateSignatureParam::ArrayParam* TemplateSignatureParam::GetArray() const
{
	return std::get_if<ArrayParam>( &something_ );
}

const TemplateSignatureParam::TupleParam* TemplateSignatureParam::GetTuple() const
{
	return std::get_if<TupleParam>( &something_ );
}

const TemplateSignatureParam::RawPointerParam* TemplateSignatureParam::GetRawPointer() const
{
	return std::get_if<RawPointerParam>( &something_ );
}

const TemplateSignatureParam::FunctionParam* TemplateSignatureParam::GetFunction() const
{
	return std::get_if<FunctionParam>( &something_ );
}

const TemplateSignatureParam::CoroutineParam* TemplateSignatureParam::GetCoroutine() const
{
	return std::get_if<CoroutineParam>( &something_ );
}

const TemplateSignatureParam::SpecializedTemplateParam* TemplateSignatureParam::GetTemplate() const
{
	return std::get_if<SpecializedTemplateParam>( &something_ );
}

bool TemplateSignatureParam::operator==( const TemplateSignatureParam& other ) const
{
	return this->something_ == other.something_;
}

} // namespace U
