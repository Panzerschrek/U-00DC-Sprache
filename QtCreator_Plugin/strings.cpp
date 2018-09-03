#include "strings.h"

namespace U
{

namespace QtCreatorPlugin
{

QString ProgramStringToQString( const ProgramString& program_string )
{
	return QString::fromUtf16( program_string.data(), int(program_string.size()) );
}

ProgramString QStringToProgramString( const QString& q_string )
{
	return ProgramString( q_string.utf16(), size_t(q_string.length()) );
}

} // namespace QtCreatorPlugin

} // namespace U
