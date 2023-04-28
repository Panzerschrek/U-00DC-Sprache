#pragma once
#include "syntax_elements.hpp"

namespace U
{

namespace Synt
{

void WriteProgram( const ProgramElements& program_elements, std::ostream& stream );
void WriteProgramElement( const ComplexName& complex_name, std::ostream& stream );
void WriteExpression( const Synt::Expression& expression, std::ostream& stream );
void WriteTypeName( const Synt::TypeName& type_name, std::ostream& stream );
void WriteFunctionDeclaration( const Synt::Function& function, std::ostream& stream );


} // namespace Synt

} // namespace U
