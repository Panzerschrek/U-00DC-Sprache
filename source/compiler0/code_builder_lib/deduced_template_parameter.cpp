#include "deduced_template_parameter.hpp"

namespace U
{

namespace CodeBuilderPrivate
{


DeducedTemplateParameter::ArrayParam::ArrayParam( const ArrayParam& other )
{
	*this= other;
}

DeducedTemplateParameter::ArrayParam& DeducedTemplateParameter::ArrayParam::operator=( const ArrayParam& other )
{
	size= std::make_unique<DeducedTemplateParameter>( *other.size );
	type= std::make_unique<DeducedTemplateParameter>( *other.type );
	return *this;
}

DeducedTemplateParameter::FunctionParam::FunctionParam( const FunctionParam& other )
{
	*this= other;
}

DeducedTemplateParameter::FunctionParam& DeducedTemplateParameter::FunctionParam::operator=( const FunctionParam& other )
{
	return_type= std::make_unique<DeducedTemplateParameter>( *other.return_type );

	return_value_is_mutable= other.return_value_is_mutable;
	return_value_is_reference= other.return_value_is_reference;
	is_unsafe= other.is_unsafe;

	params.clear();
	params.reserve( other.params.size() );
	for( const Param& param : other.params )
	{
		Param out_param;
		out_param.type= std::make_unique<DeducedTemplateParameter>( *param.type );
		out_param.is_mutable= param.is_mutable;
		out_param.is_reference= param.is_reference;
		params.push_back( std::move(out_param) );
	}

	return *this;
}

DeducedTemplateParameter::DeducedTemplateParameter( InvalidParam invalid )
{
	something_= std::move(invalid);
}

DeducedTemplateParameter::DeducedTemplateParameter( TypeParam type )
{
	something_= std::move(type);
}

DeducedTemplateParameter::DeducedTemplateParameter( VariableParam variable )
{
	something_= std::move(variable);
}

DeducedTemplateParameter::DeducedTemplateParameter( TemplateParameter template_parameter )
{
	something_= std::move(template_parameter);
}

DeducedTemplateParameter::DeducedTemplateParameter( ArrayParam array )
{
	something_= std::move(array);
}

DeducedTemplateParameter::DeducedTemplateParameter( TupleParam tuple )
{
	something_= std::move(tuple);
}

DeducedTemplateParameter::DeducedTemplateParameter( FunctionParam function )
{
	something_= std::move(function);
}

DeducedTemplateParameter::DeducedTemplateParameter( SpecializedTemplateParam template_ )
{
	something_= std::move(template_);
}

bool DeducedTemplateParameter::IsInvalid() const
{
	return std::get_if<InvalidParam>( &something_ ) != nullptr;
}

bool DeducedTemplateParameter::IsType() const
{
	return std::get_if<TypeParam>( &something_ ) != nullptr;
}

bool DeducedTemplateParameter::IsVariable() const
{
	return std::get_if<VariableParam>( &something_ ) != nullptr;
}

bool DeducedTemplateParameter::IsTemplateParameter() const
{
	return std::get_if<TemplateParameter>( &something_ ) != nullptr;
}

const DeducedTemplateParameter::ArrayParam* DeducedTemplateParameter::GetArray() const
{
	return std::get_if<ArrayParam>( &something_ );
}

const DeducedTemplateParameter::TupleParam* DeducedTemplateParameter::GetTuple() const
{
	return std::get_if<TupleParam>( &something_ );
}

const DeducedTemplateParameter::FunctionParam* DeducedTemplateParameter::GetFunction() const
{
	return std::get_if<FunctionParam>( &something_ );
}

const DeducedTemplateParameter::SpecializedTemplateParam* DeducedTemplateParameter::GetTemplate() const
{
	return std::get_if<SpecializedTemplateParam>( &something_ );
}

} // namespace CodeBuilderPrivate

} // namespace U
