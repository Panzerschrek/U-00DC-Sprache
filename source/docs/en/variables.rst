Variables
=========

A variable may be defined like this:

.. code-block:: u_spr

   var i32 x= 0;

It's possible to define multiple variables of the same type in a single definition:

.. code-block:: u_spr

   var i32 x= 0, y= 1, z= 2;

Mutability and reference modifiers are specified for each variable individually:

.. code-block:: u_spr

   var i32 mut a= 0, imut b= 0;
   var i32 &mut a_ref= a, &imut b_ref= b, & b_ref2= b, imut y= 66, mut z= -56;

************
*References*
************
Reference modifier ``&`` means that a variable is a reference.
This means, that this reference will point to some another variable.
A modification of a reference means modification of the source variable.

Reference declaration doesn't create copy of a value, that may be useful, when copying is a performance-heavy operation.

************
*Mutability*
************
A variable may be declared with three possible mutability modifiers:

* ``mut`` - variable may be mutated after definition
* ``imut`` - variable may not be mutated after definition
* ``constexpr`` - variable must be compile-time constant

If there is no explicit mutability modifier a variable is assumed to be ``imut``.

****************
*Initialization*
****************
In the examples above all variables are initialized with ``=``.
But this is not the only way of the initialization, another initializer kinds are possible depending on the variable type.
Some types doesn't require explicit initialization at all.
You can read more about initialization in the :doc:`corresponding chapther </initializers>`.

.. _auto-variables:

****************
*auto variables*
****************

There is a possibility to define a variable without explicit type specification, the type may be deduced from the initializer.
There is a special syntax for this - ``auto`` variable declaration.
It consists of ``auto`` keyword, optional reference and mutability modifiers, variable name and initializer expression after ``=``.

.. code-block:: u_spr

   auto x = 0; // Immutable auto variable. Its type is "i32".
   auto mut y = 0.5f; // Mutable auto variable. Its type is "f32".
   
   auto &mut y_ref= y; // Mutable auto reference. Its type is "f32".
   auto &imut x_ref0= x; // Immutable auto reference. Its type is "i32".
   auto & x_ref0= x; // Immutable (by-default) auto reference. Its type is "i32".
   
   var [ bool, 16 ] arr= zero_init;
   auto& arr_ref= arr;// Immutable auto reference. Its type is "[ bool, 16 ]".

******************
*Global variables*
******************

It's possible to define variables outside functions - in the root namespace, namespaces, inside structs and classes.
But these variables have a limitation - they must be compile-time constants (``constexpr``).

.. code-block:: u_spr

   auto global_var = 55;
   var f32 global_f0= 0.25f, global_f1 = 555.1f;
   
   namespace NN
   {
       auto constexpr nn_var = global_var;
       var bool imut b = global_f0 < 66.0f;
   }
   
   struct S
   {
       var [ i32, 42 ] zeros = zero_init;
       auto constexpr zero24_plus2 = zeros[24] + 2;
   }

**************************
*Global mutable variables*
**************************

Global mutable variables are similar to immutable global variables.
They must be a ``constexpr`` type and have ``constexpr`` initializer.

Access to global mutable variables is possible only in ``unsafe`` code - including reading and writing.
It's necessary, since there is no reference checking or any synchronization mechanisms for global variables.
A programmer should manually guarantee that no reference checking rules are violated and no data races happen during access to global mutable variables.

Global mutable variables are declared like immutable ones, but with ``mut`` modifier.

.. code-block:: u_spr

   auto mut global_int = 66;
   var f32 mut global_float = 0.25f;

The only substantial difference between mutable and immutable global variables is a possibility of mutable references creation.
There are forbidden, since it's not possible to synchronize access properly.


thread_local variables
-----------------------

``thread_local`` variables are just global mutable variables, but having only one difference - each thread has its own copy of such variable.
They have the same limitations as regular global mutable variables - it's allowed to access them only in ``unsafe`` code, only ``constexpr`` types are allowed for them.
Their syntax is different from regular variables - it's necessary to specify ``thread_local`` keyword, following by type name and list of variables (with initializers), separated by comma. Reference and mutability modifiers aren't allowed.

.. code-block:: u_spr

   thread_local i32 x= zero_init, y(1), z= 2;
