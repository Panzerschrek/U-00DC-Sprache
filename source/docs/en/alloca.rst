alloca
======

There is an operator in Ü for allocation of a memory block with lifetime bounded to scope block, where this operator is declared.
This operator is named ``alloca`` and it allows to allocate a memory block with size determined in runtime.

.. code-block:: u_spr

   fn Foo( size_type s )
   {
       alloca i32 arr[ s ]; // A memory block for "s" elements of "i32" type will be allocated
   } // Here this memory block will be freed

The result of  the ``alloca`` operator usage is a declared variable of raw pointer type, containing address of the allocated memory block.
This memory block is not initialized by ``alloca`` operator - constructors aren't called, destructors aren't called too.
A programmer should itself call somehow constructors/destructors after ``alloca`` declaration.

It's not recommended to use this operator directly, instead one should use some helper macro (like one from the standard library), that performs proper initialization and destruction of objects in allocated memory block.
Moreover, ``alloca`` operator is designed for usage together with such helper macros.


``alloca`` operator allocates memory from stack, but only if memory block size is less than some limit.
Otherwise it allocates memory from heap.
In both cases the compiler frees allocated memory when it's needed.
Heap allocation for large size blocks is necessary in order to avoid possible stack overflow, when an allocated block is bigger than stack area size or if total size of several allocated blocks is bigger than this size.

Since heap allocation by an ``alloca`` operator may happen, it's impossible to use it in environments without heap (where memory allocation/deallocation functions aren't implemented).
