#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Hashing.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "error_reporting.hpp"
#include "references_graph.hpp"

namespace U
{

bool ReferencesGraph::Link::operator==( const ReferencesGraph::Link& r ) const
{
	return this->src == r.src && this->dst == r.dst;
}

size_t ReferencesGraph::LinkHasher::operator()( const Link& link ) const
{
	return llvm::hash_combine( reinterpret_cast<uintptr_t>(link.src.get()), reinterpret_cast<uintptr_t>(link.dst.get()) );
}

void ReferencesGraph::AddNode( const VariablePtr& node )
{
	U_ASSERT( node != nullptr );
	U_ASSERT( nodes_.count(node) == 0 );
	nodes_.emplace( node, NodeState() );
}

void ReferencesGraph::AddNodeIfNotExists( const VariablePtr& node )
{
	if( nodes_.count( node ) == 0 )
		nodes_.emplace( node, NodeState() );
}

void ReferencesGraph::RemoveNode( const VariablePtr& node )
{
	if( nodes_.count(node) == 0 )
		return;

	if( const auto inner_reference= GetNodeInnerReference( node ) )
		RemoveNode( inner_reference );

	for( const VariablePtr& child : node->children )
		if( child != nullptr )
			RemoveNode( child );

	RemoveNodeLinks( node );

	nodes_.erase( node );
}

void ReferencesGraph::AddLink( const VariablePtr& from, const VariablePtr& to )
{
	U_ASSERT( from != nullptr );
	U_ASSERT( to != nullptr );
	U_ASSERT( nodes_.count(from) != 0 );
	U_ASSERT( nodes_.count(to  ) != 0 );

	if( from == to )
		return;

	links_.insert( Link{from, to} );
}

void ReferencesGraph::RemoveLink( const VariablePtr& from, const VariablePtr& to )
{
	U_ASSERT( from != nullptr );
	U_ASSERT( to != nullptr );
	U_ASSERT( nodes_.count(from) != 0 );
	U_ASSERT( nodes_.count(to  ) != 0 );

	const bool erased= links_.erase( Link{ from, to } ) != 0;
	(void)erased;

	U_ASSERT(erased); // Removing unexistent link.
}

bool ReferencesGraph::TryAddLink( const VariablePtr& from, const VariablePtr& to )
{
	if( (to->value_type == ValueType::ReferenceMut && HaveOutgoingLinks( from ) ) ||
		HaveOutgoingMutableNodes( from ) )
		return false;

	AddLink( from, to );
	return true;
}

VariablePtr ReferencesGraph::GetNodeInnerReference( const VariablePtr& node ) const
{
	const auto it= nodes_.find( node );
	U_ASSERT( it != nodes_.end() );
	return it->second.inner_reference;
}

VariablePtr ReferencesGraph::CreateNodeInnerReference( const VariablePtr& node, const ValueType value_type )
{
	U_ASSERT( value_type != ValueType::Value );

	const auto inner_node=
		std::make_shared<Variable>(
			FundamentalType( U_FundamentalType::InvalidType ),
			value_type,
			Variable::Location::Pointer,
			node->name + " inner reference" );

	AddNode( inner_node );

	const auto it= nodes_.find( node );
	U_ASSERT( it != nodes_.end() );
	U_ASSERT( it->second.inner_reference == nullptr );
	it->second.inner_reference= inner_node;

	return inner_node;
}

bool ReferencesGraph::HaveOutgoingLinks( const VariablePtr& from ) const
{
	for( const auto& link : links_ )
	{
		if( link.src == from )
			return true;
	}

	if( const VariablePtr parent= from->parent.lock() )
		if( HaveOutgoingLinks( parent ) )
			return true;

	for( const VariablePtr& child : from->children )
		if( child != nullptr && HaveOutgoingLinks( child ) )
			return true;

	return false;
}

bool ReferencesGraph::HaveOutgoingMutableNodes( const VariablePtr& from ) const
{
	for( const auto& link : links_ )
	{
		if( link.src == from && link.dst->value_type == ValueType::ReferenceMut )
			return true;
	}

	if( const VariablePtr parent= from->parent.lock() )
		if( HaveOutgoingMutableNodes( parent ) )
			return true;

	for( const VariablePtr& child : from->children )
		if( child != nullptr && HaveOutgoingMutableNodes( child ) )
			return true;

	return false;
}

void ReferencesGraph::MoveNode( const VariablePtr& node )
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

