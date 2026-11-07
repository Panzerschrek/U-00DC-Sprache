This article describes how unit-testing functionality in Ü can be implemented.

## Stage 0 - simple test-targets

The Ü build system should get a new build target type - a test executable.
It's just like regular executable, with only difference that it's handled specially when launching tests.
Such targets should be (probably) build in regular builds, so that breaking changes in code on which test targets depend can be detected and corrected as soon as possible.

The Ü build system should get a new command - *test* or something similar.
It should build the whole project including test targets and execute these targets.
If a test target failed to execute properly, it means failed test.
It should be displayed in the build logs which tests have succeeded and which have failed.

The *test* command should perform proper building for the specified target triple, assuming it's compatible with host triple and test executables built for it can be run on the host machine.
But even if such compatibility isn't present, cross-compiling tests should work properly, just in case someone can (somehow) run tests on the target architecture/OS distinct from the host one.

There may be a problem with tests having shared library dependencies.
If a dependency is located in a directory distinct from the parent directory of the test executable, it may not be found.
Some workaround is necessary for such cases, like changing *LD_LIBRARY_PATH* variable or creating symlinks for shared libraries.


## Stage 1 - inline unit-tests

The Ü language itself should get a special kind of construction - a unit-test function, which can be defined as regular one, but with slightly-different syntax, with no arguments, no return value and no other attributes (`nomangle`, virtual specifiers, `nodiscard`, calling convention, `unsafe`, reference notation).
Such functions should be built as regular ones without a necessity to running the build in some sort of testing configuration, so that compilation errors in tests can be detected and corrected as soon as possible.

Unit-tests should be named, so that it's possible to show names of failed tests (in order to simplify finding and correcting such tests).
Names can be also useful if it's only needed to run only one particular test or tests with names satisfying some pattern.
Name uniqueness shouldn't be enforced, since there is no strong reason for doing this.
Accessing tests by name from surrounding code shouldn't be possible, they should be hidden and use generated unique names with some prefix like *_* internally, so that it's impossible to access them.

Unit tests should be allowed in the global namespace, in nested namespaces, in structs and classes (as member functions with no `this` parameter).
In the latter case unit-tests should have proper access rights to `private` class members.

Unit tests in templates may be useful - in case if someone want to test a template in all possible instantiations.
But maybe it's not so useful and even problematic and thus such tests shouldn't be allowed.
At least it's possible to prohibit unit-tests in templates in the first version of the unit-tests implementation and if necessary to allow them later.

The intended way to fail a unit-test is to use `halt` or some macro based on it.
It's pretty convenient way of doing so, no extra language functionality is needed for that.
Such `halt`-based testing can not only detect simple failure cases with `halt` (or `assert`) written directly in the test's body, but also detect `halt` executed somewhere else down the call stack.

LLVM functions for unit-tests should be somehow processed specially.
They may be marked with some attribute, their visibility may be `public` or something similar may be added.
It's necessary, so that it's possible to collect later such tests during linking stage and to execute them.

The Ü build system should be able to collect unit-test functions from build targets of any kind, link them into test executables and run them as regular test executables described above.
It seems that for each build target a second test target can be created, which is composed of the same code (LLVM-bitcode), but is linked specially, so that all tests are there and a custom entry point capable of launching unit-tests is linked-in.
Regular entry point (*main*, *wmain*) should be removed from test executables and replaced with special entry point for tests.
Unit-tests should be removed from regular build targets, at least in release builds - in order to avoid bloating result artifacts.

For each build target only its unit-tests should be included in the result tests executable.
Unit-tests from dependencies (both private and public) should not be run and ideally should be removed.
Also unit-tests from shared-library dependencies should not be used.
The only exception are unit-tests defined in public imported files of dependencies, they should run while running tests for build targets where they are used.

It's necessary to write some library for launching unit-tests.
It should be able to launch tests, detect and (partially) recover failures, be able to receive command-line arguments from the build system in order to filter-out tests or run only specific tests.
It should define custom entry point (*main*, *wmain*).

Probably it may be useful to be able to disable building unit-test executables in the build system.
It may be necessary, if for some reason building them isn't possible (something is broken) for a particular target triple and/or in some particular environment.
Also it may be useful in order to reduce build-times, since creating a test executable for each build target may be expensive (require linking it and generating machine code).

