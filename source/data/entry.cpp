/* This is simple C++ program, working as launcher for Ü program.
  Usage - wrote your Ü program, compile it into set of object files, compile it against this launcher.
  For example:
    gcc my_ü_program.o entry.cpp -o my_ü_program.exe

  This launcher developed for usage of Ü together with some C++ code. Launcher allows you to use C++ standart library,
  which initialized normally before C++ "main" function.
*/

extern int U_Main();

int main()
{
	U_Main();
}