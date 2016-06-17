#include "assert.hpp"

#include "code_builder_types.hpp"

namespace Interpreter
{

namespace CodeBuilderPrivate
{

namespace
{

const size_t g_fundamental_types_size[ size_t(U_FundamentalType::LastType) ]=
{
	[ size_t(U_FundamentalType::InvalidType) ]= 0,
	[ size_t(U_FundamentalType::Void) ]= 0,
	[ size_t(U_FundamentalType::Bool) ]= sizeof(U_bool),
	[ size_t(U_FundamentalType::i8 ) ]= sizeof(U_i8 ),
	[ size_t(U_FundamentalType::u8 ) ]= sizeof(U_u8 ),
	[ size_t(U_FundamentalType::i16) ]= sizeof(U_i16),
	[ size_t(U_FundamentalType::u16) ]= sizeof(U_u16),
	[ size_t(U_FundamentalType::i32) ]= sizeof(U_i32),
	[ size_t(U_FundamentalType::u32) ]= sizeof(U_u32),
	[ size_t(U_FundamentalType::i64) ]= sizeof(U_i64),
	[ size_t(U_FundamentalType::u64) ]= sizeof(U_u64),
	[ size_t(U_FundamentalType::f32) ]= sizeof(U_f32),
	[ size_t(U_FundamentalType::f64) ]= sizeof(U_f64),
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

Type::Type( Type&& other )
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
		break;

	case Kind::Function:
		U_ASSERT( other.function );
		function.reset( new Function( *other.function ) );
		break;

	case Kind::Array:
		U_ASSERT( other.array );
		array.reset( new Array( *other.array ) );
		break;
	};

	return *this;
}

Type& Type::operator=( Type&& other )
{
	kind= other.kind;
	fundamental= other.fundamental;
	function= std::move( other.function );
	array= std::move( other.array );
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
	};

	U_ASSERT(false);
	return 0;
}

bool operator==( const Type& r, const Type& l )
{
	if( r.kind != l.kind )
		return false;

	if( r.kind == Type::Kind::Fundamental )
		return r.fundamental == l.fundamental;

	else
	{
		U_ASSERT( r.kind == Type::Kind::Function );
		U_ASSERT( r.function );
		U_ASSERT( l.function );

		return *l.function == *r.function;
	}
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

NamesScope::NamesScope( const NamesScope* prev )
	: prev_(prev)
{}

const NamesScope::NamesMap::value_type*
	NamesScope::AddName(
		const ProgramString& name, Variable variable )
{
	auto it_bool_pair = names_map_.emplace( name, std::move( variable ) );
	if( it_bool_pair.second )
		return &*it_bool_pair.first;

	return nullptr;
}

const NamesScope::NamesMap::value_type*
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

BlockStackContext::BlockStackContext()
	: parent_context_(nullptr)
	, stack_size_(0)
	, max_reached_stack_size_(0)
{}

BlockStackContext::BlockStackContext( BlockStackContext& parent_context )
	: parent_context_( &parent_context )
	, stack_size_( parent_context.stack_size_ )
	, max_reached_stack_size_( parent_context.stack_size_ )
{}

BlockStackContext::~BlockStackContext()
{
	if( parent_context_ )
	{
		parent_context_->max_reached_stack_size_=
			std::max(
				parent_context_->max_reached_stack_size_,
				std::max(
					max_reached_stack_size_,
					stack_size_ ) );
	}
}

void BlockStackContext::IncreaseStack( unsigned int size )
{
	stack_size_+= size;
}

unsigned int BlockStackContext::GetStackSize() const
{
	return stack_size_;
}

unsigned int BlockStackContext::GetMaxReachedStackSize() const
{
	return max_reached_stack_size_;
}

void ExpressionStackSizeCounter::operator+=( unsigned int add_size )
{
	size_+= add_size;
	max_reached_size_= std::max( size_, max_reached_size_ );
}

void ExpressionStackSizeCounter::operator-=( unsigned int sub_size )
{
	U_ASSERT( sub_size <= size_ );
	size_-= sub_size;
}

unsigned int ExpressionStackSizeCounter::GetMaxReachedStackSize() const
{
	return max_reached_size_;
}

unsigned int ExpressionStackSizeCounter::GetCurrentStackSize() const
{
	return size_;
}

} //namespace CodeBuilderPrivate

} // namespace Interpreter
