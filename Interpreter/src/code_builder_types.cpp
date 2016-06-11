#include "assert.hpp"

#include "code_builder_types.hpp"

namespace Interpreter
{

namespace CodeBuilderPrivate
{

Type::Type()
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

	if( kind == Kind::Fundamental )
		fundamental= other.fundamental;
	else
		function.reset( new Function( *other.function ) );

	return *this;
}

Type& Type::operator=( Type&& other )
{
	kind= other.kind;
	fundamental= other.fundamental;
	function= std::move( other.function );
	return *this;
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
