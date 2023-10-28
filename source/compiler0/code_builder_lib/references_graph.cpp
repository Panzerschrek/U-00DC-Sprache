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

	if( node->parent.lock() == nullptr )
		for( const VariablePtr& inner_reference_node : node->inner_reference_nodes )
			AddNode( inner_reference_node );
}

void ReferencesGraph::AddNodeIfNotExists( const VariablePtr& node )
{
	if( nodes_.count( node ) == 0 )
		nodes_.emplace( node, NodeState() );

	for( const VariablePtr& inner_reference_node : node->inner_reference_nodes )
		AddNodeIfNotExists( inner_reference_node );
}

void ReferencesGraph::RemoveNode( const VariablePtr& node )
{
	if( nodes_.count(node) == 0 )
		return;

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
		for( const Link& link : links_ )
			if( link.dst == current_node )
				result.insert( link.src );

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

	for( const auto& link : links_ )
		if( link.dst == node )
			GetAllAccessibleVariableNodes_r( link.src, visited_nodes_set, result_set );

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
		for( const Link& link : links_ )
			if( link.dst == to )
				src_nodes.push_back( link.src );

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

			const NodeState& src_state= node_pair.second;
			const NodeState& result_state= result.nodes_[ node_pair.first ];

			if( result_state.moved != src_state.moved )
				REPORT_ERROR( ConditionalMove, errors, src_loc, node_pair.first->name );
		}

		for( const auto& src_link : branch_state.links_ )
		{
			if( std::find( result.links_.begin(), result.links_.end(), src_link ) == result.links_.end() )
				result.links_.insert( src_link );
		}
	}

	// Technically it's possible to create mutliple mutable references to same node or mutable reference + immutable reference.
	// But this is not an error, actually, because only one reference is created in runtime (depending on executed branch).
	// Seeming reference rules violation happens, because we perform variables state merging instead of maintaining two or more separate states.
	// Maintaining multiple variables states after each branching is computationally too costly.

	return std::make_pair( std::move(result), std::move(errors) );
}

std::vector<CodeBuilderError> ReferencesGraph::CheckWhileBlockVariablesState( const ReferencesGraph& state_before, const ReferencesGraph& state_after, const SrcLoc& src_loc )
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
	for( const auto& link : links_ )
		if( link.src == from )
			return true;

	return false;
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
	for( const auto& link : links_ )
		if( link.src == from && link.dst->value_type == ValueType::ReferenceMut )
			return true;

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
