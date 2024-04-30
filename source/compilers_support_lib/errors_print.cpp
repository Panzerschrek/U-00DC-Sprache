#include <iostream>

#include "../lex_synt_lib_common/assert.hpp"
#include "errors_print.hpp"

namespace U
{

namespace
{

void PrintErrorsGCC( const std::vector<IVfs::Path>& source_files, const CodeBuilderErrorsContainer& errors )
{
	for( const CodeBuilderError& error : errors )
	{
		if( error.code == CodeBuilderErrorCode::TemplateContext )
		{
			U_ASSERT( error.template_context != nullptr );

			std::cerr << source_files[ error.template_context->context_declaration_src_loc.GetFileIndex() ] << ": "
				<< "In instantiation of \"" << error.template_context->context_name
				<< "\" " << error.template_context->parameters_description
				<< "\n";

			std::cerr << source_files[error.src_loc.GetFileIndex() ]
				<< ":" << error.src_loc.GetLine() << ":" << error.src_loc.GetColumn() << ": required from here: " << "\n";

			PrintErrorsGCC( source_files, error.template_context->errors );
		}
		else if( error.code == CodeBuilderErrorCode::MacroExpansionContext )
		{
			U_ASSERT( error.template_context != nullptr );

			if( error.template_context->context_name == "mixin" )
				std::cerr << source_files[ error.template_context->context_declaration_src_loc.GetFileIndex() ] << ": "
					<< "In expansion of mixin" << "\n";
			else
				std::cerr << source_files[ error.template_context->context_declaration_src_loc.GetFileIndex() ] << ": "
					<< "In expansion of macro \"" << error.template_context->context_name << "\"\n";

			std::cerr << source_files[ error.src_loc.GetFileIndex() ]
				<< ":" << error.src_loc.GetLine() << ":" << error.src_loc.GetColumn() << ": required from here: " << "\n";

			PrintErrorsGCC( source_files, error.template_context->errors );
		}
		else
		{
			std::cerr << source_files[ error.src_loc.GetFileIndex() ]
				<< ":" << error.src_loc.GetLine() << ":" << error.src_loc.GetColumn() << ": error: " << error.text << "\n";
		}
	}
}

void PrintErrorsMSVC( const std::vector<IVfs::Path>& source_files, const CodeBuilderErrorsContainer& errors )
{
	for( const CodeBuilderError& error : errors )
	{
		if( error.code == CodeBuilderErrorCode::TemplateContext )
		{
			U_ASSERT( error.template_context != nullptr );

			PrintErrorsMSVC( source_files, error.template_context->errors );

			std::cerr << source_files[ error.template_context->context_declaration_src_loc.GetFileIndex() ]
				<< "(" << error.template_context->context_declaration_src_loc.GetLine() << "): note: "
				<< "In instantiation of \"" << error.template_context->context_name
				<< "\" " << error.template_context->parameters_description
				<< "\n";

			std::cerr << source_files[ error.src_loc.GetFileIndex() ]
				<< "(" << error.src_loc.GetLine() << "): note: " << error.text << "\n";
		}
		else if( error.code == CodeBuilderErrorCode::MacroExpansionContext )
		{
			PrintErrorsMSVC( source_files, error.template_context->errors );

			if( error.template_context->context_name == "mixin" )
				std::cerr << source_files[ error.template_context->context_declaration_src_loc.GetFileIndex() ]
					<< "(" << error.template_context->context_declaration_src_loc.GetLine() << "): note: "
					<< "In expansion of mixin\n";
			else
				std::cerr << source_files[ error.template_context->context_declaration_src_loc.GetFileIndex() ]
					<< "(" << error.template_context->context_declaration_src_loc.GetLine() << "): note: "
					<< "In expansion of macro \"" << error.template_context->context_name << "\"\n";

			std::cerr << source_files[error.src_loc.GetFileIndex() ]
				<< "(" << error.src_loc.GetLine() << "): note: required from here\n";
		}
		else
		{
			std::cerr << source_files[ error.src_loc.GetFileIndex() ]
				<< "(" << error.src_loc.GetLine() << "): error: " << error.text << "\n";
		}
	}
}

} // namespace

void PrintErrors( const std::vector<IVfs::Path>& source_files, const CodeBuilderErrorsContainer& errors, const ErrorsFormat format )
{
	switch( format )
	{
	case ErrorsFormat::GCC: return PrintErrorsGCC( source_files, errors );
	case ErrorsFormat::MSVC: return PrintErrorsMSVC( source_files, errors );
	}

	U_ASSERT(false);
}


void PrintErrorsForTests( const std::vector<IVfs::Path>& source_files, const CodeBuilderErrorsContainer& errors )
{
	// For tests we print errors as "file.u 88 NameNotFound"
	for( const CodeBuilderError& error : errors )
		std::cout << source_files[error.src_loc.GetFileIndex() ]
			<< " " << error.src_loc.GetLine() << " " << CodeBuilderErrorCodeToString( error.code ) << "\n";
}

} // namespace U
