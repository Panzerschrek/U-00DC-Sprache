#pragma once
#include <memory>
#include <unordered_set>
#include <vector>

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
		Mutable,
		Immutable,
		Variable,
		ArgInnerReference,
	};

	Kind kind= Kind::Variable;
	ReferencesGraphNodePtr inner_reference; // TODO - make vector, when type can hold more, then one internal references storage.
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

private:
	std::unordered_set<ReferencesGraphNodePtr> nodes_;
	std::vector< std::pair<ReferencesGraphNodePtr, ReferencesGraphNodePtr> > links_;
};

} // namespace CodeBuilderPrivate

} // namespace U
