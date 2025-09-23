![](source/docs/logo-Gebrochene-Grotesk.png)

## The Ü programming language

Ü is a statically-typed compiled programming language, designed for writing programs, which should be both reliable and fast.
It has safe and unsafe code separation, compile-time correctness checks, powerful abstractions like RAII and templates, encapsulation, rich type system, lambdas, coroutines and many other useful features.
Ü uses RAII for memory and resources management (no GC is involved), but manual memory management may be still used in unsafe code.
Ü is memory-safe and race-condition-safe, as long as no unsafe code is involved at all or as long as unsafe code is correctly written.

Ü is heavily inspired by C++, but doesn't have its downsides.
Also it was influenced by Rust, but only slightly and thus is way easier to use in comparison to Rust.
Any possible coincidence with design and features of other programming languages is unintentional.

Ü compiler is based on LLVM library and thus leverages many its powers, including numerous optimizations and code generation support for many CPU architectures and operating systems.
Even more, there are two Ü compilers, the first one is written in C++ and the second one is mostly written in Ü itself (frontend part, backend is still LLVM).

Besides the compiler Ü has a lot of other components.
Ü provides its own standard library containing basic container classes, helpers and operating system interaction functionality.
There is a build system, which simplifies complex Ü programs building and (partially) package management.
For better development experience there is a language server and variety of syntax highlighting files for some IDEs and text editors.
Last but not least, Ü has a tool for C headers conversion, which allows to simplify interaction with foreign code.


### Supported systems

The table below lists supported operating systems and architectures for Ü compiler hosting and targeting.

| Operating system | Architecture | Hosting Ü compiler | Targeting | Notes                                                                              |
|------------------|--------------|--------------------|-----------|------------------------------------------------------------------------------------|
| Windows          | x86          |  ✅                 | ✅         | MSVC installation may be necessary.                                                |
| Windows          | x86_64       |  ✅                 | ✅         | MSVC installation may be necessary. Supports also running Ü compiler built for x86.|
| GNU/Linux        | x86          |  ✅                 | ✅         | 32-bit C libraries installation may be necessary.                                  |
| GNU/Linux        | x86_64       |  ✅                 | ✅         |                                                                                    |
| GNU/Linux        | AArch64      |  ✅                 | ✅         |                                                                                    |
| FreeBSD          | x86_64       |  ✅                 | ✅         |                                                                                    |
| OS X             | AArch64      |  ✅                 | ✅➖        | Still experimental, some standard library functionality may not work properly.     |

CPU architectures besides x86, x86_64 and AArch64 aren't tested, but they may work if Ü compiler is built with these architecture support (see *LLVM_TARGETS_TO_BUILD* option).
GNU/Linux with x32 ABI isn't supported due to some bugs in LLVM library.


### Documentation

