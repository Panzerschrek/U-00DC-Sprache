#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Hashing.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "error_reporting.hpp"
#include "references_graph.hpp"

namespace U
{


void ReferencesGraph::Delta::AddNodeOperation( const NodeOperationKind kind, const VariablePtr& node )
{
	if( kind == NodeOperationKind::Add || kind == NodeOperationKind::Remove )
		node_operations.push_back( NodeOperation{ kind, node } );
	else if( kind == NodeOperationKind::Remove )
	{
		// If this is remove node iteration, search for corresponding add node and move node.
		// Remove all "move" operations and "add" operations.
		// If add operation was removed - do not add remove operation.
		bool add_node_removed= false;
		const auto new_end= std::remove_if(
			node_operations.begin(), node_operations.end(),
			[&]( const NodeOperation& op )
			{
				if( op.node == node )
				{
					if( op.kind == NodeOperationKind::Add )
					{
						add_node_removed= true;
						return true;
					}
					if( op.kind == NodeOperationKind::Move )
						return true;
				}
				return false;
			} );

		node_operations.erase( new_end, node_operations.end() );

		if( !add_node_removed )
			node_operations.push_back( NodeOperation{ kind, node } );
	}
	else U_ASSERT(false);
}

void ReferencesGraph::Delta::AddLinkOperation( const LinkOperationKind kind, const VariablePtr& from, const VariablePtr& to )
{
	if( kind == LinkOperationKind::Add )
		link_operations.push_back( LinkOperation{ kind, from, to } );
	else if( kind == LinkOperationKind::Remove )
	{
		// if this is link removal operation - try to find and remove link addition operation.
		// If found - do not bother to add link removal operation.

		bool add_link_removed= false;
		const auto new_end= std::remove_if(
			link_operations.begin(), link_operations.end(),
			[&]( const LinkOperation& op )
			{
				if( op.kind == LinkOperationKind::Add && op.from == from && op.to == to )
				{
					add_link_removed= true;
					return true;
				}
				return false;
			} );

		link_operations.erase( new_end, link_operations.end() );

		if( !add_link_removed )
			link_operations.push_back( LinkOperation{ kind, from, to } );
	}
	else U_ASSERT(false);
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

	// TODO - maybe interleave link and node operations?
	for( auto it= cur_delta.node_operations.rbegin(); it != cur_delta.node_operations.rend(); ++it )
	{
		switch(it->kind)
		{
		case Delta::NodeOperationKind::Add:
			RemoveNode(it->node);
			break;
		case Delta::NodeOperationKind::Remove:
			AddNode(it->node);
			break;
		case Delta::NodeOperationKind::Move:
			if( const auto node_state_it= nodes_.find(it->node); node_state_it != nodes_.end() )
				node_state_it->second.moved= false;
			break;
		}
	}

	for( auto it= cur_delta.link_operations.rbegin(); it != cur_delta.link_operations.rend(); ++it )
	{
		switch(it->kind)
		{
		case Delta::LinkOperationKind::Add:
			RemoveLink( it->from, it->to );
			break;
		case Delta::LinkOperationKind::Remove:
			AddLink( it->from, it->to );
			break;
		}
	}

	delta_= std::move(prev_delta_state);
}

void ReferencesGraph::ApplyBranchingStates( const llvm::ArrayRef<Delta> branches_states )
{
	// TODO
}

void ReferencesGraph::AddNode( const VariablePtr& node )
{
	U_ASSERT( node != nullptr );
	U_ASSERT( nodes_.count(node) == 0 );
	nodes_.emplace( node, NodeState() );

	delta_.AddNodeOperation( Delta::NodeOperationKind::Add, node );

	if( node->parent.lock() == nullptr )
		for( const VariablePtr& inner_reference_node : node->inner_reference_nodes )
			AddNode( inner_reference_node );
}

void ReferencesGraph::AddNodeIfNotExists( const VariablePtr& node )
{
	if( nodes_.count( node ) == 0 )
	{
		nodes_.emplace( node, NodeState() );
		delta_.AddNodeOperation( Delta::NodeOperationKind::Add, node );
	}

	for( const VariablePtr& inner_reference_node : node->inner_reference_nodes )
		AddNodeIfNotExists( inner_reference_node );
}

void ReferencesGraph::RemoveNode( const VariablePtr& node )
{
	if( nodes_.count(node) == 0 )
		return;

	delta_.AddNodeOperation( Delta::NodeOperationKind::Remove, node );

	if( node->parent.lock() == nullptr )
		for( const VariablePtr& inner_reference_node : node->inner_reference_nodes )
			RemoveNode( inner_reference_node );

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

	NodeState& from_state= nodes_[from];
	NodeState& to_state= nodes_[to];

	for( const VariablePtr& out_link : from_state.out_links )
		if( out_link == to )
			return;

	for( const VariablePtr& in_link : to_state.out_links )
		if( in_link == to )
			return;

	delta_.AddLinkOperation( Delta::LinkOperationKind::Add, from, to );

	from_state.out_links.push_back( to );
	to_state.in_links.push_back( from );
}

void ReferencesGraph::RemoveLink( const VariablePtr& from, const VariablePtr& to )
{
	U_ASSERT( from != nullptr );
	U_ASSERT( to != nullptr );
	U_ASSERT( nodes_.count(from) != 0 );
	U_ASSERT( nodes_.count(to  ) != 0 );

	delta_.AddLinkOperation( Delta::LinkOperationKind::Remove, from, to );

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

	delta_.AddNodeOperation( Delta::NodeOperationKind::Move, node );

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

ReferencesGraph::MergeResult ReferencesGraph::MergeVariablesStateAfterIf( const llvm::ArrayRef<ReferencesGraph> branches_variables_state, const SrcLoc& src_loc )
{
	ReferencesGraph result;
	std::vector<CodeBuilderError> errors;

	// Number of nodes in different branches may be different - child nodes and global variable nodes may be added.

	for( const ReferencesGraph& branch_state : branches_variables_state )
	{
		for( const auto& node_pair : branch_state.nodes_ )
		{
			if( result.nodes_.find( node_pair.first ) == result.nodes_.end() )
				result.nodes_[ node_pair.first ]= node_pair.second;
			else
			{
				const NodeState& src_state= node_pair.second;
				NodeState& result_state= result.nodes_[ node_pair.first ];

				if( result_state.moved != src_state.moved )
					REPORT_ERROR( ConditionalMove, errors, src_loc, node_pair.first->name );

				for( const VariablePtr& in_link : src_state.in_links )
				{
					if( std::find( result_state.in_links.begin(), result_state.in_links.end(), in_link ) == result_state.in_links.end() )
						result_state.in_links.push_back( in_link );
				}
				for( const VariablePtr& out_link : src_state.out_links )
				{
					if( std::find( result_state.out_links.begin(), result_state.out_links.end(), out_link ) == result_state.out_links.end() )
						result_state.out_links.push_back( out_link );
				}
			}
		}
	}

	// Technically it's possible to create mutliple mutable references to same node or mutable reference + immutable reference.
	// But this is not an error, actually, because only one reference is created in runtime (depending on executed branch).
	// Seeming reference rules violation happens, because we perform variables state merging instead of maintaining two or more separate states.
	// Maintaining multiple variables states after each branching is computationally too costly.

	return std::make_pair( std::move(result), std::move(errors) );
}

std::vector<CodeBuilderError> ReferencesGraph::CheckVariablesStateAfterLoop( const ReferencesGraph& state_before, const ReferencesGraph& state_after, const SrcLoc& src_loc )
{
	std::vector<CodeBuilderError> errors;

	U_ASSERT( state_before.nodes_.size() <= state_after.nodes_.size() ); // Child nodes and global variable nodes may be added.

	for( const auto& var_before : state_before.nodes_ )
	{
		const VariablePtr& node= var_before.first;
		U_ASSERT( state_after.nodes_.find(node) != state_after.nodes_.end() );
		const auto& var_after= *state_after.nodes_.find( node );

		if( !var_before.second.moved && var_after.second.moved )
			REPORT_ERROR( OuterVariableMoveInsideLoop, errors, src_loc, var_before.first->name );

		if( node->value_type == ValueType::Value )
		{
			// If this is a variable node with inner references check if no input links was added in loop body.
			// Reference nodes also may have inner reference nodes, but adding of input links (pollution) for them is not possible, so, ignore them.
			for( const VariablePtr& inner_reference_node : node->inner_reference_nodes )
			{
				const NodesSet nodes_before= state_before.GetNodeInputLinks( inner_reference_node );
				NodesSet nodes_after= state_after.GetNodeInputLinks( inner_reference_node );
				for( const auto& node : nodes_before )
					nodes_after.erase(node);

				for( const auto& newly_linked_node : nodes_after )
					REPORT_ERROR( ReferencePollutionOfOuterLoopVariable, errors, src_loc, node->name, newly_linked_node->name );
			}
		}
	}
	return errors;
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
		delta_.AddLinkOperation( Delta::LinkOperationKind::Remove, in_link, node );
	}
	for( const VariablePtr& out_link : it->second.out_links )
	{
		out_links.push_back( out_link );
		delta_.AddLinkOperation( Delta::LinkOperationKind::Remove, node, out_link );
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
