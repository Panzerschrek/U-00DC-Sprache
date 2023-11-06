Function body elements
======================

A body of a function consists of sequention of elements.
Here are listed most important of these elements.

***********************
*Variables declaration*
***********************
See :doc:`variables`.

***************************
*Auto variable declaration*
***************************
See :ref:`auto-variables`.

*******************
*Simple expression*
*******************

Usually this is needed in order to call some function with side-effects.

.. code-block:: u_spr

   Foo(); // Call a function, that may have side-effects
   Add( 5, 7 ); // Call a function with or without side-effects 

************
*Assignment*
************

Assignment consists of left part - for destination and left part - for source.

.. code-block:: u_spr

   x = 2; // Assign a numeric value to a variable
   x = Foo() + Bar() - 66; // Assign a complex expression result to a variable
   Min( x, y )= 0; // It is possible too, if the result of "Min" function is a mutable reference

***********************************
*Operation combined with assinment*
***********************************

There are following operations combided with assignment:

* ``+=`` - addition
* ``-=`` - subtraction
* ``*=`` - multiplication
* ``/=`` - division
* ``%=`` - remainder
* ``&=`` - bitwise AND
* ``|=`` - bitwise OR
* ``^=`` - bitwise XOR
* ``=<<`` - right shift
* ``=>>`` - left shift

An operation with assignment is equivalent to call to corresponding binary operator for left and right parts of the operation with following assignment of the result to the left part.

.. code-block:: u_spr

   x += 2; // Increase value of "x" by 2
   x /= Foo() + Bar() - 66; // Devide variable "x" by the result of the right expression and than save result into "x"
   Min( x, y ) &= 0xFF; // Set upper bits of the "Min" call result variable to zero

*************************
*Increment and Decrement*
*************************

``++`` increases result of an integer expressions by one, ``--`` decreases it.

.. code-block:: u_spr

   ++ x;
   -- x;
   ++ Min( x, y );

***********************
*Control flow elements*
***********************
See :doc:`control_flow`.

***************
*static_assert*
***************
See :doc:`static_assert`.

******
*halt*
******
See :doc:`halt`.

*******
*Block*
*******

A block consists of sequense of elements in ``{}``.
It may include elements listed above and other blocks.

A block is necessary first of all for variables scope.
A variable defined in a block is visible inside this block and nested blocks.
Variables defined in a block has a lifetime limited to the end of this block.
When control flow reaches the end of the block lifetimes of all variables defined in this block end.

It's possible to define inside a block a variable with the same name as in one of outer blocks.
After that outer variable will no longer be accessible.

.. code-block:: u_spr

   fn Foo()
   {
       var i32 mut x= 0;
       {
            ++x; // Modify value of outer variable
            var f64 mut x= 3.14; // Define a variable with the same name as a variable in outer block. Now the outer "x" variable will not be accessible until the end of the current block.
            x= 0.0; // Modify a variable of this block.
            var i32 mut y= 0;
       }
       --y; // Error - name "y" not found
   }

A block may have a label.
This label may be used in ``break`` operators inside this block.
In such case ``break`` works for this block only if a label is specified in it.
``break`` without a label relates to current loop, but not a block with a label.
``continue`` for a label of a block is not possible, the compiler will produce an error in such case.

.. code-block:: u_spr

   fn Foo(bool cond)
   {
      {
          if( cond )
          {
              break label block_end;
          }
          // some other code
      } label block_end
   }

There are also ``unsafe`` blocks.
See :ref:`unsafe-blocks`.

***************
*with operator*
***************

This operator allows to perform some action with a result of an expression and if necessary extend the lifetime of temporary variables inside this expression.
This operator contains optional reference and mutability modifiers and the name for the expression result.

``with`` is helpfull to use as an alternative to block, inside that a variable is defined and some operations with it are performed, in cases where the lifetime of the variable should be limited.
Also it is helpfull to use ``with`` in template code where it is not clear wheather a result of an expression is a variable or a reference, because ``with`` (unlike ``var`` and ``auto``) allows to create a reference to temporary variable.

Usage examples:

.. code-block:: u_spr

   with( x : Foo() )
   {
       Bar(x);
       return x + 1;
   }

.. code-block:: u_spr

   with( &mut x : s.Get() )
   {
       ++x;
   }
