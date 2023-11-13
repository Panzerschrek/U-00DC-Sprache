Halt
====

``halt`` is a special operator for abnormal program termination.
It should be used in places where further program execution isn't possible.
``halt`` is terminal, which means, that no other operators and code constructions are possible after it.
For control flow analysis inside the compiler it is treated much like ``return``.

.. code-block:: u_spr

   fn Div5( u32 x ) : u32
   {
       if( x == 0u )
       {
           halt;
       }
       else
       {
           return x / 5u;
       }
       // Ok, function terminates in all control flow branches - either via return other via halt.
   }

*********
*halt if*
*********

There is also a conditional form of ``halt`` operator - ``halt if``.
Abnormal program termination happens if the condition is true.
``halt if`` is unlike ``halt`` not terminal.

.. code-block:: u_spr

   fn Div5( u32 x ) : u32
   {
       halt if( x == 0u );
       return x / 5u;
   }