Documentation is available here: [english](https://panzerschrek.github.io/U-00DC-Sprache-site/docs/en/contents.html), [russian](https://panzerschrek.github.io/U-00DC-Sprache-site/docs/ru/contents.html).
The language itself is described in details, other components have basic, but not very deep documentation.
Additionally there are some basic usage [examples](source/examples/README.md).


### Why choosing Ü?

The short answer is: Ü is superior in comparison to many other programming languages in terms of safety, reliability, expressiveness and feature availability.
The table below compares important features, advantages and disadvantages of various programming languages, including Ü.
It lists only languages, which may be directly compared to Ü - statically-typed compiled languages without heavy runtime and/or GC.

| Feature/Language                                                                                 | C  | C++ | Swift | Zig | Odin | Rust | Ü |
|--------------------------------------------------------------------------------------------------|----|-----|-------|-----|------|------|---|
| constructors (special methods for construction, not just factory methods with user-defined name) | -  | +   | +     | -   | -    | -    | + |
| destructors (special methods called automatically at object destruction)                         | -  | +   | +     | -   | -    | +    | + |
| encapsulation (possibility to restrict access to some items only from some scopes)               | -  | +   | +     | +   | +    | +    | + |
| memory-safety (no out-of bounds read/writes, no use-after-free errors, etc.)                     | -  | -   | +⁶    | -   | -    | +    | + |
| thread-safety (no race conditions)                                                               | -  | -   | -     | -   | -    | +    | + |
| type templates                                                                                   | -  | +   | +     | +   | +    | +    | + |
| function templates                                                                               | -  | +   | +     | +   | +    | +    | + |
| duck-typing in templates (without mandatory template type requirements specification)            | -  | +⁴  | -     | +   | +    | -    | + |
| references (with auto reference creation and dereference)                                        | -  | +   | -⁷    | -   | -    | -¹⁵  | + |
| functions overloading                                                                            | -  | +   | +     | -   | +¹¹  | -    | + |
| operators overloading                                                                            | -  | +   | +     | -   | -    | +    | + |
| frictionless copying (ability to perform deep copy values via operator `=`)                      | -¹ | +   | +     | -⁹  | -¹²  | -¹⁶  | + |
| compile-time calculations                                                                        | -² | +   | -     | +   | +    | +    | + |
| compile-time type information                                                                    | -  | -   | -     | +   | -    | -    | + |
| class inheritance and runtime polymorphism based on it                                           | -  | +   | +     | -   | -¹³  | -    | + |
| no exceptions (means no possibility to implicitly skip control flow passing)                     | +³ | -   | +⁸    | +   | +    | ±¹⁷  | + |
| async functions                                                                                  | -  | +⁵  | +     | ±¹⁰ | -    | +    | + |
| lambdas (anonymous functions defined within expression context, sometimes named closures)        | -  | +   | +     | -   | -¹⁴  | +    | + |

<details>
<summary>footnotes</summary>

1 - structs may be copied via `=`, but it's only shallow copy.<br>
2 - there is only limited compile-time evaluation of constants like (1 + 2), but without compile-time variable constants and compile-time functions evaluation.<br>
3 - `setjump`/`longjump` is still possible, but generally speaking it's not a language feature and this may be implemented almost in any language.<br>
4 - in some rare cases `typename` keyword is needed in templates. There are also concepts in newer C++ standards, but one can just avoid using them if duck-typing is needed.<br>
5 - C++ has somewhat lower-level coroutines, which allow implementing not only async functions, but generators and other constructions.<br>
6 - Memory safety was added in new versions of the language.<br>
7 - There are generally no references, there are `inout` function parameters, but they require specifying `&` for parameter passing.<br>
8 - Swift has `throw` keyword, `throws` function specifier and `catch` statement, but there is no unexpected control flow, since each possible error value must be explicitly handled or passed further. So, what it does is more likely a second function return channel rather than proper exceptions.<br>
9 - operator `=` only creates shallow copy, just like in C.<br>
10 - async functions are now in development.<br>
11 - overloading is explicit and requires adding extra code.<br>
12 - operator `=` only creates shallow copy, just like in C.<br>
13 - there is subtype polymorphism, but no proper inheritance-based polymorphism with runtime dispatching based on actual runtime type (via virtual functions or something similar).<br>
14 - there is only non-capturing functions defined within other functions.<br>
15 - Rust so-called "references" are really just pointers, one need to add `&` to create a reference and use `*` for dereferencing.<br>
16 - all types are split into two categories, the first one allows copying via = (which is basically `memcpy`), the second one requires explicitly calling `clone` method.<br>
17 - exceptions can't be thrown within Rust code, but Rust supports stack unwinding (with destructors calling) if an exception was thrown from foreign code (like C++). Code should be written with unwinding possibility in mind.<br>

</details>


### How to build

A modern C++ compiler (clang, GCC, MSVC) is required for building the project.
CMake and Ninja are also necessary.

#### Option 0 - build with LLVM sources
* Download LLVM sources [here](https://github.com/llvm/llvm-project/releases/) (version 17.0.6 is used in this project).
* Run cmake for *source/CMakeLists.txt* file to generate a project for your favorite IDE or build system. You must set *LLVM_SRC_DIR* in cmake arguments.
* In order to speed-up the build you may disable building of unnecessary targets via *LLVM_TARGETS_TO_BUILD* cmake variable. For example set it to *X86* only.
* It's recommended to disable LLVM tests and benchmarks. See LLVM documentation for more information.
* Perform the build.

#### Option 1 - build with LLVM binary libraries
* Install proper version of LLVM libraries via your system package manager or download it manually [here](https://github.com/llvm/llvm-project/releases/).
* Run cmake for *source/CMakeLists.txt* file with *LLVM_LIB_DIR* cmake variable specified.
* Perform the build.

Some components are optional and may be disabled via cmake options (see *source/CMakeLists.txt*).
Python 3 is required to build and run tests, written in Python.
Sphynx is required to build documentation.
For more information/examples see build scripts in *ci* directory.


### Component readmes

[compiler](source/compilers_common_lib/README.md)

[language server](source/language_server/README.md)

[syntax highlighting](source/syntax_highlighting/README.md)

[standard library](source/ustlib/README.md)

[build system](source/build_system/README.md)

[C++ header converter](source/cpp_header_converter/README.md)

[interpreter](source/interpreter/README.md)

[plugin for QtCreator](source/qt_creator_plugin/README.md)

[extension for Visual Studio](source/visual_studio_extension/README.md)

[documentation](source/docs/README.md)

[examples](source/examples/README.md)


### IDE support

[Ecode](https://github.com/SpartanJ/ecode/) has built-in Ü syntax highlighting, Ü language server support, debugging support.
It's recommended at least to try using it.

[QtCreator](https://www.qt.io/product/development-tools) may be used as Ü IDE.
There is a [syntax highlighting file](source/syntax_highlighting/README.md) for it.
Also it supports custom language servers, which allows using the Ü language server.

There is an Ü extension for Microsoft Visual Studio.
See [corresponding readme](source/visual_studio_extension/README.md) for details.

Many other IDEs may be used too.
Any IDE with possibility to specify a custom language server may be used for Ü code writing.
Some IDEs also allow creating custom syntax highlighting rules, one can create it themselves.


### Downloads

The compiler downloads are available on the actions page - as action artifacts.
It's recommended to use one of the latest builds of the *master* branch.


### Authors

Copyright © 2016-2025 "Panzerschrek".
