#include "template_signature_param.hpp"

namespace U
{

namespace CodeBuilderPrivate
{


TemplateSignatureParam::ArrayParam::ArrayParam( const ArrayParam& other )
{
	*this= other;
}

TemplateSignatureParam::ArrayParam& TemplateSignatureParam::ArrayParam::operator=( const ArrayParam& other )
{
	size= std::make_unique<TemplateSignatureParam>( *other.size );
	type= std::make_unique<TemplateSignatureParam>( *other.type );
	return *this;
}

TemplateSignatureParam::FunctionParam::FunctionParam( const FunctionParam& other )
{
	*this= other;
}

TemplateSignatureParam::FunctionParam& TemplateSignatureParam::FunctionParam::operator=( const FunctionParam& other )
{
	return_type= std::make_unique<TemplateSignatureParam>( *other.return_type );

	return_value_is_mutable= other.return_value_is_mutable;
	return_value_is_reference= other.return_value_is_reference;
	is_unsafe= other.is_unsafe;

	params.clear();
	params.reserve( other.params.size() );
	for( const Param& param : other.params )
	{
		Param out_param;
		out_param.type= std::make_unique<TemplateSignatureParam>( *param.type );
		out_param.is_mutable= param.is_mutable;
		out_param.is_reference= param.is_reference;
		params.push_back( std::move(out_param) );
	}

	return *this;
}

TemplateSignatureParam::TemplateSignatureParam( InvalidParam invalid )
{
	something_= std::move(invalid);
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

TemplateSignatureParam::TemplateSignatureParam( TupleParam tuple )
{
	something_= std::move(tuple);
}

TemplateSignatureParam::TemplateSignatureParam( FunctionParam function )
{
	something_= std::move(function);
}

TemplateSignatureParam::TemplateSignatureParam( SpecializedTemplateParam template_ )
{
	something_= std::move(template_);
}

bool TemplateSignatureParam::IsInvalid() const
{
	return std::get_if<InvalidParam>( &something_ ) != nullptr;
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

const TemplateSignatureParam::FunctionParam* TemplateSignatureParam::GetFunction() const
{
	return std::get_if<FunctionParam>( &something_ );
}

const TemplateSignatureParam::SpecializedTemplateParam* TemplateSignatureParam::GetTemplate() const
{
	return std::get_if<SpecializedTemplateParam>( &something_ );
}

} // namespace CodeBuilderPrivate

} // namespace U
