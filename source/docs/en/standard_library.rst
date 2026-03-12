Standard library
================

Ü has (for now) small but useful standard library, containing helpers, container classes, functionality for system interaction.
This standard library is named *ustlib*.

This document briefly describes contents of the standard library (without details).
if you want to learn more, read its sources yourself.


*********
*Helpers*
*********

* Aligned storage, that is used internally for some containers (aligned_storage.iu).
* Allocation/deallocation functions (alloc.iu).
* Arena allocator class (arena_allocator.iu).
* Asserts (assert.iu).
* Barrier synchronization primitive (barrier.iu).
* Atomic operations support (atomic.iu).
* Binary heap routines and heap sorting (binary_heap.iu).
* Binary search routines (binary_seach.iu).
* Bit math functions (bit_math.iu).
* Math functions with overflow checks (checked_math.iu).
* Comparators for sorting and binary search (compare.iu).
* Helpers for arrays and tuples construction (composite.iu).
  Contains functions which construct an array or tuple for given arguments.
* Condition variable synchronization primitive (condition_variable.iu).
* Various container utils (container_utils.iu).
* Coroutines helpers (coro.iu).
* Directory iterator (directory_iterator.iu).
* Duration class (duration.iu).
* Functions for conversion of enums into strings and strings into enums (enum_string_conversions.iu).
* File classes (file.iu).
* Additional functions for files reading/writing (file_helpers.iu).
* File metadata structures (file_metadata.iu).
* Filesystem-related functions (filesystem.iu).
* Hashing functions (hash.iu).
  Hashing of all basic language types is supported.
* Various helpers (helpers.iu).
* Ip address and socket address structures (inet_address.iu).
* Inet address resolution function declaration (inet_address_resolve.iu).
* IO result types definition (io_result.iu).
* Functions for decimal integers parsing (integer_parsing.iu).
* Iterator class template (iterator.iu).
  Supports transformations, filtering and other operations over iterators.
* A helper wrapper macro for executable entry function (main_wrapper.iu).
* Math functions (math.iu).
* Memory helpers, mostly unsafe (memory.iu).
* Minimum/maximum functions (minmax.iu).
* Mixins-related helpers (mixin_utils.iu).
* Native socket type definition (native_socket.iu).
* Filesystem path type definition (path.iu).
* Filesystem paths manipulation functions (path_utils.iu).
* Functions for polymorph objects manipulation (polymorph.iu).
* Reference notation helpers (reference_notation.iu).
  Used to simplify specifying reference notation for functions.
* Helper classes and macros for scoped arrays creation (scoped_array.iu).
* Semaphore synchronization primitive (semaphore.iu).
* Shared version of barrier synchronization primitive (shared_barrier.iu).
* Shared version of condition variable synchronization primitive (shared_condition_variable.iu).
* Sorting routines (sort.iu).
* stdin support (stdin.iu).
* stdout support (stdout.iu).
* Number to string conversion utilities (string_conversions.iu).
* System time class (system_time.iu).
* TCP listener class (tcp_listener.iu).
* TCP stream class (tcp_stream.iu).
* Thread class (thread.iu).
* Various helpers for types manipulation/type checks (type_traits.iu).
* UDP socket class (udp_socket.iu).
* Some useful macros for effective usage of some containers (ustlib_macros.iu).
* Functions for conversions between different strings representations (UTF-8, UTF-16, UTF-32) (utf.iu).
* Volatile memory operations support (volatile.iu).


************
*Containers*
************

* Constainer for storing a value of an (almost) arbitrary type (any.iu).
* Arena-allocated array container (arena_allocated_array.iu).
* Arena-allocated box (arena_allocated_box.iu) and its nullable version (arena_allocated_box_nullable.iu).
* Atomic variable container, which wraps raw atomic operations (atomic_variable.iu).
* Heap-allocated box (box.iu) and its nullable version (box_nullabe.iu).
  Designed to store a value indirectly, which allows creating recursive data structures.
* Hash map - hash-based key-value container (hash_map.iu).
* Hash set (adapter for hash map) (hash_set.iu).
* Optional class that optionally stores in-place a value of specified type or no value (optional.iu).
* Optional reference, that stores a reference to a value (mutable or immutable) or null (optional_ref.iu).
* View for continuous sequence of single type values (mutable or immutable) (random_access_range.iu).
* Result container, that stores a value of one of two provided types (result.iu).
  Designed to be used as return value type for functions which may fail.
* Shared version of atomic variable container (shared_atomic_variable.iu).
* Single-threaded shared pointers (shared_ptr.iu).
  Provide interior shared mutability.
* Single-threaded shared pointers for immutable data (shared_ptr_final.iu).
  Allow sharing the same piece of constant data across multiple owners.
* Multi-threaded shared pointers (shared_ptr_mt.iu).
  Provide interior shared mutability with thread-safety.
* Multi-threaded shared pointers for immutable data (shared_ptr_mt_final.iu).
  Allow sharing the same piece of constant data across multiple owners and threads.
* Multi-threaded shared pointers with mutex-based synchronization (shared_ptr_mt_mutex.iu).
* Strings (string.u, string_base.iu) and helper utilities.
  Strings are like vectors, but support only ``char`` types as elements and allow concatenation via overloaded ``+`` operators.
* Variant container, that can store in-place a value of one of specified types (variant.iu).
* Vector - growable sequential container (vector.iu).


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
This includes containers like *box*, *box_nullable* *string*, *vector*, *hash_map* and all *shared_ptr* containers.
Also thread class uses heap allocation for its internal state.
So, you need to avoid usage of these functionality in a heapless environment.
