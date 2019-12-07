#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"

#include "code_builder_types.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

bool Function::PointerCanBeConvertedTo( const Function& other ) const
{
	const Function&  src_function_type= *this;
	const Function& dst_function_type= other;
	if( src_function_type.return_type != dst_function_type.return_type ||
		src_function_type.return_value_is_reference != dst_function_type.return_value_is_reference )
		return false;

	if( !src_function_type.return_value_is_mutable && dst_function_type.return_value_is_mutable )
		return false; // Allow mutability conversions, except mut->imut

	if( src_function_type.args.size() != dst_function_type.args.size() )
		return false;
	for( size_t i= 0u; i < src_function_type.args.size(); ++i )
	{
		if( src_function_type.args[i].type != dst_function_type.args[i].type ||
			src_function_type.args[i].is_reference != dst_function_type.args[i].is_reference )
			return false;

		if( src_function_type.args[i].is_mutable && !dst_function_type.args[i].is_mutable )
			return false; // Allow mutability conversions, except mut->imut
	}

	// We can convert function, returning less references to function, returning more referenes.
	for( const Function::ArgReference& src_inner_arg_reference : src_function_type.return_references )
	{
		bool found= false;
		for( const Function::ArgReference& dst_inner_arg_reference : dst_function_type.return_references )
		{
			if( dst_inner_arg_reference == src_inner_arg_reference )
			{
				found= true;
				break;
			}
		}
		if( !found )
			return false;
	}

	// We can convert function, linkink less references to function, linking more references
	for( const Function::ReferencePollution& src_pollution : src_function_type.references_pollution )
	{
		 // TODO - maybe compare with mutability conversion possibility?
		if( dst_function_type.references_pollution.count(src_pollution) == 0u )
			return false;
	}

	if( src_function_type.unsafe && !dst_function_type.unsafe )
		return false; // Conversion from unsafe to safe function is forbidden.

	// Finally, we check all conditions
	return true;
}

bool Function::ReferencePollution::operator==( const ReferencePollution& other ) const
{
	return this->dst == other.dst && this->src == other.src && this->src_is_mutable == other.src_is_mutable;
}

bool Function::ReferencePollution::operator<( const ReferencePollution& other ) const
{
	// Order is significant, because references pollution is part of stable function type.
	if( this->dst != other.dst )
		return this->dst < other.dst;
	if( this->src != other.src )
		return this->src < other.src;
	return this->src_is_mutable < other.src_is_mutable;
}

bool FunctionVariable::VirtuallyEquals( const FunctionVariable& other ) const
{
	U_ASSERT( this->is_this_call && other.is_this_call );

	const Function& l_type= *this->type.GetFunctionType();
	const Function& r_type= *other.type.GetFunctionType();

	return
		l_type.return_type == r_type.return_type &&
		l_type.return_value_is_reference == r_type.return_value_is_reference &&
		l_type.return_value_is_mutable == r_type.return_value_is_mutable &&
		l_type.return_references == r_type.return_references &&
		l_type.references_pollution == r_type.references_pollution &&
		l_type.unsafe == r_type.unsafe &&
		l_type.args.size() == r_type.args.size() &&
		std::equal( l_type.args.begin() + 1, l_type.args.end(), r_type.args.begin() + 1 );  // Compare args, except first.
}

//
// Class
//

Class::Class( const ProgramString& in_name, NamesScope* const parent_scope )
	: members( in_name, parent_scope )
{}

Class::~Class()
{}

ClassMemberVisibility Class::GetMemberVisibility( const ProgramString& member_name ) const
{
	const auto it= members_visibility.find( member_name );
	if( it == members_visibility.end() )
		return ClassMemberVisibility::Public;
	return it->second;
}

void Class::SetMemberVisibility( const ProgramString& member_name, const ClassMemberVisibility visibility )
{
	if( visibility == ClassMemberVisibility::Public )
		return;
	members_visibility[member_name]= visibility;
}

Enum::Enum( const ProgramString& in_name, NamesScope* const parent_scope )
	: members( in_name, parent_scope )
{}

ClassField::ClassField( const ClassProxyPtr& in_class, Type in_type, const unsigned int in_index, const bool in_is_mutable, const bool in_is_reference )
	: type(std::move(in_type)), class_(in_class), index(in_index), is_mutable(in_is_mutable), is_reference(in_is_reference)
{}

