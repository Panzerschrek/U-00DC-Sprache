#include "syntax_analyzer.hpp"

namespace Interpreter
{

Block::Block( BlockElements elements )
	: elements_( std::move( elements ) )
{
}

Block::~Block()
{
}

VariableDeclaration::~VariableDeclaration()
{
}

FunctionDeclaration::FunctionDeclaration(
	ProgramString name,
	ProgramString return_type,
	std::vector<VariableDeclaration> arguments,
	BlockPtr block )
	: name_( std::move(name) )
	, return_type_( std::move(return_type) )
	, arguments_( std::move(arguments) )
	, block_( std::move(block) )
{
}

FunctionDeclaration::~FunctionDeclaration()
{
}

} // namespace Interpreter
