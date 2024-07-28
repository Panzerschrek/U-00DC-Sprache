#pragma once
#include "syntax_elements.hpp"

namespace U
{

namespace Synt
{

void WriteProgram( const ProgramElementsList& program_elements, std::ostream& stream );
void WriteExpression( const Synt::Expression& expression, std::ostream& stream );
void WriteTypeName( const Synt::TypeName& type_name, std::ostream& stream );
void WriteFunctionDeclaration( const Synt::Function& function, std::ostream& stream );
void WriteFunctionParamsList( const Synt::FunctionType& function_type, std::ostream& stream );
void WriteFunctionTypeEnding( const FunctionType& function_type, std::ostream& stream );
void WriteFunctionTemplate( const FunctionTemplate& function_template, std::ostream& stream );

} // namespace Synt

} // namespace U
