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
		return std::visit( []( const auto& el ) { return HasTailImpl(el); }, start_.val );
	}

	template<typename T>
	std::optional<T> TryTakeStart()
	{
		if( const auto v= std::get_if< NodePtr<T> >( &start_.val ) )
			return std::move( (*v)->payload );
		return std::nullopt;
	}

	// Iterate from start to end, applying given function to all stored values.
	template< typename Func >
	void Iter( Func&& func ) const
	{
		const VariantElement* cur= &start_;
		while( cur != nullptr )
			cur= std::visit( [&]( const auto& el ){ return IterElement( el, func ); }, cur->val );
	}

private:
	struct EmptyNode{}; // Indicate list end with it.

	struct VariantElement;

	template<typename T>
	struct Node
	{
		T payload;
		VariantElement next;
	};

	template<typename T>
	using NodePtr= std::unique_ptr< Node<T> >;

	// Wrap variant element into struct, instead of unsing type alias for variant< ... >.
	// Doing so we prevent quadratic complexity of mangled names construction.
	// Such trick significantly reduces compilation type and debug binaries size.
	struct VariantElement
	{
		std::variant< EmptyNode, NodePtr<ContainedTypes> ... > val;
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
			tail_->val= std::make_unique< NodeT >( NodeT{ std::move(t), VariantElement{} } );
			tail_= & std::get< std::unique_ptr< NodeT > >( tail_->val )->next;
		}

		// Append other list and make sure insertion position is at last element of that list.
		void AppendList( VariantLinkedList other_list )
		{
			*tail_= std::move(other_list.start_);
			tail_= GetListTail( *tail_ );
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
		if( std::holds_alternative< EmptyNode >( node.val ) )
			return &node;

		return std::visit( [](auto& el ){ return GetListTailImpl(el); }, node.val );
	}

	static bool HasTailImpl( const EmptyNode& )
	{
		return false;
	}

	template<typename T>
	static bool HasTailImpl( const NodePtr<T>& node )
	{
		return std::get_if< EmptyNode >( &node->next.val ) == nullptr;
	}

	template< typename Func >
	static const VariantElement* IterElement( const EmptyNode&, Func& func )
	{
		(void)func;
		return nullptr;
	}

	template< typename T, typename Func >
	static const VariantElement* IterElement( const NodePtr<T>& el, Func& func )
	{
		func( el->payload );
		return &el->next;
	}

private:
	VariantElement start_;
};

} // namespace U
