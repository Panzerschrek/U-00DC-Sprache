Unsafe code requirements
========================

There are some rules that should be followed in unsafe code.
Their violation may lead to unforeseen consequences.


Uninitialized data
~~~~~~~~~~~~~~~~~~

With ``uninitialized_initializer`` or via external allocation functions it's possible to obtain uninitialized (garbage) data.
Reading of uninitialized data is an undefined behavior.
Before reading a value should be written first in order to avoid undefined behavior.


Reference checking
~~~~~~~~~~~~~~~~~~

It's not allowed to violate reference checking rules via unsafe code.

It's not allowed to create references that outlive referenced data.
Otherwise destroyed data access may be possible which is an undefined behavior.

It's forbidden to make reference pollution that is not mentioned by reference-pollution notation.
Otherwise it's possible to create a reference to a data that is later changed via other reference and thus invalidate the first mentioned reference.


Non-null references
~~~~~~~~~~~~~~~~~~~

All references must be non-null.
This includes reference-params, return references, local references, references inside structs and other possible references.

The compiler frontend marks references as non-null and the compiler optimizer uses this notation for optimization.
Presense of null references may cause uncorrect code behavior.

Where it is needed raw pointers should be used instead of null references.
Raw pointers may be null.


Unsafe reference type casting
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It's possible with ``cast_ref_unsafe`` operator to convert a reference to one type into a reference to any other types.
But result reference should be used with caution.

It's assumed that via a reference to a specific type values of this type or its parts (struct fields, array elements, etc.) may be read.
The compiler frontend marks references with an attribute that allows to read via this reference a number of bytes equal to the type size.
Because of that the compiler backend may insert speculative memory reads within this size without causing reads of inacessible memory.
But if via a reference obtained by casting of some source reference less memory is accessible, a memory read from potentially inacessible memory address may happen.

Also the compiler frontend marks specially read/write instructions with a type through which this memory operation is performed.
The compiler optimizer assumes that different types can't exist in the same memory area and tries to optimiza memory operation based on it.

There are some exceptions from he rule described above:
* It's assumed that ``byte8`` values may oocupy the same memory area as any other values.
* It's assumed that fundamental type values, references, pointers, enums may oocupy the same memory area as a ``byte``-type values with the same size.
* It's assumed that ``byte16`` values may oocupy the same memory area as ``byte8`` values, ``byte32`` values may oocupy the same memory area as ``byte16`` values, etc.

From the above it follows that it's safe to convert references to any types into references to ``byte8`` or into ``byte``-types with an alignment no more than source type alignment.
It's also allowed to perform backward convertions (from ``byte``-type references into references to other types).

But, for example, a reference convertion from ``f64`` to ``byte8`` and than from ``byte8`` to ``u64`` is not allowed.

Safe are only data reads/writes via a ``byte8`` reference which is obtained from any other ``byteN`` reference, which is obtained from a data with alignment no less than N.
Also safe is to read/write data obtained initially as ``byte8`` (from an memory allocation function, for example) via any other references.


Immutability
~~~~~~~~~~~~

It's forbidden in Ü to mutate values marked as immutable, this includes global and local variables, function arguments and values accessible via immutable references.
It's not allowed to change scalar values, struct or class fields, elements of arrays and tuples.

But is't allowed to mutate data accessible indirectly via immutable data.
It's allowed, for example, to mutate data accessible via a mutable reference inside immutable struct.
It's allowed to change data accessible via standard library containers if these data are stored indirectly - via a reference or a pointer, as in `shared_ptr`-containers, for example.

The compiler frontent marks immutable reference function params specially in order to allow the compiler optimizer to assume that these values are not changed.
But if some unsafe code changes something, this code may work incorrectly.

Correct immutability is also necessary for proper multithreaded functionality in order to guarantee that data, a mutable reference to which is passed into more than one thread are not really changed - synch change may break synchronization.


Thread-unsafe mutability
~~~~~~~~~~~~~~~~~~~~~~~~

Ü allows logical mutability via immutable reference.
Examples of classes which implement this are `shared_ptr`-types of the standard library, which allow to mutate indirectly-stored (via an internal pointer) data via a mutable reference to a `shared_ptr`-type object.

But such mutability requires special considerations.
Types which implement it should either guarantee that it works correcty in multithread environment (in order to prevent data races) or such types should be marked with ``non_sync`` tag.

Containers that somehow indirect store values of some types should have ``non_sync`` tag depening on these types in order to propagate ``non_sync`` property through a container.

Types which implement synchronous mutability (like `shared_ptr_mt`-types of the standard library) should prevent storing of ``non_sync`` types inside, because their presense makes mutability thread-unsafe.

A code that somehow creates thread or passes data into another thread should ensure that passed types are not ``non_sync``.


Internal representation
~~~~~~~~~~~~~~~~~~~~~~~

It's forbidden in Ü with unsafe/external code to create impossible by the rules of the language values of some types.

Only ``true`` and ``false`` values of ``bool`` type are allowed.

Enum values except specified in the enum definition are not allowed.
For example in ``enum E{ A, B, C }`` only binary values 0 (``A``), 1 (``B``), 2(``C``) are possible.

It's not allowed to change virtual function table pointers in polymorph class values.
