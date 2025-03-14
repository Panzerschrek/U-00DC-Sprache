var interpreter_stdout = '';
var interpreter_stderr = '';

// This is main interpreter entry point. It accepts program text and returns result code, stdout, stderr.
function InterpreterCompileAndRun( program_text )
{
	// Clear contents.
	interpreter_stdout = '';
	interpreter_stderr = '';

	// Compile some ustlib sources, but ignore all functionality requiring actual access to external system functions,
	// since we can't call them from the interpreter.
	ustlib_sources_to_compile=
	[
		'/ustlib/src/unix/main_wrapper.u',
		'/ustlib/src/unix/path_utils.u',
		// ignore stdout.u
		'/ustlib/src/integer_parsing.u',
		'/ustlib/src/string_conversions.u',
		// ignore stdin.u
		'/ustlib/src/utf.u',
	];

	var file_name = 'test.u';
	FS.writeFile( file_name, program_text );
	var call_result = callMain( [ file_name, '--entry', 'main', '--include-dir', '/ustlib/imports' ].concat( ustlib_sources_to_compile ) );

	return [ call_result, interpreter_stdout, interpreter_stderr ];
};

function PirintText(text)
{
	if (arguments.length > 1)
		text = Array.prototype.slice.call(arguments).join('\n');
	console.log(text);
	console.log('\n');
	interpreter_stdout += text;
	interpreter_stdout += '\n';
};

function PirintErrText(text)
{
	if (arguments.length > 1)
		text = Array.prototype.slice.call(arguments).join('\n');
	console.log(text);
	console.log('\n');
	interpreter_stderr += text;
	interpreter_stderr += '\n';
};

// This variable is used by template script.
var Module =
{
	preRun: [],
	postRun: [],
	print: PirintText,
	printErr: PirintErrText,
	setStatus: (text) => {},
	totalDependencies: 0,
	monitorRunDependencies: (left) => {},
	arguments: [],
	noInitialRun: true // Do not run at start, run only when it is necessary.
};
