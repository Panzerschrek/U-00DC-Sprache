#include "assert.hpp"
#include "keywords.hpp"

#include "code_builder_llvm_types.hpp"

namespace Interpreter
{

namespace CodeBuilderLLVMPrivate
{

namespace
{

const size_t g_fundamental_types_size[ size_t(U_FundamentalType::LastType) ]=
{
	U_DESIGNATED_INITIALIZER( U_FundamentalType::InvalidType, 0u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Void, 0 ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Bool, sizeof(U_bool) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i8 , sizeof(U_i8 ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u8 , sizeof(U_u8 ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i16, sizeof(U_i16) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u16, sizeof(U_u16) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i32, sizeof(U_i32) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u32, sizeof(U_u32) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i64, sizeof(U_i64) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u64, sizeof(U_u64) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f32, sizeof(U_f32) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f64, sizeof(U_f64) ),
};

const char g_invalid_type_name_ascii[]= "InvalidType";
const ProgramString g_invalid_type_name= ToProgramString( g_invalid_type_name_ascii );

const char* const g_fundamental_types_names_ascii[ size_t(U_FundamentalType::LastType) ]=
{
	U_DESIGNATED_INITIALIZER( U_FundamentalType::InvalidType, g_invalid_type_name_ascii ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Void,  KeywordAscii( Keywords::void_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Bool, KeywordAscii( Keywords::bool_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i8 , KeywordAscii( Keywords::i8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u8 , KeywordAscii( Keywords::u8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i16, KeywordAscii( Keywords::i16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u16, KeywordAscii( Keywords::u16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i32, KeywordAscii( Keywords::i32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u32, KeywordAscii( Keywords::u32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i64, KeywordAscii( Keywords::i64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u64, KeywordAscii( Keywords::u64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f32, KeywordAscii( Keywords::f32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f64, KeywordAscii( Keywords::f64_ ) ),
};

const ProgramString (&g_fundamental_types_names)[ size_t(U_FundamentalType::LastType) ]=
{
	U_DESIGNATED_INITIALIZER( U_FundamentalType::InvalidType, g_invalid_type_name ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Void,  Keyword( Keywords::void_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Bool, Keyword( Keywords::bool_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i8 , Keyword( Keywords::i8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u8 , Keyword( Keywords::u8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i16, Keyword( Keywords::i16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u16, Keyword( Keywords::u16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i32, Keyword( Keywords::i32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u32, Keyword( Keywords::u32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i64, Keyword( Keywords::i64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u64, Keyword( Keywords::u64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f32, Keyword( Keywords::f32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f64, Keyword( Keywords::f64_ ) ),
};

} // namespace

Type::Type( U_FundamentalType in_fundamental )
	: kind( Kind::Fundamental )
	, fundamental( in_fundamental )
{}

Type::Type( const Type& other )
{
	*this= other;
}

Type::Type( Type&& other ) noexcept
{
	*this= std::move(other);
}

Type& Type::operator=( const Type& other )
{
	kind= other.kind;

	switch( kind )
	{
	case Kind::Fundamental:
		fundamental= other.fundamental;
		fundamental_llvm_type= other.fundamental_llvm_type;
		break;

	case Kind::Function:
		U_ASSERT( other.function );
		function.reset( new Function( *other.function ) );
		break;

	case Kind::Array:
		U_ASSERT( other.array );
		array.reset( new Array( *other.array ) );
		break;

	case Kind::Class:
		U_ASSERT( other.class_ );
		class_= other.class_;
	};

	return *this;
}

Type& Type::operator=( Type&& other ) noexcept
{
	kind= other.kind;
	fundamental= other.fundamental;
	function= std::move( other.function );
	array= std::move( other.array );
	class_= std::move( other.class_ );

	fundamental_llvm_type= other.fundamental_llvm_type;
	other.fundamental_llvm_type= nullptr;

	return *this;
}

size_t Type::SizeOf() const
{
	switch( kind )
	{
	case Kind::Fundamental:
		return g_fundamental_types_size[ size_t( fundamental ) ];

	case Kind::Function:
		U_ASSERT( false && "SizeOf method not supported for functions." );
		return 1u;

	case Kind::Array:
		return array->type.SizeOf() * array->size;

	case Kind::Class:
		U_ASSERT( false && "SizeOf method not supported for classes." );
		return 1u;
	};

	U_ASSERT(false);
	return 0;
}

llvm::Type* Type::GetLLVMType() const
{
	switch( kind )
	{
	case Kind::Fundamental:
		return fundamental_llvm_type;
	case Kind::Function:
		return function->llvm_function_type;
	case Kind::Array:
		return array->llvm_type;
	case Kind::Class:
		return class_->llvm_type;
	};

	U_ASSERT(false);
	return nullptr;
}

ProgramString Type::ToString() const
{
	switch( kind )
	{
	case Kind::Fundamental:
		return GetFundamentalTypeName( fundamental );

	case Kind::Function:
	{
		U_ASSERT( function != nullptr );
		ProgramString result;
		result+= "fn "_SpC;
		result+= function->return_type.ToString();
		result+= " ( "_SpC;
		for( const Function::Arg& arg : function->args )
		{
			if( arg.is_reference )
				result+= "&"_SpC;
			if( arg.is_mutable )
				result+= "mut "_SpC;
			else
				result+= "imut "_SpC;

			result+= arg.type.ToString();
			if( &arg != &function->args.back() )
				result+= ", "_SpC;
		}
		result+= " )"_SpC;

		return result;
	}

	case Kind::Array:
		U_ASSERT( array != nullptr );
		return
			"[ "_SpC + array->type.ToString() + ", "_SpC +
			ToProgramString( std::to_string( array->size ).c_str() ) + " ]"_SpC;

	case Kind::Class:
		U_ASSERT( class_ != nullptr );
		return "class "_SpC + class_->name;
	};

	U_ASSERT(false);
	return ""_SpC;
}

bool operator==( const Type& r, const Type& l )
{
	if( r.kind != l.kind )
		return false;

	switch( r.kind )
	{
	case Type::Kind::Fundamental:
		return r.fundamental == l.fundamental;

	case Type::Kind::Function:
		U_ASSERT( r.kind == Type::Kind::Function );
		U_ASSERT( r.function );
		U_ASSERT( l.function );

		return *l.function == *r.function;

	case Type::Kind::Array:
		return r.array->size == l.array->size && r.array->type == l.array->type;

	case Type::Kind::Class:
		return r.class_ == l.class_;
	};

	U_ASSERT(false);
	return false;
}

bool operator!=( const Type& r, const Type& l )
{
	return !( r == l );
}

bool operator==( const Function::Arg& r, const Function::Arg& l )
{
	return r.type == l.type && r.is_mutable == l.is_mutable && r.is_reference == l.is_reference;
}

bool operator!=( const Function::Arg& r, const Function::Arg& l )
{
	return !( r == l );
}

bool operator==( const Function& r, const Function& l )
{
	return
		r.return_type == l.return_type &&
		r.args == l.args;
}

bool operator!=( const Function& r, const Function& l )
{
	return !( r == l );
}

Class::Class()
{}

Class::~Class()
{}

const Class::Field* Class::GetField( const ProgramString& name )
{

	for( const Field& field : fields )
	{
		if( field.name == name )
			return &field;
	}

	return nullptr;
}

NamesScope::NamesScope( const NamesScope* prev )
	: prev_(prev)
{}

const NamesScope::InsertedName* NamesScope::AddName(
		const ProgramString& name,
		Variable variable )
{
	return AddName( name, Name{ nullptr, std::move( variable ) } );
}

const NamesScope::InsertedName* NamesScope::AddName(
	const ProgramString& name,
	const ClassPtr& class_ )
{
	return AddName( name, Name{ class_, Variable() } );
}

const NamesScope::InsertedName* NamesScope::AddName(
	const ProgramString& name,
	const Name name_value )
{
	auto it_bool_pair = names_map_.emplace( name, std::move( name_value ) );
	if( it_bool_pair.second )
		return &*it_bool_pair.first;

	return nullptr;
}

const NamesScope::InsertedName*
	NamesScope::GetName(
		const ProgramString& name ) const
{
	auto it= names_map_.find( name );
	if( it != names_map_.end() )
		return &*it;

	if( prev_ != nullptr )
		return prev_->GetName( name );

	return nullptr;
}

const ProgramString& GetFundamentalTypeName( const U_FundamentalType fundamental_type )
{
	if( fundamental_type >= U_FundamentalType::LastType )
		return g_invalid_type_name;

	return g_fundamental_types_names[ size_t( fundamental_type ) ];
}

const char* GetFundamentalTypeNameASCII( const U_FundamentalType fundamental_type )
{
	if( fundamental_type >= U_FundamentalType::LastType )
		return g_invalid_type_name_ascii;

	return g_fundamental_types_names_ascii[ size_t( fundamental_type ) ];
}

} //namespace CodeBuilderLLVMPrivate

} // namespace Interpreter
