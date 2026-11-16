Unit-tests
==========

Ü has special syntax for unit-test functions.
They are defined with ``fn_test`` keyword usage and with name following in ``()``.

.. code-block:: u_spr

   fn_test( OnePlusTwoTest )
   {
       auto x= 1 + 2;
       halt if( x != 3 );
   }

Unit-test functions can be defined in the root namespace, in named namespaces, in structs and classes.
But defining a unit-test in a template (directly or indirectly) isn't allowed.

Names for unit-tests don't need to be all unique, duplicated names are allowed.
The compiler can give unique underlying names for two tests even if they have identical name.
Also names of unit-tests can't be referenced (a unit-test can't be called manually).

Unit-test functions are always compiled together with a file where they are defined.
Normally they are ignored at linking stage and optimized-out, so that result binaries don't contain unit-tests.
But they are used when testing is requested (see the corresponding page).
