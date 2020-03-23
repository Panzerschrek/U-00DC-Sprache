#include "../lex_synt_lib/assert.hpp"
#include "type.hpp"
#include "error_reporting.hpp"

namespace U
{

namespace ErrorReportingImpl
{

const char* GetErrorMessagePattern( CodeBuilderErrorCode code )
{
	switch(code)
	{
	#define PROCESS_ERROR(Code, Message) case CodeBuilderErrorCode::Code: return Message;
	#include "errors_list.hpp"
	#undef PROCESS_ERROR
	};

	U_ASSERT(false);
	return "";
}

const std::string& PreprocessArg( const std::string& str )
{
	return str;
}

std::string PreprocessArg( const CodeBuilderPrivate::Type& type )
{
	return type.ToString();
}

std::string PreprocessArg( const Synt::ComplexName& name )
{
	std::string str;
	for( const Synt::ComplexName::Component& component : name.components )
	{
		str+= component.name;
		if( &component != &name.components.back() )
			str+= "::";
	}
	return str;
}

} // namespace ErrorReportingImpl

} // namespace U
