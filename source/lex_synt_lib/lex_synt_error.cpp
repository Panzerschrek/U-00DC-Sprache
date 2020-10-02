#include "source_graph_loader.hpp"
#include "lex_synt_error.hpp"

namespace U
{

void PrintLexSyntErrors( const SourceGraph& source_graph, const ErrorsFormat format, std::ostream& errors_stream )
{
	const std::string empty_file_path;
	for( const LexSyntError& error : source_graph.errors )
	{
		const std::string* file_path= &empty_file_path;
		if( error.file_pos.GetFileIndex() < source_graph.nodes_storage.size() )
			file_path= &source_graph.nodes_storage[error.file_pos.GetFileIndex()].file_path;

		switch(format)
		{
		case ErrorsFormat::GCC:
			errors_stream << *file_path << ":" << error.file_pos.GetLine() << ":" << error.file_pos.GetColumn() << ": error: " << error.text << "\n";
			break;

		case ErrorsFormat::MSVC:
			errors_stream << *file_path << "(" << error.file_pos.GetLine() << "): error: " << error.text << "\n";
			break;
		};
	}
}

} // namespace U
