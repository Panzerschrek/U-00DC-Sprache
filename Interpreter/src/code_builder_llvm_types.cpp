#include "assert.hpp"

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
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Void, sizeof(U_bool) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i8 , sizeof(U_i8 ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u8 , sizeof(U_u8 ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i16, sizeof(U_i16) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u16, sizeof(U_u16) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i32, sizeof(U_i32) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u32, sizeof(U_u32) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i64, sizeof(U_i64) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u64, sizeof(U_u64) ),
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
		return sizeof(FuncNumber);

	case Kind::Array:
		return array->type.SizeOf() * array->size;

	case Kind::Class:
		return class_->size;
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

} //namespace CodeBuilderLLVMPrivate

} // namespace Interpreter
