Testing
=======

Ü has special syntax for defining unit-tests.
See :doc:`unit_tests`.

The Ü build system extracts unit-tests for each build target (unless building unit-tests for a particular target is disabled) and builds an executable capable of launching them.
These unit-test executables are built always, there is no way to skip building them.
Unit-test executable for each build target includes only unit-tests defined in its sources and may include unit-tests defined in imported files of dependent build targets.

Running the Ü build system with *test* command allows building (if it's not done yet) and running all unit-test executables.
It's assumed that the target triple specified is compatible with the host target triple and thus running tests on host machine is possible.
If it's not the case, *test* command should not be used.
But it's still possible to take unit-test executables and run them manually on the target system.

Normally building a unit-test executable is possible for each build target.
But if it's not the case (like due to unresolved external dependencies) building unit-test executables can be disabled (individually for each build target).
In such case no unit-tests can be run, even if a build target contains unit-tests.


************************
*Handling test failures*
************************

There are no dedicated language means for testing conditions in unit-tests.
:doc:`halt` may be used for this.
Also a macro wrapping ``halt`` (like ``assert``) may work too.
Not only ``halt`` within test body is handled, but ``halt`` invocations in other functions directly or indirectly called.
Other failures like access violation due to incorrect ``unsafe`` code/external code, divisions by zero, stack overflows and others are also caught and reported.

There is a hard limit on the total number of failed tests which can be detected.
If too many tests have failed, no further tests for a given build target are launched and thus no further test failures can be reported.


************************
*Tips for writing tests*
************************

Unit-tests may be run in isolation from each other.
Their order isn't specified.
So, unit-tests should not implicitly depend on some particular global state.
If it's needed, such state should be prepared from within test body instead.
Such global state includes state of global mutable variables (defined in Ü code or in foreign code), state of OS primitives, filesystem state and generally any kind of state of the observable world.

It's recommended to report failed test conditions using *stdout* or *stderr*, like with a helper function or macro (assert?).
Doing so may simplify finding what exactly failed in a particular test.

.. code-block:: u_spr

   import "/assert.iu"

   fn_test( OnePlusTwoTest )
   {
       auto x= 1 + 2;
       // "assert" macro will print the given message in case of a failure.
       assert( x == 3, "1 + 2 isn't 3!" );
   }

For more advanced search for failure causes it's recommended to use a debugger.
Unit-test executables can be debugged as usual ones, assuming they are build in debug mode (with debug information enabled and optimizations disabled).

Unit-tests should be granular, if possible.
Each test should test only a handful of conditions closely related to each other.
This is needed, because only the first failure in a unit-test is detected and reported.
So, if a test checks 1000 conditions sequentially and condition #0 fails, remaining 999 conditions will not be checked and (if necessary) reported.

.. code-block:: u_spr

   // Prefer this:
   //
   fn_test( MulTest2x2 ) { halt if( 2 * 2 != 4 ); }
   fn_test( MulTest2x3 ) { halt if( 2 * 3 != 6 ); }
   fn_test( MulTest2x4 ) { halt if( 2 * 4 != 8 ); }
   // A lot of tests
   // ...
   fn_test( MulTest2x1000 ) { halt if( 2 * 1000 != 2000 ); }

   // Over this:
   //
   fn_test( MulTest )
   {
      halt if( 2 * 2 != 4 );
      halt if( 2 * 3 != 6 );
      halt if( 2 * 4 != 8 );
      // A lot of checks
      // ...
      halt if( 2 * 1000 != 2000 );
   }

If such a granular approach isn't possible, like in a test looping over a table containing test data, it's recommended to print parameters of each loop invocation into *stdout*.
This can simplify detecting which particular case triggers a failure.


********************
*Test build targets*
********************

There is one more way of doing testing, which is unrelated to unit-testing feature described above.
The Ü build system supports build targets of ``Test`` type.
They are just regular executables.
The only difference is that they are run together with other tests when the build system is executed with *test* command.
If such an executable exits with zero code, it's assumed to be a successful test run, but if exit-code is non-zero or if the test's process has crashed/been terminated, it's considered to be failed.

Test build targets and unit-tests aren't mutually-exclusive.
A build target of ``Test`` type can contain unit-tests and an executable running them can be build for it as usual and be run with *test* command - together with its parent executable.
