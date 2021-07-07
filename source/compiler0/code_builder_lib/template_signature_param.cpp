#include "template_signature_param.hpp"

namespace U
{

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

const TemplateSignatureParam::SpecializedTemplateParam* TemplateSignatureParam::GetTemplate() const
{
	return std::get_if<SpecializedTemplateParam>( &something_ );
}

} // namespace U
