#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../../code_builder_lib_common/code_builder_errors.hpp"
#include "value.hpp"

namespace U
{

class ReferencesGraph
{
public:
	void AddNode( const VariablePtr& node );
	void AddNodeIfNotExists( const VariablePtr& node );
	void RemoveNode( const VariablePtr& node );

	void AddLink( const VariablePtr& from, const VariablePtr& to );
	void RemoveLink( const VariablePtr& from, const VariablePtr& to );

	// Returns "false" in case of ReferenceProtectionError
	bool TryAddLink( const VariablePtr& from, const VariablePtr& to );

	VariablePtr GetNodeInnerReference( const VariablePtr& node ) const;
	VariablePtr CreateNodeInnerReference( const VariablePtr& node, ValueType kind );

	// Each access to variable must produce temporary reference to it.
	// Creating temporary mutable reference to reference node with outgoing links is compilation error.
	bool HaveOutgoingLinks( const VariablePtr& from ) const;

	bool HaveOutgoingMutableNodes( const VariablePtr& from ) const;

	void MoveNode( const VariablePtr& node );
	bool NodeMoved( const VariablePtr& node ) const;

	using NodesSet= std::unordered_set<VariablePtr>;
	NodesSet GetAllAccessibleVariableNodes( const VariablePtr& node ) const;
	NodesSet GetAccessibleVariableNodesInnerReferences( const VariablePtr& node ) const;
	NodesSet GetNodeInputLinks( const VariablePtr& node ) const;

	using MergeResult= std::pair<ReferencesGraph, std::vector<CodeBuilderError> >;
	static MergeResult MergeVariablesStateAfterIf( const std::vector<ReferencesGraph>& branches_variables_state, const SrcLoc& src_loc );
	static std::vector<CodeBuilderError> CheckWhileBlockVariablesState( const ReferencesGraph& state_before, const ReferencesGraph& state_after, const SrcLoc& src_loc );

private:
	struct NodeState
	{
		bool moved= false;
		VariablePtr inner_reference; // SPRACHE_TODO - make vector, when type can hold more, then one internal references storage.
	};

	struct Link
	{
		VariablePtr src;
		VariablePtr dst;

		bool operator==( const Link& r ) const;
	};

	struct LinkHasher
	{
		size_t operator()( const Link& link ) const;
	};

	using LinksSet= std::unordered_set< Link, LinkHasher >;

private:
	void RemoveNodeLinks( const VariablePtr& node );
	void GetAllAccessibleVariableNodes_r( const VariablePtr& node, NodesSet& visited_nodes_set, NodesSet& result_set ) const;
	void GetAccessibleVariableNodesInnerReferences_r( const VariablePtr& node, NodesSet& visited_nodes_set, NodesSet& result_set ) const;

private:
	std::unordered_map<VariablePtr, NodeState> nodes_;
	LinksSet links_;
};

} // namespace U
