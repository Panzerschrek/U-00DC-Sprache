#include "syntax_analyzer.hpp"

namespace Interpreter
{

FunctionDeclaration::FunctionDeclaration(
	ProgramString name,
	ProgramString return_type,
	std::vector<VariableDeclaration> arguments,
	Block block )
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
