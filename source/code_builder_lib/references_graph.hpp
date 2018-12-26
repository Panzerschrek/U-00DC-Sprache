#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include "../lex_synt_lib/program_string.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

struct ReferencesGraphNode;
using ReferencesGraphNodePtr= std::shared_ptr<ReferencesGraphNode>;

struct ReferencesGraphNode
{
	enum class Kind
	{
		Variable,
		ReferenceMut,
		ReferenceImut,
		ReferenceArg,
		ArgInnerVariable,
	};

	ProgramString name;
	Kind kind= Kind::Variable;
	ReferencesGraphNodePtr inner_reference; // TODO - make vector, when type can hold more, then one internal references storage.

	ReferencesGraphNode( ProgramString in_name, Kind in_kind ) : name(std::move(in_name)), kind(in_kind) {}
};

class ReferencesGraph
{
public:
	void AddNode( ReferencesGraphNodePtr node );
	void RemoveNode( const ReferencesGraphNodePtr& node );

	void AddLink( const ReferencesGraphNodePtr& from, const ReferencesGraphNodePtr& to );
	void RemoveLink( const ReferencesGraphNodePtr& from, const ReferencesGraphNodePtr& to );

	// Each access to variable must produce temporary reference to it.
	// Creating temporary mutable reference to reference node with outgoing links is compilation error.
	bool HaveOutgoingLinks( const ReferencesGraphNodePtr& from ) const;

	void MoveNode( const ReferencesGraphNodePtr& node );
	bool NodeMoved( const ReferencesGraphNodePtr& node ) const;

private:
	struct NodeState
	{
		bool moved= false;
	};

private:
	std::unordered_map<ReferencesGraphNodePtr, NodeState> nodes_;
	std::vector< std::pair<ReferencesGraphNodePtr, ReferencesGraphNodePtr> > links_;
};

} // namespace CodeBuilderPrivate

} // namespace U
