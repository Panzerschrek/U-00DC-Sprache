### General packages design

Each package may have a set of dependent packages specified.
There are two kinds of dependencies: local sub-package and global versioned package.

Local sub-packages are just located within package directory of a package which uses them.
They are identified by directory path.

Global versioned packages may be located in a directory containing all global packages.
Each package directory contains one or more subdirectories with different package versions.
Versions are numbered, in format like 1.2.3.4 or something similar - allowing <=> comparison.
Alternatively or as addition a centralized packages repository may be used - with similar structure of packages and their versions.
Package code itself has no version information in it - version is specified only by directory name where it's located.

For global versioned packages dependency version unification may be used in case if one build target depends on more than one build target from the same package of different versions.
For local sub-packages it's impossible and two identical or similar packages within different directories are just considered to be totally different packages.


### Build directories management

Each package has stable path within project build directory.
For versioned packages this path looks like "versioned_packages/package_name/1.2.3.4/".
Root package is located in directory like "root".
Build directories of sub-packages are located within build directories of their parent packages.

The approach described above allows to use stable directory paths for generating sources/headers and/or executing custom commands.
Dependency version unification doesn't affect these paths.


### Cross-compilation support

Each package dependency specified may be specified as target system dependency and/or host system dependency.
Default dependency kind is target system dependency only.

If a package is specified as host system dependency, `GetPackageInfoImpl` function for it is called with target triple equal to host target triple and it will be built for the host system.
Package dependencies of such host-built packages will be built for the host system too (specifying dependency as target system dependency is equivalent in this case to specifying it as host system dependency).

A separate build subdirectory should be created for building packages for the host system.
But only necessary packages should be built for the host system.

It should be possible to obtain within `GetPackageInfoImpl` a path to host system build targets of host system dependency packages in order to use it in custom commands - to run an executable or to load a dynamic library.


### Limitations

As described above target system/host system building is specified on per-package basis.
It's not possible to build only some build targets withing a package for host system.
So, if it's necessary to build some built tool, a separate package for it should be created.

Circular dependencies between packages should be forbidden - there is no legit reason for circular dependencies.

It may be good to forbid sub-packages with more than one path component in name - allow "foo", but forbid "foo/bar".


### Centralized packages repository

It may be beneficial to have a centralized package repository.
Packages in this repository are identified by unique name.
Having more than one version of the same package is possible - with different version numbers.

There should be limits for package names.
Special symbols, slashes, spaces shouldn't be allowed.
Tricks like using Cyrillic letters instead of Latin in mostly Latin names should be forbidden.
Non-Latin package names may be considered, but with caution.

Some package names should be reserved and not be available for naming a project.
Such names should include popular terms, like "json", "cpp", "ü", "sql" and others.

Generally a package published can't be unpublished.
This includes each version of a package too.
It's made in order to avoid cases like with _leftpad_.

For some reasons, like legal reasons it may be necessary to delete contents of some packages.
In such case previous package version should replace new one.
In cases when this was first version uploading, something else should be done (it's not clear now).

At first package uploads should be manually pre-moderated.
Later less restricted approach should be used.

The main moderator of the repository should have a right to block a maintainer of some package (forbid him upload new versions) in case if package control was hijacked or package purpose was changed significantly without a particular reason.
Package control ownership in such case may be transferred to some other person instead, which promises to continue development.
The most popular/significant packages may adopted as part of the Ü project.

Packages in the centralized repository should be accepted only under conditions of one specific or some several similar permisseve licenses.
This is necessary to allow building both open source and proprietary software without legal problems.
So, uploading a GNU GPL package shouldn't be possible, as shouldn't be possible uploading something proprietary or semi-proprietary.
