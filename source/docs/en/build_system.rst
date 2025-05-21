Build system
============

**********
*Overview*
**********

The Ü project has its own build system, named *Bürokratie*.
It simplifies building complex programs written in Ü, comparing to manual usage of Ü compiler.
It's recommended to use it for building of Ü projects.

A project to build consists of one or more packages.
There is a root package, which may depend on other packages.

Each package should contain *build.u* file in its root directory.
This file is just an Ü source file, with contents like this:

.. code-block:: u_spr

   import "/build_system.uh"

   fn GetPackageInfo( BK::BuildSystemInterface &mut build_system_interface ) : BK::PackageInfo
   {
       ust::ignore_unused( build_system_interface );

       var BK::BuildTarget mut target{ .target_type = BK::BuildTargetType::Executable };
       target.source_files.push_back( "main.u" );
       target.name= "hello_world";

       return BK::PackageInfo{ .build_targets= ust::make_array( move(target) ) };
   }

This file should define function ``GetPackageInfo`` as shown above, which returns a structure describing this package.
The build system compiles this file into a native shared library, loads it and calls this function to obtain information about given package.

A package consists of build targets, custom build steps, package dependencies.
Dependent packages are loaded exactly like the root package - with compilation of their *build.u* file and shared library loading.

The build system is responsible for performing the build of a project.
It executes Ü compiler for source files, links executables, executes custom build steps, etc.
It ensures that execution order of all these actions is correct and tries to use multiple CPU cores to perform build faster.
Also the build system performs builds incrementally - it rebuilds files only if they or their dependencies are changed.

Ü language itself it selected for Ü build scripts in order to simplify writing such scripts.
It's not necessary to learn a separate language for project description files (like *make* or *cmake*) - it's only enough to know Ü.
Also using Ü makes it easier to write complex code needed for building of some projects, which may be not so nice with usage of other languages.
The biggest disadvantage of this approach is absence of build scripts isolation.
One can crash the build system by using ``halt`` in his build script or by messing with ``unsafe`` code.


**********
*Features*
**********

Build targets
-------------

A package may have zero or more build targets.
A build target is logical unit of a project.

There are following build target types:

* Executable
* Library - a build target on which other build targets can depend and use code from it
* Shared library - in native format, like *dll* or *so*
* Object file - in native format

A build target should have non-empty name unique within its package.
Result file names are based on this target name.

Each build target have list of source files, list of public include directories, lists of public and private dependencies, external libraries to link, etc.


Dependencies
------------

A build target may have dependencies on other build targets - from this package or from another packages.
It's possible to depend on libraries and shared libraries, but not executables or object files.

Dependencies of a build target are listed in ``public_dependencies`` and ``private_dependencies`` lists of the build target structure.
Each element of these lists contains build target name and package name, which by-default contains current package name.
For dependencies on build targets of other packages package name should be specified - as sub-package of current package, global versioned package or sub-package of a global versioned package.

Public dependencies are also dependencies of build targets which depend on current build target.
Usually a dependency should be public, if definitions from its headers are used in public header files of current build target.
But there may be some other reasons to make a dependency public.
If a dependency is used only internally (like its headers are imported only within source files), it should be made private.

A build target may have zero or more public include directories - directories where public header files are located.
Files from these directories may be imported by dependent build targets.
While importing a file from such directory its build target name may be used as prefix, like this:

.. code-block:: u_spr

   import "/SomeLibrary/some_library_header.uh"

*SomeLibrary* is a build target name.
Such import may be used in *SomeLibrary* build target and build targets which depend on it.

A build target can't have dependencies with identical names but from different packages.
This limitation exists because otherwise conflicts of imported files are possible.

It's important to mention that for dependencies on build targets of global versioned packages it's not guaranteed that the exact version specified will be actually used.
A newer version may be used instead, if it's needed for elimination of common dependencies on build targets of different versions.


External libraries
------------------

It's possible to specify for a build target external library dependencies - both shared and static libraries.
They are specified via ``external_libraries`` list of the build target structure.
Both absolute and relative paths are possible, but relative paths work only for system libraries (like *kernel32*).

External library dependencies are inherited - if a build target *LibA* has an external dependency on *SomeLib*, all build targets which depend on *LibA* will be linked against *SomeLib*.


