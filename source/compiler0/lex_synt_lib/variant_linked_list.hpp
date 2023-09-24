#pragma once
#include <variant>
#include <memory>

namespace U
{

// Single-direction (relaitvely) compact linked list of different type values.
// Each value is stored in separate allocation.
// Size is not stored.
template< typename ... ContainedTypes>
class VariantLinkedList
{
public:
	bool HasTail() const
	{
		return std::visit( []( const auto& el ) { return HasTailImpl(el); }, start_ );
	}

	template<typename T>
	std::optional<T> TryTakeStart()
	{
		if( const auto v= std::get_if< NodePtr<T> >( &start_ ) )
			return std::move( (*v)->payload );
		return std::nullopt;
	}

	// Iterate from start to end, applying given function to all stored values.
	template< typename Func >
	void Iter( const Func& func ) const
	{
		const VariantElement* cur= &start_;
		while( cur != nullptr )
			cur= std::visit( [&]( const auto& el ){ return IterElement( el, func ); }, *cur );
	}

private:
	struct EmptyNode{}; // Indicate list end with it.

	template<typename T>
	struct Node;

	template<typename T>
	using NodePtr= std::unique_ptr< Node<T> >;

	using VariantElement= std::variant< EmptyNode, NodePtr<ContainedTypes> ... >;

	template<typename T>
	struct Node
	{
		T payload;
		VariantElement next;
	};

public:
	// Helper builder class, that allows to apped values to the end of the list.
	class Builder
	{
	public:
		Builder()
			: tail_(&result_.start_)
		{}

		// This class stores raw pointer to itself. So, disable any move.
		Builder( const Builder& )= delete;
		Builder( Builder&& )= delete;
		Builder& operator=( const Builder& )= delete;
		Builder& operator=( Builder&& )= delete;

		template<typename T>
		void Append( T t )
		{
			using NodeT= Node<T>;
			*tail_= std::make_unique< NodeT >( NodeT{ std::move(t), EmptyNode{} } );
			tail_= & std::get< std::unique_ptr< NodeT > >( *tail_ )->next;
		}

		// Append other list and make sure insertion position is at last element of that list.
		void AppendList( VariantLinkedList other_list )
		{
			const auto next_tail= GetListTail( other_list.start_ );
			*tail_= std::move(other_list.start_);
			tail_= next_tail;
		}

		// Take result, reset internal state.
		VariantLinkedList Build()
		{
			VariantLinkedList result= std::move(result_);
			tail_= &result_.start_;
			return result;
		}

	private:
		VariantLinkedList result_;
		VariantElement* tail_= nullptr;
	};

private:
	template<typename T>
	static VariantElement* GetListTailImpl( const NodePtr<T>& node )
	{
		return GetListTail( node->next );
	}

	static VariantElement* GetListTailImpl( const EmptyNode& ) { return nullptr; }

	static VariantElement* GetListTail( VariantElement& node )
	{
		if( std::get_if< EmptyNode >( &node ) != nullptr )
			return &node;

		return std::visit( [](auto& el ){ return GetListTailImpl(el); }, node );
	}

	static bool HasTailImpl( const EmptyNode& )
	{
		return false;
	}

	template<typename T>
	static bool HasTailImpl( const NodePtr<T>& node )
	{
		return std::get_if< EmptyNode >( &node->next ) == nullptr;
	}

	template< typename Func >
	static const VariantElement* IterElement( const EmptyNode&, const Func& func )
	{
		(void)func;
		return nullptr;
	}

	template< typename T, typename Func >
	static const VariantElement* IterElement( const NodePtr<T>& el, const Func& func )
	{
		func( el->payload );
		return &el->next;
	}

private:
	VariantElement start_;
};

} // namespace U
