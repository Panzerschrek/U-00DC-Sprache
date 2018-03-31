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

std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
{
	const ProgramString file_path= "_"_SpC;
	const SourceGraphPtr source_graph=
		SourceGraphLoader( std::make_shared<SingeFileVfs>( file_path, text ) ).LoadSource( file_path );

	if( source_graph == nullptr ||
		!source_graph->lexical_errors.empty() ||
		!source_graph->syntax_errors.empty() ||
		!( source_graph->root_node_index < source_graph->nodes_storage.size() ) )
		return nullptr;

	ICodeBuilder::BuildResult build_result= CodeBuilder().BuildProgram( *source_graph );

	for( const CodeBuilderError& error : build_result.errors )
		std::cout << error.file_pos.line << ":" << error.file_pos.pos_in_line << " " << ToStdString( error.text ) << "\n";

	if( !build_result.errors.empty() )
		return nullptr;

	return std::move(build_result.module);
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

	std::unique_ptr<llvm::Module> module= U::BuildProgram( program_text );

	if( module == nullptr )
		return Py_None;

	// Convert pointer to integer handle.
	return Py_BuildValue( "n", module.release() );
}

static PyObject* FreeProgram( PyObject* self, PyObject* args )
{
	Py_ssize_t module_ptr= 0;

	if( !PyArg_ParseTuple( args, "n", &module_ptr ) )
		return nullptr;

	llvm::Module* const module= reinterpret_cast<llvm::Module*>( module_ptr );
	delete module;

	return Py_None;
}

static PyMethodDef sprace_methods[]=
{
	{ "test_echo",  TestEcho, METH_VARARGS, "Test echo." },
	{ "build_program",  BuildProgram, METH_VARARGS, "Build program." },
	{ "free_program",  FreeProgram, METH_VARARGS, "Free program." },
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

PyMODINIT_FUNC PyInit_sprache_compiler_tests_py_lib(void)
{
	return PyModule_Create( &module );
}
