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
	struct Delta
	{
	public:
		struct AddNodeOp{ VariablePtr node; };
		struct MoveNodeOp{ VariablePtr node; };
		struct RemoveNodeOp{ VariablePtr node; };
		struct AddLinkOp{ VariablePtr from; VariablePtr to; };
		struct RemoveLinkOp{ VariablePtr from; VariablePtr to; };

		using Operation= std::variant<AddNodeOp, MoveNodeOp, RemoveNodeOp, AddLinkOp, RemoveLinkOp>;
	public:
		std::vector<Operation> operations;

	public:
		void ProcessAddNode( const VariablePtr& node );
		void ProcessMoveNode( const VariablePtr& node );
		void ProcessRemoveNode( const VariablePtr& node );
		void ProcessAddLink( const VariablePtr& from, const VariablePtr& to );
		void ProcessRemoveLink( const VariablePtr& from, const VariablePtr& to );
	};

public:
	ReferencesGraph()= default;
	ReferencesGraph( const ReferencesGraph& )= delete;
	ReferencesGraph operator=( const ReferencesGraph& )= delete;

public: // Delta stuff.
	Delta TakeDeltaState();
	Delta CopyDeltaState() const;

	void RollbackChanges( Delta prev_delta_state );

	void ApplyBranchingStates( llvm::ArrayRef<Delta> branches_states, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );
	void CheckLoopBodyState( CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

public:
	void AddNode( const VariablePtr& node );
	void AddNodeIfNotExists( const VariablePtr& node );

	// Removes also inner reference node and children nodes.
	void RemoveNode( const VariablePtr& node );

	void AddLink( const VariablePtr& from, const VariablePtr& to );
	void RemoveLink( const VariablePtr& from, const VariablePtr& to );

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
	bool HaveOutgoingLinks( const VariablePtr& from ) const;

	bool HaveOutgoingMutableNodes( const VariablePtr& from ) const;

	void MoveNode( const VariablePtr& node );
	bool NodeMoved( const VariablePtr& node ) const;

	using NodesSet= std::unordered_set<VariablePtr>;
	NodesSet GetAllAccessibleVariableNodes( const VariablePtr& node ) const;
	NodesSet GetNodeInputLinks( const VariablePtr& node ) const;

	// Recursively search references graph starting from "to" in order to reach inner reference node of some variable.
	// Than create link between "from" and this node.
	void TryAddLinkToAllAccessibleVariableNodesInnerReferences( const VariablePtr& from, const VariablePtr& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

private:
	struct NodeState
	{
		bool moved= false;
		// Duplicates are not allowed.
		llvm::SmallVector<VariablePtr, 3> in_links;
		llvm::SmallVector<VariablePtr, 3> out_links;
	};

private:
	bool HaveDirectOutgoingLinks( const VariablePtr& from ) const;
	bool HaveOutgoingLinksIncludingChildrenLinks_r( const VariablePtr& from ) const;

	bool HaveDirectOutgoingMutableNodes( const VariablePtr& from ) const;
	bool HaveOutgoingMutableNodesIncludingChildrenNodes_r( const VariablePtr& from ) const;

	void RemoveNodeLinks( const VariablePtr& node );
	void GetAllAccessibleVariableNodes_r( const VariablePtr& node, NodesSet& visited_nodes_set, NodesSet& result_set ) const;

	void TryAddLinkToAllAccessibleVariableNodesInnerReferences_r( const VariablePtr& from, const VariablePtr& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

private:
	std::unordered_map<VariablePtr, NodeState> nodes_;
	Delta delta_;
};

} // namespace U
