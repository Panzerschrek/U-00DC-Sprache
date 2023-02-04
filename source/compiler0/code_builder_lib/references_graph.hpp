#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../../code_builder_lib_common/code_builder_errors.hpp"
#include "value.hpp"

namespace U
{

struct ReferencesGraphNode
{
	const std::string name;
	const ReferencesGraphNodeKind kind= ReferencesGraphNodeKind::Variable;

	ReferencesGraphNode( std::string in_name, ReferencesGraphNodeKind in_kind ) : name(std::move(in_name)), kind(in_kind) {}
};

class ReferencesGraph
{
public:
	ReferencesGraphNodePtr AddNode( ReferencesGraphNodeKind kind, std::string name );
	void RemoveNode( const ReferencesGraphNodePtr& node );

	void AddLink( const ReferencesGraphNodePtr& from, const ReferencesGraphNodePtr& to );
	void RemoveLink( const ReferencesGraphNodePtr& from, const ReferencesGraphNodePtr& to );

	// Returns "false" in case of ReferenceProtectionError
	bool TryAddLink( const ReferencesGraphNodePtr& from, const ReferencesGraphNodePtr& to );

	ReferencesGraphNodePtr GetNodeInnerReference( const ReferencesGraphNodePtr& node ) const;
	ReferencesGraphNodePtr CreateNodeInnerReference( const ReferencesGraphNodePtr& node, ReferencesGraphNodeKind kind );

	// Each access to variable must produce temporary reference to it.
	// Creating temporary mutable reference to reference node with outgoing links is compilation error.
	bool HaveOutgoingLinks( const ReferencesGraphNodePtr& from ) const;

	bool HaveOutgoingMutableNodes( const ReferencesGraphNodePtr& from ) const;

	void MoveNode( const ReferencesGraphNodePtr& node );
	bool NodeMoved( const ReferencesGraphNodePtr& node ) const;

	using NodesSet= std::unordered_set<ReferencesGraphNodePtr>;
	NodesSet GetAllAccessibleVariableNodes( const ReferencesGraphNodePtr& node ) const;
	NodesSet GetAccessibleVariableNodesInnerReferences( const ReferencesGraphNodePtr& node ) const;

	using MergeResult= std::pair<ReferencesGraph, std::vector<CodeBuilderError> >;
	static MergeResult MergeVariablesStateAfterIf( const std::vector<ReferencesGraph>& branches_variables_state, const SrcLoc& src_loc );
	static std::vector<CodeBuilderError> CheckWhileBlockVariablesState( const ReferencesGraph& state_before, const ReferencesGraph& state_after, const SrcLoc& src_loc );

private:
	struct NodeState
	{
		bool moved= false;
		ReferencesGraphNodePtr inner_reference; // SPRACHE_TODO - make vector, when type can hold more, then one internal references storage.
	};

	struct Link
	{
		ReferencesGraphNodePtr src;
		ReferencesGraphNodePtr dst;

		bool operator==( const Link& r ) const;
	};

	struct LinkHasher
	{
		size_t operator()( const Link& link ) const;
	};

	using LinksSet= std::unordered_set< Link, LinkHasher >;

private:
	void RemoveNodeLinks( const ReferencesGraphNodePtr& node );
	void GetAllAccessibleVariableNodes_r( const ReferencesGraphNodePtr& node, NodesSet& visited_nodes_set, NodesSet& result_set ) const;
	void GetAccessibleVariableNodesInnerReferences_r( const ReferencesGraphNodePtr& node, NodesSet& visited_nodes_set, NodesSet& result_set ) const;

private:
	std::unordered_map<ReferencesGraphNodePtr, NodeState> nodes_;
	LinksSet links_;
};

} // namespace U
