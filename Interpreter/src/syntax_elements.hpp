#pragma once
#include <memory>
#include <vector>

#include "lexical_analyzer.hpp"

namespace Interpreter
{

class IProgramElement
{
public:
	virtual ~IProgramElement(){}
};

typedef std::unique_ptr<IProgramElement> IProgramElementPtr;
typedef std::vector<IProgramElementPtr> ProgramElements;

class IBlockElement
{
public:
	virtual ~IBlockElement(){}
};

typedef std::unique_ptr<IBlockElement> IBlockElementPtr;
typedef std::vector<IBlockElementPtr> BlockElements;

class Block final : public IBlockElement
{
public:
	Block( BlockElements elements );
	virtual ~Block() override;

private:
	const BlockElements elements_;
};

typedef std::unique_ptr<Block> BlockPtr;

struct VariableDeclaration final : public IBlockElement
{
	virtual ~VariableDeclaration() override;

	ProgramString name;
	ProgramString type;
};

typedef std::unique_ptr<VariableDeclaration> VariableDeclarationPtr;

class FunctionDeclaration final : public IProgramElement
{
public:
	FunctionDeclaration(
		ProgramString name,
		ProgramString return_type,
		std::vector<VariableDeclaration> arguments,
		BlockPtr block );

	virtual ~FunctionDeclaration() override;

private:
	const ProgramString name_;
	const ProgramString return_type_;
	const std::vector<VariableDeclaration> arguments_;
	const BlockPtr block_;
};

} // namespace Interpreter
