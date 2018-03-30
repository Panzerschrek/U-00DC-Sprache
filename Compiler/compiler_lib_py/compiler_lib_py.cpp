#include <cmath>
#include <iostream>

#include <Python.h>

#include "../src/lexical_analyzer.hpp"
#include "../src/syntax_analyzer.hpp"
#include "../src/code_builder.hpp"

// sprache_compiler_lib.pyd

static PyObject* test_echo( PyObject* self, PyObject* args )
{
	const char* command= nullptr;

	if( !PyArg_ParseTuple( args, "s", &command ) )
		return nullptr;
	std::cout << command << std::endl;
	return Py_None;
}

static PyMethodDef sprace_methods[]=
{
	{ "test_echo",  test_echo, METH_VARARGS, "Test echo." },
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
