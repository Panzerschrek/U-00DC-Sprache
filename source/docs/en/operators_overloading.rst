Operators overloading
=====================

For struct and class types it's possible to overload some operators.
Overloaded operator is a function inside a struct or class with special name and with some restrictions.
All overloaded operators should have at least one param with type equal to struct or class where this operator is defined.

An overloaded operator is defined with usage of ``op`` keyword and operator name after it.

.. code-block:: u_spr

   struct Int
   {
       i32 x;
       
       op+( Int& l, Int& r ) : Int // Overloaded binary operator
       {
           var Int i{ .x= l.x + r.x };
           return i;
       }
   
       op-( Int& a ) : Int // Overloaded prefix unary operator
       {
           var Int b{ .x= -a.x };
           return b;
       }
   
       op++( mut this ) // Overloaded unary operation
       {
           ++x;
       }
   
       op[]( this, i32 y ) : i32 // Overloaded postfix operator
       {
           return x * y;
       }
   }
   
   fn Foo()
   {
       var Int mut a{ .x= 42 }, b{ .x= 666 };
       ++a;
       auto ab= a + b;
       auto minus_b= -b;
       var i32 r= a[5];
   }

Overloaded operators:

+----------+------------------+------------------+-------------+
| Operator | kind             | number of params | return type |
+==========+==================+==================+=============+
| ``-``    | prefix operator  | 1                | any         |
+----------+------------------+------------------+-------------+
| ``!``    | prefix operator  | 1                | any         |
+----------+------------------+------------------+-------------+
| ``~``    | prefix operator  | 1                | any         |
+----------+------------------+------------------+-------------+
| ``+``    | binary operator  | 2                | any         |
+----------+------------------+------------------+-------------+
| ``-``    | binary operator  | 2                | any         |
+----------+------------------+------------------+-------------+
| ``*``    | binary operator  | 2                | any         |
+----------+------------------+------------------+-------------+
| ``/``    | binary operator  | 2                | any         |
+----------+------------------+------------------+-------------+
| ``%``    | binary operator  | 2                | any         |
+----------+------------------+------------------+-------------+
| ``&``    | binary operator  | 2                | any         |
+----------+------------------+------------------+-------------+
| ``|``    | binary operator  | 2                | any         |
+----------+------------------+------------------+-------------+
| ``^``    | binary operator  | 2                | any         |
+----------+------------------+------------------+-------------+
| ``>>``   | binary operator  | 2                | any         |
+----------+------------------+------------------+-------------+
| ``<<``   | binary operator  | 2                | any         |
+----------+------------------+------------------+-------------+
| ``==``   | binary operator  | 2                | ``bool``    |
+----------+------------------+------------------+-------------+
| ``<=>``  | binary operator  | 2                | ``i32``     |
+----------+------------------+------------------+-------------+
| ``[]``   | postfix operator | 2                | any         |
+----------+------------------+------------------+-------------+
| ``()``   | postfix operator | 1 or more        | any         |
+----------+------------------+------------------+-------------+
| ``++``   | unary operation  | 1                | ``void``    |
+----------+------------------+------------------+-------------+
| ``--``   | unary operation  | 1                | ``void``    |
+----------+------------------+------------------+-------------+
| ``=``    | binary operation | 2                | ``void``    |
+----------+------------------+------------------+-------------+
| ``+=``   | binary operation | 2                | ``void``    |
+----------+------------------+------------------+-------------+
| ``-=``   | binary operation | 2                | ``void``    |
+----------+------------------+------------------+-------------+
| ``*=``   | binary operation | 2                | ``void``    |
+----------+------------------+------------------+-------------+
| ``/=``   | binary operation | 2                | ``void``    |
+----------+------------------+------------------+-------------+
| ``%=``   | binary operation | 2                | ``void``    |
+----------+------------------+------------------+-------------+
| ``&=``   | binary operation | 2                | ``void``    |
+----------+------------------+------------------+-------------+
| ``|=``   | binary operation | 2                | ``void``    |
+----------+------------------+------------------+-------------+
| ``^=``   | binary operation | 2                | ``void``    |
+----------+------------------+------------------+-------------+
| ``<<=``  | binary operation | 2                | ``void``    |
+----------+------------------+------------------+-------------+
| ``>>=``  | binary operation | 2                | ``void``    |
+----------+------------------+------------------+-------------+

Some overloaded operators are called specially.

For both ``==`` and ``!=`` overloaded operator ``==`` is called.
There is no overloaded ``!=`` operator.
For obtaining of ``!=`` result the result of ``==`` is inverted.

For order compare operators (``<``, ``<=``, ``>``, ``>=``) overloaded operator ``<=>`` is called.
Its result is compared against zero in order to obrain final result.
Overloaded ``<=>`` is not required to return only -1 or +1 for less/more, only the sign of the result is significant.

*******************
*Postfix operators*
*******************

Postifx operators are somewhat special.
All postfix operators should have first parameter of struct or class type where this operator is defined.

Indexation operator ``[]`` allows to access struct or class like this is an array.
Any type may be used as index type.
This allows, for example, to implement associative containers with key access via this operator.

Call operator ``()`` allows to call a struct or class value like a function.
This allows, for example, to implement functional objects.
