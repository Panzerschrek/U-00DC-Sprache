extern char** environ;

extern "C" char** BKGetEnvironment()
{
	return environ;
}
