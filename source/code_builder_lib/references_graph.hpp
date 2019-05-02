#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../lex_synt_lib/program_string.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

struct ReferencesGraphNode
{
	enum class Kind
	{
		Variable,
		ReferenceMut,
		ReferenceImut,
		ReferenceArg,
		ArgInnerVariableLevel0, // Arg or ReferenceArg have reference to
		ArgInnerVariableLevel1, // ArgInnerVariableLevel0 Have reference to. Also ArgInnerVariableLevel1 hae "virtual" reeference to itself.
	};

	const ProgramString name;
	const Kind kind= Kind::Variable;

	ReferencesGraphNode( ProgramString in_name, Kind in_kind ) : name(std::move(in_name)), kind(in_kind) {}
};

using ReferencesGraphNodePtr= std::shared_ptr<const ReferencesGraphNode>;

class ReferencesGraph
{
public:
	void AddNode( ReferencesGraphNodePtr node );
	void RemoveNode( const ReferencesGraphNodePtr& node );

	void AddLink( const ReferencesGraphNodePtr& from, const ReferencesGraphNodePtr& to );
	void RemoveLink( const ReferencesGraphNodePtr& from, const ReferencesGraphNodePtr& to );

	ReferencesGraphNodePtr GetNodeInnerReference( const ReferencesGraphNodePtr& node ) const;
	void SetNodeInnerReference( const ReferencesGraphNodePtr& node, ReferencesGraphNodePtr inner_reference );

	// Each access to variable must produce temporary reference to it.
	// Creating temporary mutable reference to reference node with outgoing links is compilation error.
	bool HaveOutgoingLinks( const ReferencesGraphNodePtr& from ) const;

	bool HaveOutgoingMutableNodes( const ReferencesGraphNodePtr& from ) const;

	void MoveNode( const ReferencesGraphNodePtr& node );
	bool NodeMoved( const ReferencesGraphNodePtr& node ) const;

	std::unordered_set<ReferencesGraphNodePtr> GetAllAccessibleInnerNodes_r( const ReferencesGraphNodePtr& node ) const;
	std::unordered_set<ReferencesGraphNodePtr> GetAllAccessibleVariableNodes_r( const ReferencesGraphNodePtr& node ) const;

private:
	struct NodeState
	{
		bool moved= false;
		ReferencesGraphNodePtr inner_reference; // SPRACHE_TODO - make vector, when type can hold more, then one internal references storage.
	};

private:
	std::unordered_map<ReferencesGraphNodePtr, NodeState> nodes_;
	// first - from, second - to
	std::vector< std::pair<ReferencesGraphNodePtr, ReferencesGraphNodePtr> > links_;
};

} // namespace CodeBuilderPrivate

} // namespace U
