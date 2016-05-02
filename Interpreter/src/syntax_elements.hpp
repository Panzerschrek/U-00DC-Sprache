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
typedef std::vector<IBlockElementPtr> Block;

struct VariableDeclaration
{
	ProgramString name;
	ProgramString type;
};

class FunctionDeclaration final : public IProgramElement
{
public:
	FunctionDeclaration(
		ProgramString name,
		ProgramString return_type,
		std::vector<VariableDeclaration> arguments,
		Block block );

	virtual ~FunctionDeclaration() override;

private:
	const ProgramString name_;
	const ProgramString return_type_;
	const std::vector<VariableDeclaration> arguments_;
	const Block block_;
};

} // namespace Interpreter
