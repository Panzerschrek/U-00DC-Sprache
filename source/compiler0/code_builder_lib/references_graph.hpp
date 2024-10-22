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

	// Removes also inner reference node and children nodes.
	void RemoveNode( const VariablePtr& node );

	void AddLink( const VariablePtr& from, const VariablePtr& to );

	// May emit ReferenceProtectionError.
	void TryAddLink( const VariablePtr& from, const VariablePtr& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	// Destination should contain no more inner references than source.
	void TryAddInnerLinks( const VariablePtr& from, const VariablePtr& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	// Create inner links between tuple node and tuple element node.
	// "from" shold be tuple.
	void TryAddInnerLinksForTupleElement( const VariablePtr& from, const VariablePtr& to, size_t element_index, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	// Create inner links between class node and class field node.
	// "from" shold be class.
	void TryAddInnerLinksForClassField( const VariablePtr& from, const VariablePtr& to, const ClassField& field, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	// Each access to variable must produce temporary reference to it.
	// Creating temporary mutable reference to reference node with outgoing links is compilation error.
	bool HasOutgoingLinks( const VariablePtr& from ) const;

	bool HasOutgoingMutableNodes( const VariablePtr& from ) const;

	void MoveNode( const VariablePtr& node );
	bool NodeMoved( const VariablePtr& node ) const;

	using NodesSet= std::unordered_set<VariablePtr>;
	NodesSet GetAllAccessibleVariableNodes( const VariablePtr& node ) const;
	NodesSet GetNodeInputLinks( const VariablePtr& node ) const;

	NodesSet GetAllAccessibleNonInnerNodes( const VariablePtr& node ) const;

	// Recursively search references graph starting from "to" in order to reach inner reference node of some variable.
	// Than create link between "from" and this node.
	void TryAddLinkToAllAccessibleVariableNodesInnerReferences( const VariablePtr& from, const VariablePtr& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	using MergeResult= std::pair<ReferencesGraph, std::vector<CodeBuilderError> >;
	static MergeResult MergeVariablesStateAfterIf( llvm::ArrayRef<ReferencesGraph> branches_variables_state, const SrcLoc& src_loc );
	static std::vector<CodeBuilderError> CheckVariablesStateAfterLoop( const ReferencesGraph& state_before, const ReferencesGraph& state_after, const SrcLoc& src_loc );

private:
	struct NodeState
	{
		bool moved= false;
	};

	struct Link
	{
		VariablePtr src;
		VariablePtr dst;

		bool operator==( const Link& r ) const;
	};

private:
	bool HasDirectOutgoingLinks( const VariablePtr& from ) const;
	bool HasOutgoingLinksIncludingChildrenLinks_r( const VariablePtr& from ) const;

	bool HasDirectOutgoingMutableNodes( const VariablePtr& from ) const;
	bool HasOutgoingMutableNodesIncludingChildrenNodes_r( const VariablePtr& from ) const;

	void RemoveNodeLinks( const VariablePtr& node );
	void GetAllAccessibleVariableNodes_r( const VariablePtr& node, NodesSet& visited_nodes_set, NodesSet& result_set ) const;

	void GetAllAccessibleNonInnerNodes_r( const VariablePtr& node, NodesSet& result_set ) const;

	void TryAddLinkToAllAccessibleVariableNodesInnerReferences_r( const VariablePtr& from, const VariablePtr& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

private:
	std::unordered_map<VariablePtr, NodeState> nodes_;
	std::vector<Link> links_; // Check for duplicates before insertion!
};

} // namespace U
