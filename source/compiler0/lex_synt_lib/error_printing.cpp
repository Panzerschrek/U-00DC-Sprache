#include "error_printing.hpp"

namespace U
{

void PrintLexSyntErrors( const SourceGraph& source_graph, const ErrorsFormat format, std::ostream& errors_stream )
{
	for( const LexSyntError& error : source_graph.errors )
	{
		std::string_view file_path;
		if( error.src_loc.GetFileIndex() < source_graph.nodes_storage.size() )
			file_path= source_graph.nodes_storage[ error.src_loc.GetFileIndex() ].file_path;

		switch(format)
		{
		case ErrorsFormat::GCC:
			errors_stream << file_path << ":" << error.src_loc.GetLine() << ":" << error.src_loc.GetColumn() << ": error: " << error.text << "\n";
			break;

		case ErrorsFormat::MSVC:
			errors_stream << file_path << "(" << error.src_loc.GetLine() << "): error: " << error.text << "\n";
			break;
		};
	}
}

} // namespace U
