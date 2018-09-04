#pragma once
#include <QString>
#include "../lex_synt_lib/program_string.hpp"

namespace U
{

namespace QtCreatorPlugin
{

QString ProgramStringToQString( const ProgramString& program_string );
ProgramString QStringToProgramString( const QString& q_string );

} // namespace QtCreatorPlugin

} // namespace U