ArgOverloadingClass GetArgOverloadingClass( const bool is_reference, const bool is_mutable )
{
	if( is_reference && is_mutable )
		return ArgOverloadingClass::MutalbeReference;
	return ArgOverloadingClass::ImmutableReference;
}

ArgOverloadingClass GetArgOverloadingClass( const ValueType value_type )
{
	switch( value_type )
	{
	case ValueType::Value:
	case ValueType::ConstReference:
		return ArgOverloadingClass::ImmutableReference;

	case ValueType::Reference:
		return ArgOverloadingClass::MutalbeReference;
	};

	U_ASSERT(false);
	return ArgOverloadingClass::ImmutableReference;
}

ArgOverloadingClass GetArgOverloadingClass( const Function::Arg& arg )
{
	return GetArgOverloadingClass( arg.is_mutable, arg.is_reference );
}

NamesScope::NamesScope( ProgramString name, NamesScope* const parent )
	: name_(std::move(name) )
	, parent_(parent)
{}

bool NamesScope::IsAncestorFor( const NamesScope& other ) const
{
	const NamesScope* n= other.parent_;
	while( n != nullptr )
	{
		if( this == n )
			return true;
		n= n->parent_;
	}

	return false;
}

const ProgramString& NamesScope::GetThisNamespaceName() const
{
	return name_;
}

void NamesScope::SetThisNamespaceName( ProgramString name )
{
	name_= std::move(name);
}

ProgramString NamesScope::ToString() const
{
	if( parent_ == nullptr ) // Global namespace have no name.
		return ""_SpC;
	if( parent_->parent_ == nullptr )
		return name_;
	return parent_->ToString() + "::"_SpC + name_;
}

Value* NamesScope::AddName(
	const ProgramString& name,
	Value value )
{
	U_ASSERT( iterating_ == 0u );
	auto it_bool_pair=
		names_map_.insert(
			std::make_pair(
				llvm::StringRef( reinterpret_cast<const char*>(name.data()), name.size() * sizeof(sprache_char) ),
				std::move( value ) ) );

	if( it_bool_pair.second )
	{
		max_key_size_= std::max( max_key_size_, name.size() );
		return &it_bool_pair.first->second;
	}

	return nullptr;
}

Value* NamesScope::GetThisScopeValue( const ProgramString& name )
{
	const auto it= names_map_.find(
		llvm::StringRef( reinterpret_cast<const char*>(name.data()), name.size() * sizeof(sprache_char) ) );
	if( it != names_map_.end() )
		return &it->second;
	return nullptr;
}

const Value* NamesScope::GetThisScopeValue( const ProgramString& name ) const
{
	return const_cast<NamesScope*>(this)->GetThisScopeValue( name );
}

NamesScope* NamesScope::GetParent()
{
	return parent_;
}

const NamesScope* NamesScope::GetParent() const
{
	return parent_;
}

NamesScope* NamesScope::GetRoot()
{
	NamesScope* root= this;
	while( root->parent_ != nullptr )
		root= root->parent_;
	return root;
}

const NamesScope* NamesScope::GetRoot() const
{
	const NamesScope* root= this;
	while( root->parent_ != nullptr )
		root= root->parent_;
	return root;
}

void NamesScope::SetParent( NamesScope* const parent )
{
	parent_= parent;
}

void NamesScope::AddAccessRightsFor( const ClassProxyPtr& class_, const ClassMemberVisibility visibility )
{
	access_rights_[class_]= visibility;
}

ClassMemberVisibility NamesScope::GetAccessFor( const ClassProxyPtr& class_ ) const
{
	const auto it= access_rights_.find(class_);
	const auto this_rights= it == access_rights_.end() ? ClassMemberVisibility::Public : it->second;
	const auto parent_rights= parent_ == nullptr ? ClassMemberVisibility::Public : parent_->GetAccessFor( class_ );
	return std::max( this_rights, parent_rights );
}

void NamesScope::CopyAccessRightsFrom( const NamesScope& src )
{
	access_rights_= src.access_rights_;
}

void NamesScope::SetErrors( CodeBuilderErrorsContainer& errors )
{
	errors_= &errors;
}

CodeBuilderErrorsContainer& NamesScope::GetErrors() const
{
	if( errors_ != nullptr )
		return *errors_;
	return parent_->GetErrors();
}

//
// DeducedTemplateParameter
//

DeducedTemplateParameter::Array::Array( const Array& other )
{
	*this= other;
}

