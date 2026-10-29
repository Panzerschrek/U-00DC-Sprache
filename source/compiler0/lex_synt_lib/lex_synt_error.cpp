#include "lex_synt_error.hpp"

namespace U
{

void PrintLexSyntErrors( const std::vector<std::string>& source_files, const LexSyntErrors& errors, const ErrorsFormat format, std::ostream& errors_stream )
{
	const std::string empty_file_path;
	for( const LexSyntError& error : errors )
	{
		const std::string* file_path= &empty_file_path;
		if( error.src_loc.GetFileIndex() < source_files.size() )
			file_path= &source_files[error.src_loc.GetFileIndex()];

		switch(format)
		{
		case ErrorsFormat::GCC:
			errors_stream << *file_path << ":" << error.src_loc.GetLine() << ":" << error.src_loc.GetColumn() << ": error: " << error.text << "\n";
			break;

		case ErrorsFormat::MSVC:
			errors_stream << *file_path << "(" << error.src_loc.GetLine() << "): error: " << error.text << "\n";
			break;
		};
	}
}

} // namespace U
