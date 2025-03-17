Standard library
================

Ü has (for now) small but useful standard library, containing helpers and container classes.
This standard library is named *ustlib*.

This document briefly describes contents of the standard library (without details).
if you want to learn more, read its sources yourself.


*********
*Helpers*
*********

* Aligned storage, that is used internally for some containers (aligned_storage.u).
* Allocation/deallocation functions (alloc.u).
* Asserts (assert.u).
* Atomic operations support (atomic.u).
* Binary search routines (binary_seach.u).
* Math functions with overflow checks (checked_math.u).
* Comparators for sorting and binary search (compare.u).
* Helpers for arrays and tuples construction (composite.u).
  Contains functions which construct an array or tuple for given arguments.
* Various container utils (container_utils.u).
* Coroutines helpers (coro.u).
* Directory iterator (directory_iterator.u).
* Functions for conversion of enums into strings and strings into enums (enum_string_conversions.u).
* File classes (file.u).
* Additional functions for files reading/writing (file_helpers.u).
* File metadata structures (file_metadata.u).
* Filesystem-related functions (filesystem.u).
* Hashing functions (hash.u).
  Hashing of all basic language types is supported.
* Various helpers (helpers.u).
* Functions for decimal integers parsing (integer_parsing.u).
* Iterator class template (iterator.u).
  Supports transformations, filtering and other operations over iterators.
* A helper wrapper macro for executable entry function (main_wrapper.u).
* Math functions (math.u).
* Memory helpers, mostly unsafe (memory.u).
* Minimum/maximum functions (minmax.u).
* Mixins-related helpers (mixin_utils.u).
* Filesystem path type definition (path.u).
* Filesystem paths manipulation functions (path_utils.u).
* Functions for polymorph objects manipulation (polymorph.u).
* Reference notation helpers (reference_notation.u).
  Used to simplify specifying reference notation for functions.
* Helper classes and macros for scoped arrays creation (scoped_array.u).
* Sorting routines (sort.u).
* stdin support (stdin.u).
* stdout support (stdout.u).
* Number to string conversion utilities (string_conversions.u).
* System time class (system_time.u).
* Thread class (thread.u).
* Various helpers for types manipulation/type checks (type_traits.u).
* Some useful macros for effective usage of some containers (ustlib_macros.u).
* Functions for conversions between different strings representations (UTF-8, UTF-16, UTF-32) (utf.u).
* Volatile memory operations support (volatile.u).


************
*Containers*
************

* Heap-allocated box (box.u) and its nullable version (box_nullabe.u).
  Designed to store a value indirectly, which allows creating recursive data structures.
* Optional class that optionally stores in-place a value of specified type or no value (optional.u).
* Optional reference, that stores a reference to a value (mutable or immutable) or null (optional_ref.u).
* View for continuous sequence of single type values (mutable or immutable) (random_access_range.u).
* Result container, that stores a value of one of two provided types (result.u).
  Designed to be used as return value type for functions which may fail.
* Single-threaded shared pointers (shared_ptr.u).
  Provide interior shared mutability.
* Single-threaded shared pointers for immutable data (shared_ptr_final.u).
  Allow sharing the same piece of constant data across multiple owners.
* Multi-threaded shared pointers (shared_ptr_mt.u).
  Provide interior shared mutability with thread-safety.
* Multi-threaded shared pointers for immutable data (shared_ptr_mt_final.u).
  Allow sharing the same piece of constant data across multiple owners and threads.
* Strings (string.u, string_base.u) and helper utilities.
  Strings are like vectors, but support only ``char`` types as elements and allow concatenation via overloaded ``+`` operators.
* Unordered map - hash-based key-value container (unordered_map.u).
* Unordered set (adapter for unordered map) (unordered_set.u)
* Variant container, that can store in-place a value of one of specified types (variant.u).
* Vector - growable sequential container (vector.u).


********************
*Compiler built-ins*
********************

Some *ustlib* functions are implemented by Ü compiler itself.
These functions are implemented in *ll* files in *src* sub-directory.
These *ll* files are compiled during Ü Compiler compilation and embedded in it.
So, it's strictly recommended to use *ustlib* shipped together with Ü compiler, to avoid possible mismatches in these functions.


**************************
*Allocation functionality*
**************************

Some functionality of *ustlib* is allocation-free and can be used in environment without heap.

But some functionality uses heap.
This includes containers like *box*, *box_nullable* *string*, *vector*, *unordered_map* and all *shared_ptr* containers.
Also thread class uses heap allocation for its internal state.
So, you need to avoid usage of these functionality in a heapless environment.