It may be useful to have a way to disable some unit-tests, so that a message is shown during running tests how many tests are disabled and why particular tests are disabled.

### Handling failures

In normal programs handling `halt` is not a good idea and they should avoid doing that.
But for unit-testing there is no way avoiding this.
So, unit-test executable should use a custom `halt` handler.
Such handler should be able to return control flow to the code executing unit-tests.
The failed test should be marked properly (its failure should be reported) and remaining tests should be launched.

Since the same exact compiled code should be used for both regular build targets and unit-test targets, using custom `halt` handler may not be possible.
In such case some other approach giving the same result may be used instead, like an OS-specific handler for the target-dependent `trap` instruction (like `ud2` on x86 architecture).

Recovering from `halt` may be possible with some sort of `setjmp`/`longjmp` functionality.
Using it allows at least restoring stack state.
But generally it isn't a perfect recovery.
Memory allocated in a failed test, which should be deallocated via destructors, may not be deallocated, so that a leak may created.
Opened files and network connections may not be closed properly.
Threads spawned may continue running without someone waiting for them and destroying them.
Temporary files may remain permanent.
Mutexes may remain locked permanently.
All this is problematic, but it's a lesser evil.
Even with such problems present it may be still possible to run further tests, in hope that at least they are fine.
Languages like C++ or Rust can solve some of such problems by using stack unwinding, but it isn't ideal and adds another point of failure in destructors executed during unwinding.
Ü doesn't support ubnwinding, because it introduces additional speed and size overheads, so, it's not an option for recovering from failed unit-tests.

Extra consideration is necessary in order to prevent deadlocks in cases of failed unit-tests.
It's necessary, since some global mutexes may remain in locked state after a test is failed and subsequent locking of these mutexes in tests launched later may produce a deadlock.
It seems to be reasonable to run unit-tests on a separate thread.
In case if a test has failed, some shared atomic flag should be set to `true` on failure.
The main thread should check this flag from time to time.
If this flag is set, the main thread should not wait forever for the thread running tests to finish.
It should instead use some sort of timeout (like 10 seconds or so), and if it's expired, the runner thread should be considered to be deadlocked and the tests executable should finish quickly with an error.


### Linking tests

Unit-test functions should have public linkage, so that they are not removed while building each compiled Ü file of a build target.
They also should have some attribute - to distinguish them from regular functions.
Unit-test functions defined in imported files should be also public, but also use comdats in order to deduplicate them.

While linking a Ü library (composing LLVM bitcode files), these tests should be removed.
While linked a build target requiring machine code generation (object files, shared libraries, executables), tests should be removed too.

While a tests executable is linked, some special option should be provided to the compiler.
It should enable preserving all unit-test functions and putting them into some synthesized array or function with public visibility, so that it can be accessed later.
Also a special option may be necessary in order to force-internalize and thus remove `main` and `wmain` functions - default entry-points.

Linking library doing unit-tests launching can be done in form of an object file.
It should contain proper `main` or `wmain` function, so that the result tests executable gets proper entry point.
Also it should depend on a function/global variable synthesized by the compiler in the way described above, which provides this library the list of unit-tests to run.

In order to speed-up the build, Ü source files should be built into LLVM bitcode exactly once - with both regular code and tests included.
Nodes of the build graph linking the result (regular) build target and the result tests target should both depend on bitcode files compiled previously.
It's needed in order to avoid increasing build times much for building and linking tests.

In many cases a build target may contain no unit-tests at all.
In such case a tests executable may be still built into test executables running zero tests.
All should work fine in such cases too.
And it should be pretty fast to link such empty tests, since all code of a build target should be optimized-out and no machine code should be generated at all.
But if at least one unit-test is added, the result executable should reflect that and contain such test.


## Stage 2 - benchmarks

Benchmarks can be implemented almost like tests by using mostly the same functionality.
The only difference is in the way how they are launched.
It's probably fine to not care about `halt`-based failures in benchmarks and fail them using default `halt` handler.
Benchmarks should be run with precise time measurements and result times should be displayed.
For benchmarks with pretty small execution time it has sense to run them multiple times - in order to level possible noise in measurements and (maybe) CPU-cache and scheduling-related effects.
