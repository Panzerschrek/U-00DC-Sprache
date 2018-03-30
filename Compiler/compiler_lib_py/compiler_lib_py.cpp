#include <cmath>
#include <iostream>

#include <Python.h>

#include "../src/assert.hpp"
#include "../src/lexical_analyzer.hpp"
#include "../src/syntax_analyzer.hpp"
#include "../src/code_builder.hpp"
#include "../src/source_graph_loader.hpp"

namespace U
{

class SingeFileVfs final : public IVfs
{
public:

	SingeFileVfs( ProgramString file_path, const char* const text )
		: file_path_(file_path), file_text_(text)
	{}

	virtual boost::optional<LoadFileResult> LoadFileContent( const Path& file_path, const Path& full_parent_file_path ) override
	{
		U_UNUSED( full_parent_file_path );
		if( file_path == file_path_ )
			return LoadFileResult{ file_path_, DecodeUTF8( file_text_ ) };
		return boost::none;
	}

private:
	const ProgramString file_path_;
	const char* const file_text_;
};

void BuildProgram( const char* const text )
{
	const ProgramString file_path= "_"_SpC;
	const SourceGraphPtr source_graph=
		SourceGraphLoader( std::make_shared<SingeFileVfs>( file_path, text ) ).LoadSource( file_path );

	if( source_graph == nullptr ||
		!source_graph->lexical_errors.empty() ||
		!source_graph->syntax_errors.empty() ||
		!( source_graph->root_node_index < source_graph->nodes_storage.size() ) )
		return;

	const ICodeBuilder::BuildResult build_result= CodeBuilder().BuildProgram( *source_graph );

	for( const CodeBuilderError& error : build_result.errors )
		std::cout << error.file_pos.line << ":" << error.file_pos.pos_in_line << " " << ToStdString( error.text ) << "\n";
}

} // namespace U

static PyObject* TestEcho( PyObject* self, PyObject* args )
{
	const char* command= nullptr;

	if( !PyArg_ParseTuple( args, "s", &command ) )
		return nullptr;
	std::cout << command << std::endl;
	return Py_None;
}

static PyObject* BuildProgram( PyObject* self, PyObject* args )
{
	const char* program_text= nullptr;

	if( !PyArg_ParseTuple( args, "s", &program_text ) )
		return nullptr;

	U::BuildProgram( program_text );

	return Py_None;
}

static PyMethodDef sprace_methods[]=
{
	{ "test_echo",  TestEcho, METH_VARARGS, "Test echo." },
	{ "build_program",  BuildProgram, METH_VARARGS, "Build program." },
	{ nullptr, nullptr, 0, nullptr }        /* Sentinel */
};

static struct PyModuleDef module=
{
	PyModuleDef_HEAD_INIT,
	"sprache_compiler_lib",   /* name of module */
	nullptr, /* module documentation, may be NULL */
	-1,       /* size of per-interpreter state of the module,
				 or -1 if the module keeps state in global variables. */
	sprace_methods
};

PyMODINIT_FUNC PyInit_sprache_compiler_lib(void)
{
	return PyModule_Create( &module );
}
