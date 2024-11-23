## Bürokratie - the Ü build system

WORK IN PROGRESS!


### About the build system

The Ü build system is designed to simplify Ü projects building.
It's easier to use it rather than using Ü compiler directly or wrapping its usage into something like _make_ files.

A project to build (its build targets, dependencies) is described in a declarative way, using a special build script file, written in Ü.
This gives flexibility in cases where it's needed, compared to pure declarative formats.
The build system compiles the build script file using Ü compiler into a shared library (in platform-dependent format), then loads it and calls a function from it in order to obtain project info (what and how to build).


### Usage

Run the build system executable inside a directory, containing _build.u_ file.
This file should define a function named _GetPackageInfo_ returning a struct with package description (build targets, their sources, etc.).

An example of a build file:
```
import "/build_system.uh" // This file contains definitions of the build system types.

fn GetPackageInfo() : BK::PackageInfo
{
	// Create a target with single source file.
	var BK::BuildTarget mut target;
	target.source_files.push_back( "main.u" );
	target.name= "hello_world";
	return BK::PackageInfo{ .build_targets= ust::make_array( move(target) ) };
}

```

By default the build system creates a directory named _build_ in the project directory, where all build results and intermediate files are placed.

For more info, run the build system executable using ``--help`` option.
