#include "../lex_synt_lib/assert.hpp"
#include "references_graph.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void ReferencesGraph::AddNode( ReferencesGraphNodePtr node )
{
	U_ASSERT( nodes_.count(node) == 0 );
	nodes_.emplace( std::move(node), NodeState() );
}

void ReferencesGraph::RemoveNode( const ReferencesGraphNodePtr& node )
{
	U_ASSERT( nodes_.count(node) != 0 );

	// Collect in/out nodes.
	std::vector<ReferencesGraphNodePtr> in_nodes;
	std::vector<ReferencesGraphNodePtr> out_nodes;
	for( const auto& link : links_ )
	{
		if( link.first == node )
			out_nodes.push_back( link.second );
		if( link.second == node )
			in_nodes.push_back( link.first );
	}

	// Remove links.
	for( size_t i= 0u; i < links_.size(); )
	{
		if( links_[i].first == node || links_[i].second == node )
		{
			if( i != links_.size() - 1u )
				links_[i]= links_.back();
			links_.pop_back();
		}
		else
			++i;
	}

	// Erase node.
	nodes_.erase(node);

	// Create new links.
	for( const ReferencesGraphNodePtr& from : in_nodes )
		for( const ReferencesGraphNodePtr& to : out_nodes )
			AddLink( from, to );
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

bool ReferencesGraph::HaveOutgoingMutableNodes( const ReferencesGraphNodePtr& from ) const
{
	for( const auto& link : links_ )
	{
		if( link.first == from && link.second->kind == ReferencesGraphNode::Kind::ReferenceMut  )
			return true;
	}

	return false;
}

void ReferencesGraph::MoveNode( const ReferencesGraphNodePtr& node )
{
	U_ASSERT( nodes_.count(node) != 0 );

	NodeState& node_state= nodes_[node];
	U_ASSERT( !node_state.moved );

	node_state.moved= true;

	// TODO - remove links between nodes?
}

bool ReferencesGraph::NodeMoved( const ReferencesGraphNodePtr& node ) const
{
	const auto it= nodes_.find(node);
	U_ASSERT( it != nodes_.end() );

	return it->second.moved;
}

} // namespace CodeBuilderPrivate

} // namespace U
