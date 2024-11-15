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


### Requirements

An Ü project consists of many build targets.
Following build targets are supported:
* Executable file
* Ü library (internally just a LLVM bitcode file)
* C static library (with platform-dependent format)
* Dynamic (shared) library
* Custom target

Each build target has set of source files, also it has list of public header files.
A build target may have dependencies on other targets - public or private.


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

The build system should perform proper version selection, trying to find the smallest possible version which can be used.
For example if a library A has dependent libraries B and C and B depends on package D of version 3 and C dependes on package D of version 5, version 5 should be selected.
But is for example B, C, or both has D as their private version, both versions 3 and 5 should be used and properly isolated.
Such approach allows for software to be stable - ensures a library is built with dependencies of preferrable versions.

Rust build system uses another approach - it tries to find a newest possible version with backward compatibility still present.
But such approach seems to be problematic.
It's generally impossible to guarantee backward compatibility, it's just assumed that backward compatibility is there if major version number isn't changed.

Unifying versions of dependencies isn't really necessary, if two versions of the same library have no change to cause a conflict - are used in separate places of the whole program.
Unification may just save a little bit of built time, in cost of decreasing stability due to dependency hell.


### Name

One of possible names is _Bürokratie_.


### Language server integration