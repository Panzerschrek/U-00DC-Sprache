#include "../compilers_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Hashing.h>
#include "../compilers_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"
#include "error_reporting.hpp"
#include "references_graph.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

bool ReferencesGraph::Link::operator==( const ReferencesGraph::Link& r ) const
{
	return this->src == r.src && this->dst == r.dst;
}

size_t ReferencesGraph::LinkHasher::operator()( const Link& link ) const
{
	return llvm::hash_combine( reinterpret_cast<uintptr_t>(link.src.get()), reinterpret_cast<uintptr_t>(link.dst.get()) );
}

void ReferencesGraph::AddNode( ReferencesGraphNodePtr node )
{
	U_ASSERT( nodes_.count(node) == 0 );
	nodes_.emplace( std::move(node), NodeState() );
}

void ReferencesGraph::RemoveNode( const ReferencesGraphNodePtr& node )
{
	U_ASSERT( nodes_.count(node) != 0 );

	if( const auto inner_reference= GetNodeInnerReference( node ) )
		RemoveNode( inner_reference );

	// Collect in/out nodes.
	std::vector<ReferencesGraphNodePtr> in_nodes;
	std::vector<ReferencesGraphNodePtr> out_nodes;
	for( const auto& link : links_ )
	{
		if( link.src == link.dst ) // Self loop link.
			continue;

		if( link.src == node )
			out_nodes.push_back( link.dst );
		if( link.dst == node )
			in_nodes.push_back( link.src );
	}

	// Remove links.
	for( auto it= links_.begin(); it != links_.end(); )
	{
		if( it->src == node || it->dst == node )
		{
			it= links_.erase(it);
		}
		else
			++it;
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

	links_.insert( Link{from, to} );
}

void ReferencesGraph::RemoveLink( const ReferencesGraphNodePtr& from, const ReferencesGraphNodePtr& to )
{
	U_ASSERT( nodes_.count(from) != 0 );
	U_ASSERT( nodes_.count(to  ) != 0 );

	const bool erased= links_.erase( Link{ from, to } ) != 0;
	(void)erased;

	U_ASSERT(erased); // Removing unexistent link.
}

ReferencesGraphNodePtr ReferencesGraph::GetNodeInnerReference( const ReferencesGraphNodePtr& node ) const
{
	const auto it= nodes_.find( node );
	U_ASSERT( it != nodes_.end() );
	return it->second.inner_reference;
}

void ReferencesGraph::SetNodeInnerReference( const ReferencesGraphNodePtr& node, ReferencesGraphNodePtr inner_reference )
{
	AddNode( inner_reference );

	const auto it= nodes_.find( node );
	U_ASSERT( it != nodes_.end() );
	U_ASSERT( it->second.inner_reference == nullptr );
	it->second.inner_reference= std::move(inner_reference);
}

bool ReferencesGraph::HaveOutgoingLinks( const ReferencesGraphNodePtr& from ) const
{
	for( const auto& link : links_ )
	{
		if( link.src == from )
			return true;
	}

	return false;
}

bool ReferencesGraph::HaveOutgoingMutableNodes( const ReferencesGraphNodePtr& from ) const
{
	for( const auto& link : links_ )
	{
		if( link.src == from && link.dst->kind == ReferencesGraphNode::Kind::ReferenceMut  )
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
	if( node_state.inner_reference != nullptr )
	{
		RemoveNode( node_state.inner_reference );
		node_state.inner_reference= nullptr;
	}

	// Remove links.
	for( auto it= links_.begin(); it != links_.end(); )
	{
		if( it->src == node || it->dst == node )
		{
			it= links_.erase(it);
		}
		else
			++it;
	}
}

bool ReferencesGraph::NodeMoved( const ReferencesGraphNodePtr& node ) const
{
	const auto it= nodes_.find(node);
	if( it == nodes_.end() ) // Can be for global constants, for example.
		return false;

	return it->second.moved;
}

ReferencesGraph::NodesSet ReferencesGraph::GetAllAccessibleInnerNodes( const ReferencesGraphNodePtr& node ) const
{
	NodesSet visited_nodes_set, result_set;
	GetAllAccessibleInnerNodes_r( node, visited_nodes_set, result_set );
	return result_set;
}

ReferencesGraph::NodesSet ReferencesGraph::GetAllAccessibleVariableNodes( const ReferencesGraphNodePtr& node ) const
{
	NodesSet visited_nodes_set, result_set;
	GetAllAccessibleVariableNodes_r( node, visited_nodes_set, result_set );
	return result_set;
}

void ReferencesGraph::GetAllAccessibleInnerNodes_r(
	const ReferencesGraphNodePtr& node,
	NodesSet& visited_nodes_set,
	NodesSet& result_set ) const
{
	U_ASSERT( nodes_.find(node) != nodes_.end() );

	if( !visited_nodes_set.insert(node).second )
		return; // Already visited

	if( const ReferencesGraphNodePtr inner_reference= GetNodeInnerReference( node ) )
	{
		result_set.emplace( inner_reference );
		GetAllAccessibleInnerNodes_r( inner_reference, visited_nodes_set, result_set );
	}

	for( const auto& link : links_ )
		if( link.dst == node )
			GetAllAccessibleInnerNodes_r( link.src, visited_nodes_set, result_set );

}

void ReferencesGraph::GetAllAccessibleVariableNodes_r(
	const ReferencesGraphNodePtr& node,
		NodesSet& visited_nodes_set,
		NodesSet& result_set ) const
{
	U_ASSERT( nodes_.find(node) != nodes_.end() );

	if( !visited_nodes_set.insert(node).second )
		return; // Already visited

	if( node->kind == ReferencesGraphNode::Kind::Variable )
		result_set.emplace( node );

	for( const auto& link : links_ )
		if( link.dst == node )
			GetAllAccessibleVariableNodes_r( link.src, visited_nodes_set, result_set );
}

ReferencesGraph::MergeResult ReferencesGraph::MergeVariablesStateAfterIf( const std::vector<ReferencesGraph>& branches_variables_state, const FilePos& file_pos )
{
	ReferencesGraph result;
	std::vector<CodeBuilderError> errors;

	std::vector< std::pair<ReferencesGraphNodePtr, ReferencesGraphNodePtr> > replaced_nodes; // First node replaced with second node.
	for( const ReferencesGraph& branch_state : branches_variables_state )
	{
		for( const auto& node_pair : branch_state.nodes_ )
		{
			if( result.nodes_.find( node_pair.first ) == result.nodes_.end() )
				result.nodes_[ node_pair.first ]= node_pair.second;

			const NodeState& src_state= node_pair.second;
			NodeState& result_state= result.nodes_[ node_pair.first ];

			if( result_state.moved != src_state.moved )
				REPORT_ERROR( ConditionalMove, errors, file_pos, node_pair.first->name );

				 if( result_state.inner_reference == nullptr && src_state.inner_reference == nullptr ) {}
			else if( result_state.inner_reference == nullptr && src_state.inner_reference != nullptr )
			{
				result.nodes_[ result_state.inner_reference ]= NodeState();
				result_state.inner_reference= src_state.inner_reference;
			}
			else if( result_state.inner_reference != nullptr && src_state.inner_reference == nullptr ) {}
			else // both nonnull
			{
				// Variable inner reference created in multiple braches.

				// If linked as mutable and as immutable in different branches - result is mutable.
				if( ( result_state.inner_reference->kind != ReferencesGraphNode::Kind::ReferenceMut && src_state.inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut ) )
					replaced_nodes.emplace_back( result_state.inner_reference, src_state.inner_reference );
				// else - remove duplicated nodes with same kind.
				else if( result_state.inner_reference != src_state.inner_reference )
					replaced_nodes.emplace_back( src_state.inner_reference, result_state.inner_reference );
			}
		}

		for( const auto& src_link : branch_state.links_ )
		{
			if( std::find( result.links_.begin(), result.links_.end(), src_link ) == result.links_.end() )
				result.links_.insert( src_link );
		}
	}

	if( !replaced_nodes.empty() )
	{
		LinksSet links_corrected;
		for( auto link : result.links_ )
		{
			for( const auto& replaced_node : replaced_nodes )
			{
				if( link.src == replaced_node.first )
					link.src = replaced_node.second;
				if( link.dst == replaced_node.first )
					link.dst= replaced_node.second;
			}
			links_corrected.insert(link);
		}
		result.links_= std::move(links_corrected);
	}

	for( const auto& replaced_node_pair : replaced_nodes )
		result.nodes_.erase( replaced_node_pair.first );

	// Check mutable reference count correctness.
	for( const auto& node : result.nodes_ )
	{
		size_t mutable_links_count= 0u;
		size_t immutable_links_count= 0u;
		for( const auto& link : result.links_ )
		{
			if( link.src == node.first )
			{
				if( link.dst->kind == ReferencesGraphNode::Kind::ReferenceMut )
					++mutable_links_count;
				else
					++immutable_links_count;
			}
		}
		if( mutable_links_count > 1u || ( immutable_links_count > 0u && mutable_links_count > 0u ) )
			REPORT_ERROR( ReferenceProtectionError, errors, file_pos, node.first->name );
	}

	return std::make_pair( std::move(result), std::move(errors) );
}

std::vector<CodeBuilderError> ReferencesGraph::CheckWhileBlokVariablesState( const ReferencesGraph& state_before, const ReferencesGraph& state_after, const FilePos& file_pos )
{
	std::vector<CodeBuilderError> errors;

	U_ASSERT( state_before.nodes_.size() <= state_after.nodes_.size() ); // Pollution can add nodes.

	for( const auto& var_before : state_before.nodes_ )
	{
		U_ASSERT( state_after.nodes_.find( var_before.first ) != state_after.nodes_.end() );
		const auto& var_after= *state_after.nodes_.find( var_before.first );

		if( !var_before.second.moved && var_after.second.moved )
			REPORT_ERROR( OuterVariableMoveInsideLoop, errors, file_pos, var_before.first->name );

		// Add mutalbe reference in while loop.
		if( !state_before.HaveOutgoingLinks( var_before.first ) && state_after.HaveOutgoingMutableNodes( var_after.first ) )
			REPORT_ERROR( MutableReferencePollutionOfOuterLoopVariable, errors, file_pos, var_before.first->name, var_after.first->name );
	}

	return errors;
}

} // namespace CodeBuilderPrivate

} // namespace U
