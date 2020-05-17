#pragma once
#include "syntax_elements.hpp"

namespace U
{

namespace Synt
{

void WriteProgramElement( const ComplexName& complex_name, std::ostream& stream );
void WriteProgram( const ProgramElements& program_elements, std::ostream& stream );

} // namespace Synt

} // namespace U