DeducedTemplateParameter::Array& DeducedTemplateParameter::Array::operator=( const Array& other )
{
	size.reset( new DeducedTemplateParameter( *other.size ) );
	type.reset( new DeducedTemplateParameter( *other.type ) );
	return *this;
}

DeducedTemplateParameter::Function::Function( const Function& other )
{
	*this= other;
}

DeducedTemplateParameter::Function& DeducedTemplateParameter::Function::operator=( const Function& other )
{
	return_type.reset( new DeducedTemplateParameter( *other.return_type ) );
	argument_types= other.argument_types;
	return *this;
}

DeducedTemplateParameter::DeducedTemplateParameter( Invalid invalid )
{
	something_= std::move(invalid);
}

DeducedTemplateParameter::DeducedTemplateParameter( Type type )
{
	something_= std::move(type);
}

DeducedTemplateParameter::DeducedTemplateParameter( Variable variable )
{
	something_= std::move(variable);
}

DeducedTemplateParameter::DeducedTemplateParameter( TemplateParameter template_parameter )
{
	something_= std::move(template_parameter);
}

DeducedTemplateParameter::DeducedTemplateParameter( Array array )
{
	something_= std::move(array);
}

DeducedTemplateParameter::DeducedTemplateParameter( Tuple tuple )
{
	something_= std::move(tuple);
}

DeducedTemplateParameter::DeducedTemplateParameter( Function function )
{
	something_= std::move(function);
}

DeducedTemplateParameter::DeducedTemplateParameter( Template template_ )
{
	something_= std::move(template_);
}

bool DeducedTemplateParameter::IsInvalid() const
{
	return std::get_if<Invalid>( &something_ ) != nullptr;
}

bool DeducedTemplateParameter::IsType() const
{
	return std::get_if<Type>( &something_ ) != nullptr;
}

bool DeducedTemplateParameter::IsVariable() const
{
	return std::get_if<Variable>( &something_ ) != nullptr;
}

bool DeducedTemplateParameter::IsTemplateParameter() const
{
	return std::get_if<TemplateParameter>( &something_ ) != nullptr;
}

const DeducedTemplateParameter::Array* DeducedTemplateParameter::GetArray() const
{
	return std::get_if<Array>( &something_ );
}

const DeducedTemplateParameter::Tuple* DeducedTemplateParameter::GetTuple() const
{
	return std::get_if<Tuple>( &something_ );
}

const DeducedTemplateParameter::Function* DeducedTemplateParameter::GetFunction() const
{
	return std::get_if<Function>( &something_ );
}

const DeducedTemplateParameter::Template* DeducedTemplateParameter::GetTemplate() const
{
	return std::get_if<Template>( &something_ );
}

//
//
//

const ProgramString g_invalid_type_name= "InvalidType"_SpC;

const ProgramString& GetFundamentalTypeName( const U_FundamentalType type )
{
	switch(type)
	{
	case U_FundamentalType::InvalidType: return g_invalid_type_name;
	case U_FundamentalType::Void: return Keyword( Keywords::void_ );
	case U_FundamentalType::Bool: return Keyword( Keywords::bool_ );
	case U_FundamentalType::i8 : return Keyword( Keywords::i8_ );
	case U_FundamentalType::u8 : return Keyword( Keywords::u8_ );
	case U_FundamentalType::i16: return Keyword( Keywords::i16_ );
	case U_FundamentalType::u16: return Keyword( Keywords::u16_ );
	case U_FundamentalType::i32: return Keyword( Keywords::i32_ );
	case U_FundamentalType::u32: return Keyword( Keywords::u32_ );
	case U_FundamentalType::i64: return Keyword( Keywords::i64_ );
	case U_FundamentalType::u64: return Keyword( Keywords::u64_ );
	case U_FundamentalType::i128: return Keyword( Keywords::i128_ );
	case U_FundamentalType::u128: return Keyword( Keywords::u128_ );
	case U_FundamentalType::f32: return Keyword( Keywords::f32_ );
	case U_FundamentalType::f64: return Keyword( Keywords::f64_ );
	case U_FundamentalType::char8 : return Keyword( Keywords::char8_  );
	case U_FundamentalType::char16: return Keyword( Keywords::char16_ );
	case U_FundamentalType::char32: return Keyword( Keywords::char32_ );
	case U_FundamentalType::LastType: break;
	};

	U_ASSERT( false );
	return g_invalid_type_name;
}

} //namespace CodeBuilderPrivate

} // namespace U
