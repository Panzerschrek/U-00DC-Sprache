#include <iostream>
#include <fstream>
#include <sstream>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/JSON.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../lex_synt_lib_common/assert.hpp"
#include "connection.hpp"
#include "parser.hpp"

namespace U
{

namespace LangServer
{

#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h>
#include <io.h>

void PlatformInit()
{
	auto res = _setmode( _fileno(stdin), _O_BINARY );
	U_ASSERT(res != -1);
	res = _setmode( _fileno(stdout), _O_BINARY );
	U_ASSERT(res != -1);
}
#else
void platform_init() { }
#endif

int Main()
{
	PlatformInit();

	std::ofstream log_file( "C:/Users/user/Documents/Projects/other/U-00DC-Sprache/other/sprache_lang_server.txt", std::ios::app );

	Connection connection( std::cin, std::cout );
	while(true)
	{
		const std::string message= connection.Read();
		log_file << "Message: " << message << std::endl;
		llvm::Expected<llvm::json::Value> parse_result= llvm::json::parse( message );
		if( parse_result )
		{
			const llvm::json::Value& value= parse_result.get();
			log_file << "JSON parsed successfully" << std::endl;
			if( const auto request= ParseRequestMessage( value ) )
			{
				log_file << "request with id= " << request->id << " and method= " << request->method << std::endl;

				llvm::json::Object response_obj;
				response_obj["id"]= request->id;

				std::string response_str;
				llvm::raw_string_ostream stream(response_str);
				stream << llvm::json::Object( std::move(response_obj) );
				stream.flush();

				log_file << "Response: " << response_str;
				connection.Write( response_str );
			}
			else
			{
				log_file << "Request parse error" << std::endl;
			}
		}
		else
		{
			log_file << "JSON parse error" << std::endl;
		}
	}

	return 0;
}

} // namespace LangServer

} // namespace U

int main()
{
	return U::LangServer::Main();
}
