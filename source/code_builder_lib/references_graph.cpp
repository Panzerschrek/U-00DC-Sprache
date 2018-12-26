#include "../lex_synt_lib/assert.hpp"
#include "references_graph.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void ReferencesGraph::AddNode( ReferencesGraphNodePtr node )
{
	U_ASSERT( nodes_.count(node) == 0 );
	nodes_.insert( std::move(node) );
}

void ReferencesGraph::RemoveNode( const ReferencesGraphNodePtr& node )
{
	U_ASSERT( nodes_.count(node) != 0 );
	nodes_.erase(node);
}

void ReferencesGraph::AddLink( const ReferencesGraphNodePtr& from, const ReferencesGraphNodePtr& to )
{
	U_ASSERT( nodes_.count(from) != 0 );
	U_ASSERT( nodes_.count(to  ) != 0 );

	for( const auto& link : links_ )
	{
		if( link.first == from && link.second == to )
			return; // Link already exists.
	}

	links_.emplace_back( from, to );
}

void ReferencesGraph::RemoveLink( const ReferencesGraphNodePtr& from, const ReferencesGraphNodePtr& to )
{
	U_ASSERT( nodes_.count(from) != 0 );
	U_ASSERT( nodes_.count(to  ) != 0 );

	for( size_t i= 0u; i < links_.size(); ++i )
	{
		auto& link= links_[i];
		if( link.first == from && link.second == to )
		{
			if( &link != & links_.back() )
				link= links_.back();
			return;
		}
	}

	U_ASSERT(false); // Removing unexistent link.
}


bool ReferencesGraph::HaveOutgoingLinks( const ReferencesGraphNodePtr& from ) const
{
	for( const auto& link : links_ )
	{
		if( link.first == from )
			return true;
	}

	return false;
}

} // namespace CodeBuilderPrivate

} // namespace U
