#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Hashing.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "error_reporting.hpp"
#include "references_graph.hpp"

namespace U
{


void ReferencesGraph::Delta::ProcessAddNode( const VariablePtr& node )
{
	operations.push_back( AddNodeOp{ node } );
}

void ReferencesGraph::Delta::ProcessMoveNode( const VariablePtr& node )
{
	operations.push_back( MoveNodeOp{ node } );
}

void ReferencesGraph::Delta::ProcessRemoveNode( const VariablePtr& node )
{
	// Remove all previous operations for this node.
	// This is fine, until "move", "add link", "remove link" operations are performed only between "add node" and "remove node".
	// If "add" operation was removed - do not add "remove" operation.

	bool add_node_removed= false;
	const auto new_end= std::remove_if(
		operations.begin(), operations.end(),
		[&]( const Operation& op )
		{
			if( const auto add_op= std::get_if<AddNodeOp>( &op ) )
			{
				if( add_op->node == node )
				{
					add_node_removed= true;
					return true;
				}
			}
			if( add_node_removed )
			{
				if( const auto move_op= std::get_if<MoveNodeOp>( &op ) )
				{
					if( move_op->node == node )
						return true;
				}
				if( const auto add_link_op= std::get_if<AddLinkOp>( &op ) )
				{
					if( add_link_op->from == node || add_link_op->to == node )
						return true;
				}
				if( const auto remove_link_op= std::get_if<RemoveLinkOp>( &op ) )
				{
					if( remove_link_op->from == node || remove_link_op->to == node )
						return true;
				}
			}

			return false;
		} );

	operations.erase( new_end, operations.end() );

	if( !add_node_removed )
		operations.push_back( RemoveNodeOp{ node } );
}

void ReferencesGraph::Delta::ProcessAddLink( const VariablePtr& from, const VariablePtr& to )
{
	operations.push_back( AddLinkOp{ from, to } );
}

void ReferencesGraph::Delta::ProcessRemoveLink( const VariablePtr& from, const VariablePtr& to )
{
	bool add_link_removed= false;
	const auto new_end= std::remove_if(
		operations.begin(), operations.end(),
		[&]( const Operation& op )
		{
			if( const auto add_link_op= std::get_if<AddLinkOp>( &op ) )
			{
				if( add_link_op->from == from && add_link_op->to == to )
				{
					add_link_removed= true;
					return true;
				}
			}

			return false;
		} );

	operations.erase( new_end, operations.end() );

	if( !add_link_removed )
		operations.push_back( RemoveLinkOp{ from, to } );
}

ReferencesGraph::Delta ReferencesGraph::TakeDeltaState()
{
	Delta result;
	std::swap( result, delta_ );
	return result;
}

ReferencesGraph::Delta ReferencesGraph::CopyDeltaState() const
{
	return delta_;
}

void ReferencesGraph::RollbackChanges( Delta prev_delta_state )
{
	// Take current delta, because rollback methods may modify it.
	const Delta cur_delta= std::move(delta_);

	for( auto it= cur_delta.operations.rbegin(); it != cur_delta.operations.rend(); ++it )
	{
		if( const auto add_node_op= std::get_if<Delta::AddNodeOp>( &*it ) )
			RemoveNode(add_node_op->node);
		else if( const auto remove_node_op= std::get_if<Delta::RemoveNodeOp>( &*it ) )
			AddNode(remove_node_op->node);
		else if( const auto move_node_op= std::get_if<Delta::MoveNodeOp>( &*it ) )
		{
			if( const auto node_state_it= nodes_.find(move_node_op->node); node_state_it != nodes_.end() )
				node_state_it->second.moved= false;
		}
		else if( const auto add_link_op= std::get_if<Delta::AddLinkOp>( &*it ) )
			RemoveLink( add_link_op->from, add_link_op->to );
		else if( const auto remove_link_op= std::get_if<Delta::RemoveLinkOp>( &*it ) )
			AddLink( remove_link_op->from, remove_link_op->to );
		else U_ASSERT(false);
	}

	delta_= std::move(prev_delta_state);
}

void ReferencesGraph::ApplyBranchingStates( const llvm::ArrayRef<Delta> branches_states, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	// TODO optimize this - use faster container.
	std::unordered_map<VariablePtr, uint32_t> moved_variables;

	for( const Delta& branch_delta : branches_states )
	{
		// Iterate over all operations in this branch and apply them.
		// Normally no nodes should be added or removed.
		// Nodes may be moved.
		// Links may be added and removed (in case of move).
		// It is fine to add same links from multiple branches - links deduplication should work properly.

		for( const Delta::Operation& op : branch_delta.operations )
		{
			if( const auto add_node_op= std::get_if<Delta::AddNodeOp>( &op ) )
			{
				// Lazy-added nodes (global variables, children nodes) may be added more than in one branch.
				if( nodes_.count( add_node_op->node ) == 0 )
					AddNode(add_node_op->node);
			}
			else if( const auto remove_node_op= std::get_if<Delta::RemoveNodeOp>( &op ) )
				RemoveNode(remove_node_op->node);
			else if( const auto move_node_op= std::get_if<Delta::MoveNodeOp>( &op ) )
			{
				const auto node_state_it= nodes_.find(move_node_op->node);
				if( node_state_it != nodes_.end() )
				{
					if( !node_state_it->second.moved )
						MoveNode( move_node_op->node );

					if( moved_variables.count( move_node_op->node ) == 0 )
						moved_variables[move_node_op->node]= 1;
					else
						++moved_variables[move_node_op->node];
				}
			}
			else if( const auto add_link_op= std::get_if<Delta::AddLinkOp>( &op ) )
				AddLink( add_link_op->from, add_link_op->to );
			else if( const auto remove_link_op= std::get_if<Delta::RemoveLinkOp>( &op ) )
				RemoveLink( remove_link_op->from, remove_link_op->to );
			else U_ASSERT(false);
		}
	}

	for( const auto& node_pair : moved_variables )
		if( node_pair.second != branches_states.size() )
			REPORT_ERROR( ConditionalMove, errors_container, src_loc, node_pair.first->name );
}

void ReferencesGraph::CheckLoopBodyState( const Delta& delta, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	for( const Delta::Operation& op : delta.operations )
	{
		if( const auto move_op = std::get_if<Delta::MoveNodeOp>( &op ) )
		{
			REPORT_ERROR( OuterVariableMoveInsideLoop, errors_container, src_loc, move_op->node->name );
		}
		else if( const auto add_link_op= std::get_if<Delta::AddLinkOp>( &op ) )
		{
			REPORT_ERROR( ReferencePollutionOfOuterLoopVariable, errors_container, src_loc, add_link_op->to->name, add_link_op->from->name );
		}
	}
}

ReferencesGraph::Delta ReferencesGraph::CombineDeltas( const llvm::ArrayRef<Delta> deltas, const Delta& current_state )
{
	Delta result;

	if( deltas.empty() )
		return current_state;

	result= deltas.front();
	for( size_t i= 1; i < deltas.size(); ++i )
		CombineDeltasImpl( result, deltas[i] );

	CombineDeltasImpl( result, current_state );

	return result;
}

void ReferencesGraph::CombineDeltasImpl( Delta& dst, const Delta& src )
{
	for( const Delta::Operation& op : src.operations )
	{
		if( const auto add_node_op= std::get_if<Delta::AddNodeOp>( &op ) )
			dst.ProcessAddNode( add_node_op->node );
		else if( const auto remove_node_op= std::get_if<Delta::RemoveNodeOp>( &op ) )
			dst.ProcessRemoveNode( remove_node_op->node );
		else if( const auto move_node_op= std::get_if<Delta::MoveNodeOp>( &op ) )
			dst.ProcessMoveNode( move_node_op->node );
		else if( const auto add_link_op= std::get_if<Delta::AddLinkOp>( &op ) )
			dst.ProcessAddLink( add_link_op->from, add_link_op->to );
		else if( const auto remove_link_op= std::get_if<Delta::RemoveLinkOp>( &op ) )
			dst.ProcessRemoveLink( remove_link_op->from, remove_link_op->to );
		else U_ASSERT(false);
	}
}

void ReferencesGraph::AddNode( const VariablePtr& node )
{
	U_ASSERT( node != nullptr );
	U_ASSERT( nodes_.count(node) == 0 );
	nodes_.emplace( node, NodeState() );

	delta_.ProcessAddNode( node );

	if( node->parent.lock() == nullptr )
		for( const VariablePtr& inner_reference_node : node->inner_reference_nodes )
		{
			U_ASSERT( inner_reference_node != nullptr );
			U_ASSERT( nodes_.count(inner_reference_node) == 0 );
			nodes_.emplace( inner_reference_node, NodeState() );
		}
}

void ReferencesGraph::AddNodeIfNotExists( const VariablePtr& node )
{
	if( nodes_.count( node ) == 0 )
	{
		nodes_.emplace( node, NodeState() );
		delta_.ProcessAddNode( node );
	}

	for( const VariablePtr& inner_reference_node : node->inner_reference_nodes )
	{
		if( nodes_.count( inner_reference_node ) == 0 )
			nodes_.emplace( inner_reference_node, NodeState() );
	}
}

void ReferencesGraph::RemoveNode( const VariablePtr& node )
{
	if( nodes_.count(node) == 0 )
		return;

	if( node->parent.lock() == nullptr )
		for( const VariablePtr& inner_reference_node : node->inner_reference_nodes )
		{
			if( nodes_.count(inner_reference_node) != 0 )
			{
				RemoveNodeLinks( inner_reference_node );
				nodes_.erase(inner_reference_node);
			}
		}

	for( const VariablePtr& child : node->children )
		if( child != nullptr )
			RemoveNode( child );

	RemoveNodeLinks( node );

	nodes_.erase( node );
	delta_.ProcessRemoveNode( node );
}

void ReferencesGraph::AddLink( const VariablePtr& from, const VariablePtr& to )
{
	U_ASSERT( from != nullptr );
	U_ASSERT( to != nullptr );
	U_ASSERT( nodes_.count(from) != 0 );
	U_ASSERT( nodes_.count(to  ) != 0 );

	if( from == to )
		return;

	NodeState& from_state= nodes_[from];
	NodeState& to_state= nodes_[to];

	for( const VariablePtr& out_link : from_state.out_links )
		if( out_link == to )
			return;

	for( const VariablePtr& in_link : to_state.out_links )
		if( in_link == to )
			return;

	delta_.ProcessAddLink( from, to );

	from_state.out_links.push_back( to );
	to_state.in_links.push_back( from );
}

void ReferencesGraph::RemoveLink( const VariablePtr& from, const VariablePtr& to )
{
	U_ASSERT( from != nullptr );
	U_ASSERT( to != nullptr );
	U_ASSERT( nodes_.count(from) != 0 );
	U_ASSERT( nodes_.count(to  ) != 0 );

	delta_.ProcessRemoveLink( from, to );

	NodeState& from_state= nodes_[from];
	for( size_t i= 0; i < from_state.out_links.size(); ++i )
		if( from_state.out_links[i] == to )
		{
			if( i + 1 < from_state.out_links.size() )
				from_state.out_links[i]= std::move(from_state.out_links.back());
			from_state.out_links.pop_back();
			break;
		}

	NodeState& to_state= nodes_[to];
	for( size_t i= 0; i < to_state.in_links.size(); ++i )
		if( to_state.in_links[i] == from )
		{
			if( i + 1 < to_state.in_links.size() )
				to_state.in_links[i]= std::move(to_state.in_links.back());
			to_state.in_links.pop_back();
			break;
		}
}

void ReferencesGraph::TryAddLink( const VariablePtr& from, const VariablePtr& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	U_ASSERT( from != nullptr );
	U_ASSERT( to != nullptr );
	if( (to->value_type == ValueType::ReferenceMut && HaveOutgoingLinks( from ) ) ||
		HaveOutgoingMutableNodes( from ) )
	{
		REPORT_ERROR( ReferenceProtectionError, errors_container, src_loc, from->name );
	}

	AddLink( from, to );
}

void ReferencesGraph::TryAddInnerLinks( const VariablePtr& from, const VariablePtr& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	const size_t reference_tag_count= to->type.ReferencesTagsCount();
	U_ASSERT( from->inner_reference_nodes.size() >= reference_tag_count );
	U_ASSERT( to->inner_reference_nodes.size() >= reference_tag_count );
	for( size_t i= 0; i < reference_tag_count; ++i )
		TryAddLink( from->inner_reference_nodes[i], to->inner_reference_nodes[i], errors_container, src_loc );
}

void ReferencesGraph::TryAddInnerLinksForTupleElement( const VariablePtr& from, const VariablePtr& to, const size_t element_index, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	const TupleType* const tuple_type= from->type.GetTupleType();
	U_ASSERT( tuple_type != nullptr );
	U_ASSERT( element_index < tuple_type->element_types.size() );
	U_ASSERT( tuple_type->element_types[element_index] == to->type );
	const size_t element_type_reference_tag_count= to->type.ReferencesTagsCount();
	if( element_type_reference_tag_count == 0 )
		return;

	size_t offset= 0;
	for( size_t i= 0; i < element_index; ++i )
		offset+= tuple_type->element_types[i].ReferencesTagsCount();

	U_ASSERT( offset <= from->inner_reference_nodes.size() );
	U_ASSERT( offset + element_type_reference_tag_count <= from->inner_reference_nodes.size() );
	U_ASSERT( to->inner_reference_nodes.size() == element_type_reference_tag_count );
	for( size_t i= 0; i < element_type_reference_tag_count; ++i )
		TryAddLink( from->inner_reference_nodes[i + offset], to->inner_reference_nodes[i], errors_container, src_loc );
}

void ReferencesGraph::TryAddInnerLinksForClassField( const VariablePtr& from, const VariablePtr& to, const ClassField& field, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	U_ASSERT( from->type.GetClassType() != nullptr );

	const auto field_reference_tag_count= to->type.ReferencesTagsCount();
	U_ASSERT( to->inner_reference_nodes.size() == to->type.ReferencesTagsCount() );
	U_ASSERT( to->type == field.type );
	U_ASSERT( !field.is_reference );
	U_ASSERT( field.inner_reference_tags.size() == field_reference_tag_count );

	for( size_t i= 0u; i < field_reference_tag_count; ++i )
	{
		const auto src_tag_number= field.inner_reference_tags[i];
		U_ASSERT( src_tag_number < from->inner_reference_nodes.size() );
		TryAddLink( from->inner_reference_nodes[src_tag_number], to->inner_reference_nodes[i], errors_container, src_loc );
	}
}

bool ReferencesGraph::HaveOutgoingLinks( const VariablePtr& from ) const
{
	// Check if any parent have links and any child (including children of children) have links.
	// Doesn't count sibling nodes and other indirect relatives.

	if( HaveOutgoingLinksIncludingChildrenLinks_r( from ) )
		return true;

	VariablePtr parent= from->parent.lock();
	while( parent != nullptr )
	{
		if( HaveDirectOutgoingLinks( parent ) )
			return true;
		parent= parent->parent.lock();
	}

	return false;
}

bool ReferencesGraph::HaveOutgoingMutableNodes( const VariablePtr& from ) const
{
	// Check if any parent have mutable links and any child (including children of children) have mutable links.
	// Doesn't count sibling nodes and other indirect relatives.

	if( HaveOutgoingMutableNodesIncludingChildrenNodes_r( from ) )
		return true;

	VariablePtr parent= from->parent.lock();
	while( parent != nullptr )
	{
		if( HaveDirectOutgoingMutableNodes( parent ) )
			return true;
		parent= parent->parent.lock();
	}

	return false;
}

void ReferencesGraph::MoveNode( const VariablePtr& node )
{
	U_ASSERT( nodes_.count(node) != 0 );

	delta_.ProcessMoveNode( node );

	NodeState& node_state= nodes_[node];

	U_ASSERT( !node_state.moved );
	node_state.moved= true;

	for( const VariablePtr& inner_reference_node : node->inner_reference_nodes )
		RemoveNodeLinks( inner_reference_node );

	// Move child nodes first in order to replace links from children with links from parent.
	for( const VariablePtr& child : node->children )
		if( child != nullptr && nodes_.count(child) != 0 ) // Children nodes are lazily-added.
			MoveNode( child );

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

ReferencesGraph::NodesSet ReferencesGraph::GetNodeInputLinks( const VariablePtr& node ) const
{
	NodesSet result;

	VariablePtr current_node= node;
	while( current_node != nullptr )
	{
		const auto it= nodes_.find( current_node );
		U_ASSERT( it != nodes_.end() );
		for( const VariablePtr& in_link : it->second.in_links )
			result.insert( in_link );

		current_node= current_node->parent.lock();
	}

	return result;
}

void ReferencesGraph::TryAddLinkToAllAccessibleVariableNodesInnerReferences( const VariablePtr& from, const VariablePtr& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	TryAddLinkToAllAccessibleVariableNodesInnerReferences_r( from, to, errors_container, src_loc );
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

	const auto it= nodes_.find( node );
	U_ASSERT( it != nodes_.end() );
	for( const VariablePtr& in_link : it->second.in_links )
		GetAllAccessibleVariableNodes_r( in_link, visited_nodes_set, result_set );

	if( const VariablePtr parent= node->parent.lock() )
		GetAllAccessibleVariableNodes_r( parent, visited_nodes_set, result_set );

	// Children nodes can't have input links. So, ignore them.
}

void ReferencesGraph::TryAddLinkToAllAccessibleVariableNodesInnerReferences_r( const VariablePtr& from, const VariablePtr& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	if( to->is_variable_inner_reference_node )
		TryAddLink( from, to, errors_container, src_loc );
	else
	{
		// Fill container with reachable nodes and only that perform recursive calls.
		// Do this in order to avoid iteration over container of links, which may be modified in recursive call.
		llvm::SmallVector< VariablePtr, 12 > src_nodes;
		const auto it= nodes_.find( to );
		U_ASSERT( it != nodes_.end() );
		for( const VariablePtr& in_link : it->second.in_links )
			src_nodes.push_back( in_link );

		for( const VariablePtr& src_node : src_nodes )
			TryAddLinkToAllAccessibleVariableNodesInnerReferences_r( from, src_node, errors_container, src_loc );
	}
}

bool ReferencesGraph::HaveDirectOutgoingLinks( const VariablePtr& from ) const
{
	const auto it= nodes_.find(from);
	U_ASSERT( it != nodes_.end() );
	return !it->second.out_links.empty();
}

bool ReferencesGraph::HaveOutgoingLinksIncludingChildrenLinks_r( const VariablePtr& from ) const
{
	if( HaveDirectOutgoingLinks( from ) )
			return true;

	for( const VariablePtr& child : from->children )
		if( child != nullptr &&
			nodes_.count(child) != 0 && // Children nodes are lazily-added.
			HaveOutgoingLinksIncludingChildrenLinks_r( child ) )
			return true;

	return false;
}

bool ReferencesGraph::HaveDirectOutgoingMutableNodes( const VariablePtr& from ) const
{
	const auto it= nodes_.find(from);
	U_ASSERT( it != nodes_.end() );
	for( const VariablePtr& link : it->second.out_links )
	{
		if( link->value_type == ValueType::ReferenceMut )
			return true;
	}

	return false;
}

bool ReferencesGraph::HaveOutgoingMutableNodesIncludingChildrenNodes_r( const VariablePtr& from ) const
{
	if( HaveDirectOutgoingMutableNodes( from ) )
		return true;

	for( const VariablePtr& child : from->children )
		if( child != nullptr &&
			nodes_.count(child) != 0 && // Children nodes are lazily-added.
			HaveOutgoingMutableNodesIncludingChildrenNodes_r( child ) )
			return true;

	return false;
}

void ReferencesGraph::RemoveNodeLinks( const VariablePtr& node )
{
	const auto it= nodes_.find(node);
	U_ASSERT( it != nodes_.end() );

	// Collect in/out nodes.
	llvm::SmallVector<VariablePtr, 8> in_links;
	llvm::SmallVector<VariablePtr, 8> out_links;
	for( const VariablePtr& in_link : it->second.in_links )
	{
		in_links.push_back( in_link );
		delta_.ProcessRemoveLink( in_link, node );
	}
	for( const VariablePtr& out_link : it->second.out_links )
	{
		out_links.push_back( out_link );
		delta_.ProcessRemoveLink( node, out_link );
	}

	// Remove links.
	it->second.in_links.clear();
	for( const VariablePtr& in_link : in_links )
	{
		const auto in_it= nodes_.find(in_link);
		U_ASSERT( in_it != nodes_.end() );
		for( size_t i= 0; i < in_it->second.out_links.size(); ++i )
		{
			if( in_it->second.out_links[i] == node )
			{
				if( i + 1 < in_it->second.out_links.size() )
					in_it->second.out_links[i]= std::move(in_it->second.out_links.back());
				in_it->second.out_links.pop_back();
				break;
			}
		}
	}

	it->second.out_links.clear();
	for( const VariablePtr& out_link : out_links )
	{
		const auto out_it= nodes_.find(out_link);
		U_ASSERT( out_it != nodes_.end() );
		for( size_t i= 0; i < out_it->second.in_links.size(); ++i )
		{
			if( out_it->second.in_links[i] == node )
			{
				if( i + 1 < out_it->second.in_links.size() )
					out_it->second.in_links[i]= std::move(out_it->second.in_links.back());
				out_it->second.in_links.pop_back();
				break;
			}
		}
	}

	// Create new links.
	for( const VariablePtr& from : in_links )
		for( const VariablePtr& to : out_links )
			AddLink( from, to );

	// If this is a child node, replace links from it with links from parent.
	if( const VariablePtr parent= node->parent.lock() )
	{
		U_ASSERT( in_links.empty() ); // Child node has no input links, it has only parent.

		for( const VariablePtr& to : out_links )
			AddLink( parent, to );
	}
}

} // namespace U