	// TODO - is this correct to remove children nodes?
	for( const VariablePtr& child : node->children )
		if( child != nullptr )
			RemoveNode( child );

	RemoveNodeLinks( node );
}

bool ReferencesGraph::NodeMoved( const VariablePtr& node ) const
{
	const auto it= nodes_.find(node);
	if( it == nodes_.end() ) // Can be for global constants, for example.
		return false;

	return it->second.moved;
}

ReferencesGraph::NodesSet ReferencesGraph::GetAllAccessibleVariableNodes( const VariablePtr& node ) const
{
	NodesSet visited_nodes_set, result_set;
	GetAllAccessibleVariableNodes_r( node, visited_nodes_set, result_set );
	return result_set;
}

ReferencesGraph::NodesSet ReferencesGraph::GetAccessibleVariableNodesInnerReferences( const VariablePtr& node ) const
{
	NodesSet visited_nodes_set, result_set;
	GetAccessibleVariableNodesInnerReferences_r( node, visited_nodes_set, result_set );
	return result_set;
}

ReferencesGraph::NodesSet ReferencesGraph::GetNodeInputLinks( const VariablePtr& node ) const
{
	NodesSet result;

	VariablePtr current_node= node;
	while( current_node != nullptr )
	{
		for( const Link& link : links_ )
			if( link.dst == current_node )
				result.insert( link.src );

		current_node= current_node->parent.lock();
	}

	return result;
}

void ReferencesGraph::GetAllAccessibleVariableNodes_r(
	const VariablePtr& node,
	NodesSet& visited_nodes_set,
	NodesSet& result_set ) const
{
	U_ASSERT( nodes_.find(node) != nodes_.end() );

	if( !visited_nodes_set.insert(node).second )
		return; // Already visited

	if( node->value_type == ValueType::Value )
		result_set.emplace( node );

	for( const auto& link : links_ )
		if( link.dst == node )
			GetAllAccessibleVariableNodes_r( link.src, visited_nodes_set, result_set );

	if( const VariablePtr parent= node->parent.lock() )
		GetAllAccessibleVariableNodes_r( parent, visited_nodes_set, result_set );

	// Children nodes can't have input links. So, ignore them.
}

void ReferencesGraph::GetAccessibleVariableNodesInnerReferences_r(
	const VariablePtr& node,
	NodesSet& visited_nodes_set,
	NodesSet& result_set ) const
{
	U_ASSERT( nodes_.find(node) != nodes_.end() );

	if( !visited_nodes_set.insert(node).second )
		return; // Already visited

	if( node->value_type == ValueType::Value )
	{
		if( auto inner_node= GetNodeInnerReference( node ) )
			result_set.emplace( inner_node );
		return;
	}

	for( const auto& link : links_ )
		if( link.dst == node )
			GetAccessibleVariableNodesInnerReferences_r( link.src, visited_nodes_set, result_set );

	if( const VariablePtr parent= node->parent.lock() )
		GetAccessibleVariableNodesInnerReferences_r( parent, visited_nodes_set, result_set );
}