Custom build steps
------------------

A package may contain custom build steps - additional actions which are performed to build this package.
Usually they are used for code generation, but may be used for other purposes.

Each custom build step have list of input and output files with absolute paths.
It's recommended to place output files within build directory of the current package.
These lists are used in order to schedule custom build steps execution properly - to ensure that a step generating a file is executed before another build step which uses this file.

There are several kind of commands which are possible for a custom build step:

* Running an external executable
* Running an executable built previously for host system
* Copying a file
* Creating a file with contents specified

A custom build step may have a comment, which is displayed during the build.
It's recommended to specify it.


Generated sources
-----------------

A custom build step may generate source or header files.
Such files should be placed within generated sources directory or generated public includes directory of a build target.

Each generated source file should be listed in ``generated_source_files`` - with name relative to the generated sources directory of this build target.
Each generated private header file should be listed in ``generated_private_header_files`` - with name relative to the generated sources directory of this build target.
Each generated public header file should be listed in ``generated_public_header_files`` - with name relative to the generated public headers directory of this build target.

Generated files may import other files using absolute paths with build target prefix.
Generated files may be imported by files of their build target or dependent build targets using absolute paths with build target prefix.

Specifying lists of generated sources/headers is necessary in order to ensure that custom build steps which generate them are executed prior to compilation of build targets of these sources/headers.


Package dependencies
--------------------

A package may have other dependent packages.
Such packages may be sub-packages of current package (located within a directory) or global versioned packages (or their sub-packages).

If build targets of a package depend on build targets from other packages, these packages should be listed in list of dependent packages.

There are following kinds of package dependencies:

* Target system dependency - default mode, which means, a package should be built for target system and its build targets may be used as dependencies of current package build targets
* Host system dependency - a package should be buily for host system. Its executable build targets may be used as commands for custom build steps.
* Both - combined target and host system dependency


************************
*Command-line interface*
************************

The build system executable supports following commands:

* build - perform the build
* build_single - build a program consisting of a single specified source file and having no dependencies
* init - initialize a stub project within current directory or directory specified via ``--project-directory`` option
* help - print help message and exit

There are also many options, which affect some commands.

``--project-directory`` option specifies path to the root package directory.
Default value is current directory.

``--build-directory`` option is used to provide custom build directory.
Default value is *build* subdirectory within root package directory.

``--build-configuration`` option selects build configuration.
Available configurations are ``release``, ``debug``, ``min_size_release``.

``--configuration-options`` option specifies path to a JSON file with additional configuration options.
Such file should be JSON object with string values.
These values are available for reading by package build scripts.

``--target-triple`` option allows specifying target triple in form *architecture-vendor-operating_system* or *architecture-vendor-operating_system-environment*.
Examples are *x86_64-unknown-linux-gnu* or *i686-pc-windows-msvc*.
The build system will perform the build for target triple specified.

``--target-cpu`` option allows to tune Ü compiler for some specific CPU.

``--sysroot`` option allows to specify path to the toolchain root directory for selected target system.
This may be necessary for cross-compilation.
``--host-sysroot`` may be used to specify path to the toolchain root directory used for host build targets building.

``--release-optimization-level`` option allows to specify optimization level for release builds - ``O2`` or ``O3``.

``--min-size-release-optimization-level`` option allows to specify optimization level for min size release builds - ``Os`` or ``Oz``.

``--halt-mode`` option can be used to tune ``halt`` language operator behavior.
See compiler's doculemtation for more details.

``-q`` option makes the build system executable quiet - it prints only error messages.
``-v`` option has an opposite meaning - the build system executable prints a lot of messages.
``-v`` option has priority over ``-q``.

``-j`` option specifies number of threads using for building.
Default value is 0, which means using all available CPU cores.

``--packages-repository-directory`` option provides path to the global packages directory.
Such directory should contain subdirectories (for each package) and one or more version directory within a package directories in format *major.minor.patch.tweak*.
This directory is used for searching for global versioned packages.

Options ``--compiler-executable``, ``--ustlib-path``, ``--build-system-imports-path`` are used to override default paths for components used by the build system - Ü compiler executable, standard library, imports directory containing build system headers.
It's not recommended to override these paths, unless it's really necessary to do so.
