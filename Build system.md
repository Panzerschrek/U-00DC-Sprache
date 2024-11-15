### Motivation

For now the only way to build an Ü program is to execute the Ü compiler directly or use some existing build tool, like _make_, _ninja_ or _cmake_.
Using such tools isn't easy and they are not designed to be used with Ü.
So, it would be better to have a special tool for building Ü code.

This document describes this tool - what should it do and how.


### Approach

An Ü project should be defined via a root build file.
This file describes what and how should be built.

One of the possible ways is to use some declarative description - describe files to build, targets (executables, libraries), dependencies.
But such declarative way isn't flexible enough - it doesn't allow to vary declarations based on some conditions.
So, a more powerfull approach is required - with Turing-completeness.

The most natural way to achieve Turing-completeness is to use a proper programming language for build system files.
Using Ü itself as such language is the obvious solution.
A programmer don't need to learn a separate language just to create build files, it can use Ü langauge itself.

Ü langauge useage means no practical isolation.
This means that build code may basically do whatever it wants - read files, access network, etc.
It's not nice, but it's acceptable price.
Maybe some OS-level isolation machanisms may be used in future to limit network access and filesystem access (allow only build directory access).


### Build process

At start of a build process the root build file (a Ü source) is compiled for host system into a shared library.
Such compilation is very limited - only one file can be used (but other files may be imported), no dependencies are allowed except Ü standard library, build system-specific headers and dependencies which may be directly imported.
This build root file should provide a specific function, which returns a declarative list of build targets, dependencies, maybe other information.
Then this shared library is dynamically loaded by the build system executable and this specific function is called in order to obtain all information necessary.
Then the build system can perform the build.

One of possible optimizations - it's not necessary to recompile the root build file (and build files of dependencies) each time, recompilation only required on change.
Other possible optimization - a single shared library may be used for all build files (root file + dependencies), in order to speed-up build system startup.

The build system should support incremental building.
If a source file or an entire target isn't changed - there is a corresponding artifact with modification time newer than modification time of the source file, no actual compiler invocation is performed.


### Build system interface

The build tool should be an executable file with support of different commands:
* build project - all or only selected targets
* run tests
* (maybe) prepare installation package
* (maybe) run checks

The build tool should have a command-like option for target triple specification.
If it's not specified, default triple is used (likely host target triple).
For each target triple a separate build directory is created.

Another option is configuration - release, debug, release with minimal size, etc.
Custom build types should be supported too.
Each configuration has its own subdirectory inside build directory of target triple.

Creating different build configurations via command line allows more precise configuration, compared to approaches like in Visual Studio, where release and debug builds are mixed.

Of course there should be an option to specify number of threads to build, default value should be set to number of logical CPUs in the system.


### Build targets

An Ü project consists of many build targets.
Following build targets are supported:
* Executable file
* Ü library (internally just a LLVM bitcode file)
* C static library (with platform-dependent format)
* Dynamic (shared) library
* Custom target
* (maybe) test target

Each build target has set of source files, also it has list of public header files.
A build target may have dependencies on other targets - public or private.

There should be a machanism of detection of reporting of errors when public headers of a target include headers of private dependency.


### Build script stage file generation

Ü build system should support generating files directly within build stript.
This is primary intended for source files generation used some simple approach, like basic text file processing, where writing a separate generator tool is too complex.

There should be a mechanism to avoid overwriting generated file if its contents didn't changed - in order to support incremental builds.

This functionality is similar to `generate_file` function in CMake.


### Custom targets

Build script stage files generation is pretty limited.
It doesn't allow using complex dependencies.
So, when something more satisfied is needed, custom build targets may be used.

A custom target is just a target with custom build rule, specified unside build file via a custom functor.
This feature is needed for running code for non-Ü dependencies building and Ü code generation.

Each custom target has output file(s) which it produces.
Also it may have input dependencies on files and/or build targets.

Other targets may depend on custom targets, like an Ü library which requires generation of some headers prior to building it.

Custom target building functor may use another build target as its command.
Such build taret should target host triple in order to be used for it.
This is necessary if some generation/building tool should be built prior to its usage (is distributed in source form).


### Cross-compilation

Ü build system should work properly with corss-compilation.
For example, it should be possible to build a shared library for Android with 64-bit ARM processors, using Windows on x86-64 machine.

Target system (precisely target triple in LLVM terms) is specified as option for build system execution.

In single build each target may be built for:
* target system
* host system (in order to use it in build process)
* (maybe) both target and host systems


### Packages dependency resolution

There should be a way to declare dependent packages (like libraries).

The simplest option is to specify dependency as a filesystem path.
Such approach works for cases where a dependent package is shipped together with the root package, for example via git sumbodules.

There should be also a way to specify a package from a remote repository, possibly centralized.
In such case required package version should be specified.

The build system should perform proper version selection, trying to find the oldest possible version which can be used.
For example if a library A has dependent libraries B and C and B depends on package D of version 3 and C dependes on package D of version 5, version 5 should be selected.
But is for example B, C, or both has D as their private version, both versions 3 and 5 should be used and properly isolated.
Such approach allows for software to be stable - ensures a library is built with dependencies of preferrable versions.

Rust build system uses another approach - it tries to find a newest possible version with backward compatibility still present.
But such approach seems to be problematic.
It's generally impossible to guarantee backward compatibility, it's just assumed that backward compatibility is there if major version number isn't changed.

Unifying versions of dependencies isn't really necessary, if two versions of the same library have no change to cause a conflict - are used in separate places of the whole program.
Unification may just save a little bit of built time, in cost of decreasing stability due to dependency hell.


### Linking process

Ü libraries are not linked.
They are just combined from defferent bitcode files (generated from their source files).
No dependencies are combined (this happens later).

Native code targets - static libraries, executables, shared (dynamic) libraries are properly linked.
All used Ü libraries are loaded, all symbols from them are internalized, except symbols which need to be exported.
Than an LTO pass may be run.
After that native code is generated.
In case of executables and shared libraries dependent static libraries (including externally-built C libraries) are linked in order to produce result.

Internalization step is necessary in order to avoid collisions of symbols from different versions of the same library or just names from different libraries which happend to be the same.


### Out of scope features

Some functionality isn't planned or at least not planned in first versions.
This includes:
* PGO builds
* Other than Ü languages support. If necessary, custom build targets may be used to build code written in other languages


### Name

One of possible names is _Bürokratie_.
This is a nice german word and includes language name Ü inside.


### Language server integration

There should be a way for language server to interact with build system - to obtain list of project's files and import directories.

One way of doing this is to integrate build system directly into the language server.
This may work, but may increase result executable size.

Another way is to generate during first build a file with all necessary information and put it into the build directory.
The language server can read this file.
The only problem is to find this file.
Standartizing build directory location may help.


### Implementation strategy

The natural way to write Ü build system is to use Ü itself.
But this may be a little bit hard, since Ü for now has no functionality for filesystem interation, so, such functionality should be created.

And of course Ü build system can't use Ü build system for building itself, at least for first version.
But it's fine, the Ü compiler may be used directly.

A significant problem may be build system testing.
A lot of test cases involving real build system invocation should be created.
But some functionality may be hard to test using this way, so, unit-testing of some specific parts should be used too.

Ü compiler may be also modified to work together with Ü build system.
This may include include directories virtualisation/limitations, modes for external dependencies extractions, shared libraries support, etc.
