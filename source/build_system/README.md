## Bürokratie - the Ü build system

WORK IN PROGRESS!

Many options and APIs aren't stabilized yet!


### About the build system

The Ü build system is designed to simplify Ü projects building.
It's easier to use it rather than using Ü compiler directly or wrapping its usage into something like _make_ files.

A project to build (its build targets, dependencies) is described in a declarative way, using a special build script file, written in Ü.
This gives flexibility in cases where it's needed, compared to pure declarative formats.
The build system compiles the build script file using Ü compiler into a shared library (in platform-dependent format), then loads it and calls a function from it in order to obtain project info (what and how to build).


### Usage

Run the build system executable with _init_ command.
This will create a stub project, including file _build.u_.
This initialization step is needed only once and is optional - the build script file may be written manually instead.

After a project is initialized in may be built.
To build a project run the build system executable within a directory contained such _build.u_ file.

By default the build system creates a directory named _build_ in the project directory, where all build results and intermediate files are placed.
But this may be changed via ``--build-directory`` command line option.
Also it's possible to choose project directory other than current via ``--project-directory`` option.

Build configuration may be specified via ``--build-configuration`` option, supported configurations are _release_ and _debug_.
Target triple may be specified with ``--target-triple`` option.
For more info, run the build system executable using ``--help`` option or ``help`` command.


### Features

For now following features are implemented:

* Executable build targets
* Library build targets
* Dependencies between build targets - public and private
* Shared library build targets
* Object file build targets
* Imports isolation - in each build target it's allowed to import only own header files or header files of dependencies
* Isolation of symbols in different libraries - in order to prevent possible name conflicts and have possibility to build different versions of the same library into one result binary
* Build results caching - if nothing was changed, nothing will be rebuilt, if only some source files were changed, only these files and their dependencies will be rebuilt.
* Build configurations - debug, release. Each configuration has its own set of compiler flags.
* Multithreaded building - several compilation/custom command processes can be run in parallel.
* Configuration options - for tweaking build targets
* Target triple specifying
* Limited (for now) cross-compilation support - using `--sysroot` option
* Packages - as subpackages within directories of another packages or as global versioned packages
* Generated sources and headers
* Host package dependencies - for building build tools


### Caveats

Since build scripts are normal Ü programs, it's possible to trigger a crash by using `halt` or by messing with unsafe code.
Since build script code is running inside the build system process, the whole process is terminated if an error in one of build script occurs.


### Necessary dependencies

If the build system executable was build using MSVC, Windows SDK installation in the system is required.
If the build system executable was build using MinGW, MinGW installation in the system is required and its path should be provided via `--sysroot` option.
Such dependencies are necessary in order to build build script shared libraries using the same (or at least compatible) runtime libraries as for the build system executable itself.

Building target code for different from host systems/environments is still possible, regardless of how the build system itself was built.
But it may be necessary to specify `--sysroot` in order to do this.
