### Motivation

For now the only way to build an Ü program is to execute the Ü compiler directly or use some existing build tool to do this, like _make_, _ninja_ or _Cmake_.
Using such tools isn't easy and they are not designed to be used with Ü.
So, it would be better to have a special tool for building Ü projects.

This document roughly describes this tool - what should it do and how.


### Name

One of possible names for Ü build system is _Bürokratie_.
This is a nice German word and includes language name Ü inside.


### Approach

An Ü project should be defined via a root build file.
This file describes what and how should be built.

One of the possible ways of doing this is to use some declarative description - describe files to build, targets (executables, libraries), dependencies.
But such declarative way isn't flexible enough - it doesn't allow to vary declarations based on some conditions.
So, a more powerful approach is required - with Turing-completeness.

The most natural way to achieve Turing-completeness is to use a proper programming language for build system files.
Using Ü itself as such language is the obvious solution.
A programmer doesn't need to learn a separate language just to create build files, it can use the Ü language itself.

Ü language usage means no practical isolation.
This means that build code may basically do whatever it wants - read/write files, access network, etc.
It's not nice, but it's acceptable price.
Maybe some OS-level isolation mechanisms may be used in future to limit network access and filesystem access (allow only build directory access).


### Build system interface

The build tool should be an executable file with support of different commands:

* build project - all or only selected targets
* run tests
* (maybe) prepare installation package
* (maybe) run checks
* run an executable within a project
* (maybe) build a single-file project with no dependencies

The build tool should have a command-like option for target triple specification.
If it's not specified, default triple is used (likely host target triple).
For each target triple a separate build directory is created.

Another option is configuration - release, debug, release with minimal size, etc.
Custom build types should be supported too.
Each configuration has its own subdirectory inside build directory of the target triple.

Creating different build configurations via command line allows more precise configuration, compared to approaches like in Visual Studio, where release and debug builds are mixed.

Of course there should be an option to specify number of threads to build, default value should be set to number of logical CPUs in the system.


### Build process

At start of a build process (at the build system executable invocation) the root build file (an Ü source) is compiled for host system into a shared library.
Such compilation is very limited - only one file can be used (but other files may be imported), no dependencies are allowed except Ü standard library, build system-specific headers and dependencies which may be directly imported.
This build root file should provide a specific function, which returns a declarative list of build targets, dependencies, maybe other information.
Then the shared library created on previous step is dynamically loaded by the build system executable and this specific function is called in order to obtain all information necessary.
Then the build system can perform the build.

One of possible optimizations - it's not necessary to recompile the root build file (and build files of dependencies) each time, recompilation only required on change.
Other possible optimization - a single shared library may be used for all build files (root file + dependent packages), in order to speed-up build system startup.

The build system should support incremental building.
If a source file or an entire build target isn't changed - there is a corresponding artifact with modification time newer than modification time of the source file, no actual compiler invocation is performed.


### Build targets

An Ü project consists of zero or more build targets.
Following build targets are supported:

* Executable file
* Ü library (internally just a LLVM bitcode file)
* C static library (with platform-dependent format)
* Dynamic (shared) library
* Custom target
* (maybe) test target

Each build target has set of source files, also it has list of directories with public header files.
A build target may have dependencies on other targets - public or private.

There should be a mechanism to detect and report errors when public headers of a target include headers of its private dependencies.


### Build script stage file generation

Ü build system should support generating files directly via the build script.
This is primary intended for source files generation using some simple approach, like basic text file processing, where writing a separate generator tool is too complex.

There should be a mechanism to avoid overwriting generated file if its contents didn't changed - in order to support incremental builds.

This functionality is similar to `generate_file` function in CMake.


### Custom targets

Build script stage files generation is pretty limited.
It doesn't allow using complex dependencies for files generation.
So, when something more satisfied is needed, custom build targets may be used.

A custom build target is just a target with a custom build rule, specified inside the build file via a custom functor.
This feature is needed for running code for non-Ü dependencies building and Ü code generation.

Each custom target has output file(s) which it produces.
Also it may have input dependencies on files and/or other build targets.

Other targets may depend on custom targets, like an Ü library which requires generation of some headers prior to building it.

Custom target building functor may use another build target as its command.
Such build target should be built for the host triple in order to be used as a build command.
This is necessary if some generation/building tool should be built prior to its usage (is distributed in source form).


### Packages

A package is defined via a single build script file.
It combines several build targets, has set of dependent packages.
Build targets of a package may be referenced in other packages in format like "package_name:build_target" or something similar.
If no package prefix is specified, a target from current package is assumed.

The package concept is the highest level code organization unit.
For now über-packages or sub-packages are not planned.
But technically it should be possible to create a package which just creates a build target just combining build targets from several other packages.


### Default package target

A package can have default target.
It may be used in other projects directly, using only this package name, with no build target name specified.
Importing headers of this default target should require only package name prefix itself at import path start.

This feature is designed for packages consisting of a single build target or with only single main build target - in order to avoid verbosity by using this target.


### Cross-compilation

