non_sync Tag
============

Most of the types in Ü should obey nested mutability rule.
This means, that with an immutable reference to some value it's not allowed to mutate nested values - fields, containers contents, etc.
Such property is useful in a multithreaded environment.
Immutable references of such types are possible to pass into a different thread without any synchronization problems.

But there are cases where interior mutability is required.
For shared pointers, for example, it's necessary to modify usage counters.
Such types are not safe to use in a multithreaded environment.
In order to prevent usage of such types in a multithreaded environment Ü has ``non_sync`` tag.

Such tag may be defined in struct or class definition.
There are two ways to do this - unconditionally or with condition in ``()`` after ``non_sync`` tag.

It's possible to check for ``non_sync`` tag existence via ``non_sync`` expression.
``non_sync`` tag is recursively propagated.
If at least one field in a struct or class is of ``non_sync`` type, all struct or class is also ``non_sync``.
``non_sync`` are also arrays and tuples which have ``non_sync`` elements.

.. code-block:: u_spr

   class A non_sync {} // Unconditional non_sync
   struct B non_sync( true ) {} // non_sync by the condition
   class C non_sync( false ) {} // is not non_sync by the condition
   class D{ A a; } // non_sync because it contains non_sync elements
   class E polymorph non_sync {}
   class F : E { } // non_sync because it has a non_sync parent
   
   // Fundamental types are not non_sync
   static_assert( !non_sync</i32/> );
   static_assert( !non_sync</bool/> );
   static_assert( !non_sync</[f32, 4]/> );
   
   static_assert( non_sync</A/> );
   static_assert( non_sync</[A, 8]/> ); // An array of non_sync elements is non_sync
   static_assert( non_sync</B/> );
   static_assert( non_sync</tup[bool, B, char8]/> ); // A tuple with at least one non_sync element is non_sync
   static_assert( !non_sync</C/> );
   static_assert( !non_sync</[C, 7]/> );
   static_assert( non_sync</D/> );
   static_assert( non_sync</E/> );
   static_assert( non_sync</F/> );

It's possible to create dependency loop via ``non_sync`` tag definition.
But it's not an error until an expression inside a ``non_sync`` tag is ``non_sync`` expression.

.. code-block:: u_spr

   class A non_sync( non_sync</A/> ) {} // self-dependency - result is not non_sync
   static_assert( !non_sync</A/> );
   
   class B non_sync( non_sync</C/> ) {} // loop dependency - result is not non_sync, because there is no initial non_sync tag source
   class C non_sync( non_sync</B/> ) {}
   static_assert( !non_sync</B/> );
   static_assert( !non_sync</C/> );
   
   class D non_sync( non_sync</E/> ) {} // loop dependency via a field - result is non_sync, because a field is an initial non_sync tag source
   class E non_sync { D d; }
   static_assert( non_sync</D/> );
   static_assert( non_sync</E/> );

It's not allowed to change ``non_sync`` property in inheritance.
If a ``non_sync`` class has non-``non_sync`` parent, the compiler will produce an error.
It's important in order to avoid missing ``non_sync`` property when storing a value of derived class via a reference or inside a container for base class.

********************
*non_sync tag usage*
********************

There is almost no need to use ``non_sync`` tag in regular code, because it's not possible to make something ``non_sync`` without usage of ``unsafe`` code.

It's necessary to use ``non_sync`` tag only in containers which via ``unsafe`` code implement some thread-unsafe interior mutability.
Also containers with indirect values storage (box, vector, variant, etc) should use conditional ``non_sync`` tag - depending on the contained type in order to propagate ``non_sync`` tag through these containers.

In a code that somehow creates threads or in a code where an object is transferred into another thread it's necessary to add ``static_assert( !non_sync</T/> )`` in order to prevent usage of unsafe for multithreaded environment types.

*******************************
*Safe multithreaded mutability*
*******************************

As it was mentioned above, values of ``non_sync`` types can't be used in a multithreaded environment.
But what if some sort of interior mutability is needed in combination with multithreading?
Solution - use types, that implement thread-safe interior mutability.
Such types are not marked as ``non_sync``.
Internally they use some synchronization primitives in order to prevent race conditions.
Such primitives are mutex, rw_lock, atomics, etc.

Container types with interior mutability synchronization should check contained type for absence of thread-unsafe interior mutability - via ``static_assert( !non_sync</T/> )``.
