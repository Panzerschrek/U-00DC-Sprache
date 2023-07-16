var interpreter_stdout = '';
var interpreter_stderr = '';

// This is main interpreter entry point. It accepts program text and returns result code, stdout, stderr.
function InterpreterCompileAndRun( program_text )
{
	// Clear contents.
	interpreter_stdout = '';
	interpreter_stderr = '';

	var file_name = 'test.u';
	FS.writeFile( file_name, program_text );
	var call_result = callMain( [file_name, '--entry', 'main'] );

	return [ call_result, interpreter_stdout, interpreter_stderr ];
};

function PirintText(text)
{
	if (arguments.length > 1)
		text = Array.prototype.slice.call(arguments).join('');
	console.log(text);
	interpreter_stdout += text;
};

function PirintErrText(text)
{
	if (arguments.length > 1)
		text = Array.prototype.slice.call(arguments).join('');
	console.log(text);
	interpreter_stderr += text;
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
