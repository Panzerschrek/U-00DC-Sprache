#include "template_signature_param.hpp"
#include "class.hpp"
#include "keywords.hpp"

namespace U
{

const std::string_view Class::c_template_class_name= "_";

Class::Class( std::string in_name, NamesScope* const parent_scope )
	: members( std::make_shared<NamesScope>( std::move(in_name), parent_scope ) )
	, members_initial(members)
{}

ClassMemberVisibility Class::GetMemberVisibility( const std::string_view member_name ) const
{
	const auto it= members_visibility.find( member_name );
	if( it == members_visibility.end() )
		return ClassMemberVisibility::Public;
	return it->second;
}

void Class::SetMemberVisibility( const std::string_view member_name, const ClassMemberVisibility visibility )
{
	if( visibility == ClassMemberVisibility::Public )
		return;
	members_visibility[ member_name ]= visibility;
}

bool Class::HasAncestor( const ClassPtr class_ ) const
{
	for( const auto& parent : parents )
	{
		if( parent.class_ == class_ )
			return true;
		if( parent.class_->HasAncestor( class_ ) )
			return true;
	}
	return false;
}

void Class::AppendToString( std::string& s ) const
{
	if( const auto typeinfo_class_description= std::get_if<TypeinfoClassDescription>( &generated_class_data ) )
	{
		s+= Keyword(Keywords::typeof_);
		s+= "(";
		s+= Keyword(Keywords::typeinfo_);
		s+= "</";
		typeinfo_class_description->source_type.AppendToString( s );
		s+= "/>";
		s+=")";
	}
	else if( const auto base_template= std::get_if< Class::BaseTemplate >( &generated_class_data ) )
	{
		// Skip template parameters namespace.
		const NamesScope* parent_scope= members->GetParent()->GetParent();
		if( parent_scope->GetParent() != nullptr )
		{
			parent_scope->AppendToString( s );
			s+= "::";
		}

		const std::string& class_name= base_template->class_template->syntax_element->name;
		s+= class_name;
		s+= "</";
		for( const TemplateArg& arg : base_template->signature_args )
		{
			if( const Type* const param_as_type = std::get_if<Type>( &arg ) )
				param_as_type->AppendToString( s );
			else if( const auto param_as_variable= std::get_if<TemplateVariableArg>( &arg ) )
				AppendConstantVariableToString( *param_as_variable, s );
			else if( const auto param_as_type_template= std::get_if<TypeTemplatePtr>( &arg ) )
				(*param_as_type_template)->AppendToString( s );
			else U_ASSERT(false);

			if( &arg != &base_template->signature_args.back() )
				s+= ", ";
		}
		s+= "/>";
	}
	else if( const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &generated_class_data ) )
	{
		switch( coroutine_type_description->kind )
		{
		case CoroutineKind::Generator:
			s+= Keyword( Keywords::generator_ );
			break;
		case CoroutineKind::AsyncFunc:
			s+= Keyword( Keywords::async_ );
			break;
		}

		// TODO - print inner references here.

		s+= " ";

		if( coroutine_type_description->non_sync )
			s+= Keyword( Keywords::non_sync_ );

		s+= ": ";

		coroutine_type_description->return_type.AppendToString( s );

		if( coroutine_type_description->return_value_type == ValueType::Value )
		{}
		else if( coroutine_type_description->return_value_type == ValueType::ReferenceImut )
		{
			s+= " &";
			s+= Keyword( Keywords::imut_ );
		}
		else if( coroutine_type_description->return_value_type == ValueType::ReferenceMut )
		{
			s+= " &";
			s+= Keyword( Keywords::mut_ );
		}
		else U_ASSERT(false);
	}
	else
		members->AppendToString( s );
}

} // namespace U
