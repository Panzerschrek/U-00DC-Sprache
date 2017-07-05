include (Common.pri)

TEMPLATE= subdirs

SUBDIRS= CompilerLib Tests Compiler
CompilerLib.file= CompilerLib.pro
Tests.file= Tests.pro
Compiler.file= Compiler.pro

Compiler.depends= Tests CompilerLib
Tests.depends= CompilerLib
