#pragma once
#include "../compiler0/code_builder_lib/code_builder.hpp"

namespace U
{

namespace LangServer
{

using CompletionSyntaxElement= std::variant<
	const Synt::RootNamespaceNameLookupCompletion*,
	const Synt::NameLookupCompletion*,
	const Synt::NamesScopeNameFetchCompletion*,
	const Synt::MemberAccessOperatorCompletion*,
	const Synt::StructNamedInitializer::MemberInitializer*,
	const Synt::MoveOperatorCompletion*,
	const Synt::Function::NameComponent*,
	const Synt::CallOperatorSignatureHelp*,
	const Synt::ConstructorInitializerSignatureHelp*,
	const Synt::Lambda::CaptureListElement*,
	const Synt::DecomposeDeclarationStructComponent::Entry*>;

using GlobalItem= std::variant<
	const Synt::Namespace*,
	const Synt::VariablesDeclaration*,
	const Synt::AutoVariableDeclaration*,
	const Synt::StaticAssert*,
	const Synt::TypeAlias*,
	const Synt::Enum*,
	const Synt::Function*,
	const Synt::ClassField*,
	const Synt::ClassVisibilityLabel*,
	const Synt::Class*,
	const Synt::TypeTemplate*,
	const Synt::FunctionTemplate*,
	const Synt::Mixin*>;

struct SyntaxTreeLookupResult
{
	std::vector<CodeBuilder::CompletionRequestPrefixComponent> prefix;
	CompletionSyntaxElement element;
	GlobalItem global_item;
};

using SyntaxTreeLookupResultOpt= std::optional<SyntaxTreeLookupResult>;

// Complexity is linear.
SyntaxTreeLookupResultOpt FindCompletionSyntaxElement( uint32_t line, uint32_t column, const Synt::ProgramElementsList& program_elements );

} // namespace LangServer

} // namespace U
