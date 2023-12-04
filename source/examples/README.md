### About

This directory contains basic usage examples of some Ü features/constructions.
Each file is a self-contained example.

Note that these examples contain only limited set of all Ü features.
Read the documentation to know more about the language.


### How to build

Use the compiler to build each file, like this:

```
Compiler some_example.u -o some_example.exe --filetype=exe --include-dir ../ustlib/
```

It's important to specify correct path to the *ustlib*.
Different optimization options may be specified, see [compiler readme](../compilers_common_lib/README.md) for more information.

Alternatively it's possible to build all examples as part of the whole Ü project.