ReferencesGraph::MergeResult ReferencesGraph::MergeVariablesStateAfterIf( const std::vector<ReferencesGraph>& branches_variables_state, const SrcLoc& src_loc )
{
	ReferencesGraph result;
	std::vector<CodeBuilderError> errors;

	std::vector< std::pair<VariablePtr, VariablePtr> > replaced_nodes; // First node replaced with second node.
	for( const ReferencesGraph& branch_state : branches_variables_state )
	{
		for( const auto& node_pair : branch_state.nodes_ )
		{
			if( result.nodes_.find( node_pair.first ) == result.nodes_.end() )
				result.nodes_[ node_pair.first ]= node_pair.second;

			const NodeState& src_state= node_pair.second;
			NodeState& result_state= result.nodes_[ node_pair.first ];

			if( result_state.moved != src_state.moved )
				REPORT_ERROR( ConditionalMove, errors, src_loc, node_pair.first->name );

				 if( result_state.inner_reference == nullptr && src_state.inner_reference == nullptr ) {}
			else if( result_state.inner_reference == nullptr && src_state.inner_reference != nullptr )
			{
				result.nodes_[ result_state.inner_reference ]= NodeState();
				result_state.inner_reference= src_state.inner_reference;
			}
			else if( result_state.inner_reference != nullptr && src_state.inner_reference == nullptr ) {}
			else if( result_state.inner_reference != src_state.inner_reference ) // both nonnull and different
			{
				// Variable inner reference created in multiple braches.

				// If linked as mutable and as immutable in different branches - result is mutable.
				if( ( result_state.inner_reference->value_type != ValueType::ReferenceMut && src_state.inner_reference->value_type == ValueType::ReferenceMut ) )
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
			if( link.src != link.dst )
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
				if( link.dst->value_type == ValueType::ReferenceMut )
					++mutable_links_count;
				else
					++immutable_links_count;
			}
		}
		if( mutable_links_count > 1u || ( immutable_links_count > 0u && mutable_links_count > 0u ) )
			REPORT_ERROR( ReferenceProtectionError, errors, src_loc, node.first->name );
	}

	return std::make_pair( std::move(result), std::move(errors) );
}

std::vector<CodeBuilderError> ReferencesGraph::CheckWhileBlockVariablesState( const ReferencesGraph& state_before, const ReferencesGraph& state_after, const SrcLoc& src_loc )
{
	std::vector<CodeBuilderError> errors;

	U_ASSERT( state_before.nodes_.size() <= state_after.nodes_.size() ); // Pollution can add nodes.

	for( const auto& var_before : state_before.nodes_ )
	{
		U_ASSERT( state_after.nodes_.find( var_before.first ) != state_after.nodes_.end() );
		const auto& var_after= *state_after.nodes_.find( var_before.first );

		if( !var_before.second.moved && var_after.second.moved )
			REPORT_ERROR( OuterVariableMoveInsideLoop, errors, src_loc, var_before.first->name );

		if( var_before.second.inner_reference != var_after.second.inner_reference || // Reference pollution added first time
			// Or accessible variables changed.
			( var_before.second.inner_reference != nullptr &&
			  state_before.GetAllAccessibleVariableNodes( var_before.second.inner_reference ) != state_after.GetAllAccessibleVariableNodes( var_after.second.inner_reference ) ) )
		{
			NodesSet added_nodes= state_after.GetAllAccessibleVariableNodes( var_after.second.inner_reference );
			if( var_before.second.inner_reference != nullptr )
			{
				for( const auto& node : state_before.GetAllAccessibleVariableNodes( var_before.second.inner_reference ) )
					added_nodes.erase(node);
			}
			for( const auto& node : added_nodes )
				REPORT_ERROR( ReferencePollutionOfOuterLoopVariable, errors, src_loc, var_before.first->name, node->name );
		}
	}
	return errors;
}

void ReferencesGraph::RemoveNodeLinks( const VariablePtr& node )
{
	// Collect in/out nodes.
	NodesSet in_nodes;
	NodesSet out_nodes;
	for( const auto& link : links_ )
	{
		if( link.src == link.dst ) // Self loop link.
			continue;

		if( link.src == node )
			out_nodes.insert( link.dst );
		if( link.dst == node )
			in_nodes.insert( link.src );
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

	// Create new links.
	for( const VariablePtr& from : in_nodes )
		for( const VariablePtr& to : out_nodes )
			AddLink( from, to );

	// If this is a child node, replace links from it with links from parent.
	if( const VariablePtr parent= node->parent.lock() )
	{
		U_ASSERT( in_nodes.empty() ); // Child node has no input links, it has only parent.

		for( const VariablePtr& to : out_nodes )
			AddLink( parent, to );
	}
}

} // namespace U
