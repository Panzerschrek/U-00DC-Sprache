#include <iostream>

#include "../../lex_synt_lib_common/assert.hpp"
#include "errors_print.hpp"

namespace U
{

namespace
{

void PrintErrorsGCC( const SourceGraph& source_graph, const CodeBuilderErrorsContainer& errors )
{
	for( const CodeBuilderError& error : errors )
	{
		if( error.code == CodeBuilderErrorCode::TemplateContext )
		{
			U_ASSERT( error.template_context != nullptr );

			std::cerr
				<< source_graph.nodes_storage[ error.template_context->context_declaration_src_loc.GetFileIndex() ].file_path << ":"
				<< error.template_context->context_declaration_src_loc.GetLine() << ":"
				<< error.template_context->context_declaration_src_loc.GetColumn() << ": "
				<< "In instantiation of \"" << error.template_context->context_name
				<< "\" " << error.template_context->parameters_description
				<< ":\n";

			std::cerr << source_graph.nodes_storage[error.src_loc.GetFileIndex() ].file_path
				<< ":" << error.src_loc.GetLine() << ":" << error.src_loc.GetColumn() << ": required from here\n";

			PrintErrorsGCC( source_graph, error.template_context->errors );
		}
		else if( error.code == CodeBuilderErrorCode::MacroExpansionContext )
		{
			U_ASSERT( error.template_context != nullptr );

			std::cerr
				<< source_graph.nodes_storage[ error.template_context->context_declaration_src_loc.GetFileIndex() ].file_path << ":"
				<< error.template_context->context_declaration_src_loc.GetLine() << ":"
				<< error.template_context->context_declaration_src_loc.GetColumn() << ": ";

			if( error.template_context->context_name == "mixin" )
				std::cerr << "In expansion of mixin:\n";
			else
				std::cerr << "In expansion of macro \"" << error.template_context->context_name << "\":\n";

			std::cerr << source_graph.nodes_storage[ error.src_loc.GetFileIndex() ].file_path
				<< ":" << error.src_loc.GetLine() << ":" << error.src_loc.GetColumn() << ": required from here\n";

			PrintErrorsGCC( source_graph, error.template_context->errors );
		}
		else
		{
			std::cerr << source_graph.nodes_storage[ error.src_loc.GetFileIndex() ].file_path
				<< ":" << error.src_loc.GetLine() << ":" << error.src_loc.GetColumn() << ": error: " << error.text << "\n";
		}
	}
}

void PrintErrorsMSVC( const SourceGraph& source_graph, const CodeBuilderErrorsContainer& errors )
{
	for( const CodeBuilderError& error : errors )
	{
		if( error.code == CodeBuilderErrorCode::TemplateContext )
		{
			U_ASSERT( error.template_context != nullptr );

			PrintErrorsMSVC( source_graph, error.template_context->errors );

			std::cerr << source_graph.nodes_storage[ error.template_context->context_declaration_src_loc.GetFileIndex() ].file_path
				<< "(" << error.template_context->context_declaration_src_loc.GetLine() << "): note: "
				<< "In instantiation of \"" << error.template_context->context_name
				<< "\" " << error.template_context->parameters_description
				<< "\n";

			std::cerr << source_graph.nodes_storage[ error.src_loc.GetFileIndex() ].file_path
				<< "(" << error.src_loc.GetLine() << "): note: required from here\n";
		}
		else if( error.code == CodeBuilderErrorCode::MacroExpansionContext )
		{
			PrintErrorsMSVC( source_graph, error.template_context->errors );

			if( error.template_context->context_name == "mixin" )
				std::cerr << source_graph.nodes_storage[ error.template_context->context_declaration_src_loc.GetFileIndex() ].file_path
					<< "(" << error.template_context->context_declaration_src_loc.GetLine() << "): note: "
					<< "In expansion of mixin\n";
			else
				std::cerr << source_graph.nodes_storage[ error.template_context->context_declaration_src_loc.GetFileIndex() ].file_path
					<< "(" << error.template_context->context_declaration_src_loc.GetLine() << "): note: "
					<< "In expansion of macro \"" << error.template_context->context_name << "\"\n";

			std::cerr << source_graph.nodes_storage[error.src_loc.GetFileIndex() ].file_path
				<< "(" << error.src_loc.GetLine() << "): note: required from here\n";
		}
		else
		{
			std::cerr << source_graph.nodes_storage[ error.src_loc.GetFileIndex() ].file_path
				<< "(" << error.src_loc.GetLine() << "): error: " << error.text << "\n";
		}
	}
}

} // namespace

void PrintErrors( const SourceGraph& source_graph, const CodeBuilderErrorsContainer& errors, const ErrorsFormat format )
{
	switch( format )
	{
	case ErrorsFormat::GCC: return PrintErrorsGCC( source_graph, errors );
	case ErrorsFormat::MSVC: return PrintErrorsMSVC( source_graph, errors );
	}

	U_ASSERT(false);
}


void PrintErrorsForTests( const SourceGraph& source_graph, const CodeBuilderErrorsContainer& errors )
{
	// For tests we print errors as "file.u 88 NameNotFound"
	for( const CodeBuilderError& error : errors )
		std::cout << source_graph.nodes_storage[ error.src_loc.GetFileIndex() ].file_path
			<< " " << error.src_loc.GetLine() << " " << CodeBuilderErrorCodeToString( error.code ) << "\n";
}

} // namespace U
