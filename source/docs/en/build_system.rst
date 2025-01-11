Build system
============


**********
*Features*
**********

Build targets
-------------

A package may have zero or more build targets.
A build target is logical unit of a project.

There are following build target types:

* Executable
* Library - a build target on which other build targets can depend on and use code from it
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
Usually a dependency should be public if definitions from its headers are used in public header files of current build target.
But there may be some other reasons to make a dependency public.
If a dependency is used only internally (like its headers are imported only within source files), it should be made private.

A build target may have zero or more public include directories - directories where public head files are located.
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


External dependencies
---------------------

It's possible to specify for a build target external library dependencies - both shared and static libraries.
They are specified via ``external_libraries`` list of the build target structure.
Both absolute and relative paths are possible, but relative paths work only for system libraries (like *kernel32*).

External dependencies are inherited - if a build target *LibA* has an external dependency on *SomeLib*, all build targets which depend on *LibA* will be linked against *SomeLib*.


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
Such packages may be sub-packages of current package (located within a directory), or global versioned packages (or their sub-packages).

If a package depends on build targets from other packages, these packages should be listed in list of dependent packages.

There are following kids of package dependencies:

* Target system dependency - default mode, which means, a package should be built for target system and its build targets may be used as dependencies of current package build targets
* Host system dependency - a package should be build for host system. Its executable build targets may be used as commands for custom build steps.
* Both - combined target and host system dependency