Ü build system should work properly with cross-compilation.
For example, it should be possible to build a shared library for Android with 64-bit ARM processors, using Windows on x86-64 machine.

Target system (precisely target triple in LLVM terms) is specified as an option for the build system executable execution.

In single build each target may be built for:

* target system
* host system (in order to use it in build process)
* (maybe) both target and host systems

If host and target systems are identical, there should be no duplicated builds if a build target is is specified both as target system and host system build target.


### Packages dependency resolution

There should be a way to declare dependent packages (like libraries).

The simplest option is to specify dependency as a filesystem path.
Such approach works for cases where a dependent package is shipped together with the package which uses it - for example via git submodules.

There should be also a way to specify a package from a remote repository, possibly centralized.
In such case required package version should be specified.

The build system should perform proper version selection, trying to find the oldest possible version which satisfies requirements specified.
For example if a library A has dependent libraries B and C and B depends on package D of version 3 and C depends on package D of version 5, version 5 should be selected.
But is for example B, C, or both has D as their private dependency, both versions 3 and 5 should be used and properly isolated.
Such approach allows for software to be stable - ensures a library is built with dependencies of preferable versions.

Rust build system (for example) uses another approach - it tries to find he newest possible version with backward compatibility still present.
But such approach seems to be problematic.
It's generally impossible to guarantee backward compatibility, it's just assumed that backward compatibility is present if major version number isn't changed.
Such assumption may be easily broken due to oversight.

Unifying versions of dependencies isn't necessary, if two versions of the same library have no chance to cause a conflict - are used in separate places of the whole project, even in different part of the same executable.
Unification may just save a little bit of built time, in cost of decreasing stability due to dependency hell.

When the default package versions resolution approach doesn't give suitable version for some package, there should be a possibility to override its version manually.


### Linking

Ü libraries are not linked.
They are just combined from different bitcode files (generated from their source files).
No dependencies are combined (this happens later).

Native code targets - static libraries, executables, shared (dynamic) libraries are properly linked.
All used Ü libraries are loaded, all symbols from them are internalized except symbols which need to be exported.
Than an LTO pass may be run.
After that native code is generated.
In case of executables and shared libraries dependent static libraries (including externally-built C libraries) are linked in order to produce the result file.

The symbols internalization step is necessary in order to avoid collisions of symbols from different versions of the same library or just names from different libraries which just happen to be the same.

Generating result native code on per-target rather than on per-source-file basis allows link-time optimizations, like inlining or duplicated code elimination.
The second is important, since it's allowed to link together different version of the same dependency.
In such case it allows to reduce result executable size overhead by deduplicating code identical for two or more versions of a library.


### Compiler options

Generally the build system itself sets all compiler options - depending on target triple and configuration.
But there should be a possibility to override at least some of compiler options.
Such override should be global - be applied for all code within a project, in order to prevent possible inconsistency due to different options.
In rare cases where custom compiler options for only specific code parts are needed, a custom build target with manual compiler invocation can be used.


### Imports managing

The Ü build system should create some sort of a virtual file system for imports in Ü source files of each build target.
Via such virtual filesystem dependencies of a build target should be accessed via import directive with name starting with dependency name, like
```
import "/some_library_name/some_header.uh"
import "/some_other_library/another_header.uh"
import "some_own_header.uh"
```

Imports of dependencies from other packages should be prefixed with package name:
```
import "/package_a/some_lib/inc.uh"
import "/package_b/other_lib/abc.uh"
```

It shouldn't be allowed to import files outside allowed imported directories by misusing ".." in import paths.
Preventing this should allow catching nasty errors with dependencies managing.
Using global paths for imports (like "D:/files/some_header.uh" or "/home/panzerschrek/Projects/some_project/some_header.uh") should be disallowed to.

There also should be a way to import generated files.
One possible approach to do this is to declare a directory with generated files as public headers directory for a target.


### Out of scope features

Some functionality isn't planned or at least not planned in first versions.
This includes:

* PGO builds
* Other than Ü languages support. If necessary, custom build targets may be used to build code written in other languages.
* Linking with C or C++ code via integrated build system functionality. But custom targets may be used to achieve this.


### Language server integration

There should be a way for the language server to interact with the build system - to obtain list of project's files and import directories.

One way of doing this is to integrate build system directly into the language server.
This may work, but may increase result executable size.

Another way is to generate during first build a file with all necessary information and put it into the build directory.
The language server can read this file.
The only problem is to find this file.
Standardizing build directory location may help.


### Implementation strategy

The natural way to write Ü build system is to use write it in Ü itself.
But this may be a little bit hard, since Ü for now has no functionality for filesystem integration, so, such functionality should be created.

Of course Ü build system can't use Ü build system for building itself, at least for first version.
But it's fine, the Ü compiler may be used directly.
Later the Ü build system may be adopted for self-hosted build.

A significant problem may be build system testing.
A lot of test cases involving real build system invocation should be created.
But some functionality may be hard to test using this way, so, unit-testing of some specific parts should be used too.

Ü compiler may be also modified to work together with the Ü build system.
This may include include directories virtualization/limitations, modes for external dependencies extractions, shared libraries support, etc.
