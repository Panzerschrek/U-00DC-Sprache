Raw pointers
============

Ü has raw pointer types that are somewhat close to the pointer types in some low-level programming languages like C.
Syntactically raw pointers in Ü are different comparing to C, but the meaning is similar.

Raw pointers are intended for usage in low-level code like containers, foreign language interaction, etc.
There is no reference checking for raw pointers, thus pointer dereference operation is allowed only inside ``unsafe`` code.

Raw pointer type is declared with ``$`` symbol and following element type name in ``()``.
Examples:

.. code-block:: u_spr

   type i32_ptr = $(i32); // pointer to "i32".
   type tup_ptr = $(tup[i32, f32]); // pointer to a tuple.
   type bool_ptr_ptr = $($(bool)); // pointer to pointer to "bool".

A value of raw pointer type may be obtained from a mutable reference via special operator, that consists of ``$<`` and following expression in ``()``.
Backward convertion (from a raw pointer to reference) is performed via dereference operator ``$>``.

.. code-block:: u_spr

   var i32 mut x= 0;
   var (i32) x_ptr = $<(x); // Convert reference to "x" into raw pointer.
   unsafe{  $>(x_ptr) = 24;  } // Dereference pointer value and assign something to this value. This is allowed only in unsafe code.

It's important to mention that raw pointers in Ü have no mutability modifiers (unlike C).
All raw pointers are considered to point to mutable data.
Thus a raw pointer may be obtained only from a mutable reference and dereference operator returns mutable reference.

``zero_init`` initializer is supported for raw pointer types.

********************
*Pointer arithmetic*
********************

It's possible to perform some arithmetic operations with raw pointers.
Following operatoions are possible:

* Pointer and integer number addition.
  It's allowed to perform additon with signed and unsigned integers, but with size no more than pointer size.
  It's possible to add a number to pointer or a pointer to number (the order of operands doesn't matter).
  There is ``+=`` operator for a pointer and a number.
  The result of the addition is a pointer that points to address that is greater than original by value of the integer multiplied by element size in bytes.
* Subtraction of an integer from a pointer.
  It's allowed to subtract signed or unsigned integer from a pointer.
  The size of an integer must be no more than pointer size.
  There is ``-=`` operator for a pointer abd a number.
  The result of the subtraction is a pointer that points to address that is less that original by value of the integer multiplied by element size in bytes.
* Pointers difference.
  It's allowed to calculate pointers difference via ``-`` operator.
  The element type should have non-zero size.
  The result of the pointer difference is a signed integer number with size equal to pointer size that is equal to address difference divided by the element size.
* Increment and decrement.
  It's allowed to use ``++`` and ``--`` for pointers.
  The value of a pointer will be increased or decreased by the element size.
* Pointer comparison.
  All compare operators (``==``, ``!=``, ``<``, ``<=``, ``>``, ``>=``) are allowed for pointers.

Examples:

.. code-block:: u_spr

   var [ i32, 8 ] mut arr= zero_init;
   var $(i32) ptr0= $<(a[0]);

   auto ptr3 = ptr0 + 3; // ptr3 points to arr[3]
   auto ptr4 = 4 + ptr0; // ptr4 points to arr[4]

   auto diff5_2 = $<(a[5]) - $<(a[2]); // diff = 3
   auto diff1_6 = $<(a[1]) - $<(a[6]); // diff = -5

   auto mut ptr = $<(a[2]); // ptr points to arr[2]
   ++ptr; // ptr now points to arr[3]
   ptr+= 3; // ptr now points to arr[6]
   --ptr; // ptr now points to arr[5]
   ptr-= 4; // ptr now points to arr[1]

   auto is_3_less_4 = ptr3 < prt4; // true
   auto is_3_greater_0 = ptr3 > prt0; // true
   auto is_3_equal_4 = ptr3 == prt4; // false
   auto is_4_less_or_equal_4 = ptr4 <= ptr4; // true
