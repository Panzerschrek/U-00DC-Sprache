Standard library
================

Ü has (for now) small but useful standard library, containing helpers, container classes, functionality for system interaction.
This standard library is named *ustlib*.

This document briefly describes contents of the standard library (without details).
if you want to learn more, read its sources yourself.


*********
*Helpers*
*********

* Aligned storage, that is used internally for some containers (aligned_storage.u).
* Allocation/deallocation functions (alloc.u).
* Asserts (assert.u).
* Barrier synchronization primitive (barrier.u).
* Atomic operations support (atomic.u).
* Binary heap routines and heap sorting (binary_heap.u).
* Binary search routines (binary_seach.u).
* Bit math functions (bit_math.u).
* Math functions with overflow checks (checked_math.u).
* Comparators for sorting and binary search (compare.u).
* Helpers for arrays and tuples construction (composite.u).
  Contains functions which construct an array or tuple for given arguments.
* Condition variable synchronization primitive (condition_variable.u).
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
* Ip address and socket address structures (inet_address.u).
* Inet address resolution function declaration (inet_address_resolve.u).
* IO result types definition (io_result.u).
* Functions for decimal integers parsing (integer_parsing.u).
* Iterator class template (iterator.u).
  Supports transformations, filtering and other operations over iterators.
* A helper wrapper macro for executable entry function (main_wrapper.u).
* Math functions (math.u).
* Memory helpers, mostly unsafe (memory.u).
* Minimum/maximum functions (minmax.u).
* Mixins-related helpers (mixin_utils.u).
* Native socket type definition (native_socket.u).
* Filesystem path type definition (path.u).
* Filesystem paths manipulation functions (path_utils.u).
* Functions for polymorph objects manipulation (polymorph.u).
* Reference notation helpers (reference_notation.u).
  Used to simplify specifying reference notation for functions.
* Helper classes and macros for scoped arrays creation (scoped_array.u).
* Semaphore synchronization primitive (semaphore.u).
* Shared version of barrier synchronization primitive (shared_barrier.u).
* Shared version of condition variable synchronization primitive (shared_condition_variable.u).
* Sorting routines (sort.u).
* stdin support (stdin.u).
* stdout support (stdout.u).
* Number to string conversion utilities (string_conversions.u).
* System time class (system_time.u).
* TCP listener class (tcp_listener.u).
* TCP stream class (tcp_stream.u).
* Thread class (thread.u).
* Various helpers for types manipulation/type checks (type_traits.u).
* UDP socket class (udp_socket.u).
* Some useful macros for effective usage of some containers (ustlib_macros.u).
* Functions for conversions between different strings representations (UTF-8, UTF-16, UTF-32) (utf.u).
* Volatile memory operations support (volatile.u).


************
*Containers*
************

* Constainer for storing a value of an (almost) arbitrary type (any.u).
* Atomic variable container, which wraps raw atomic operations (atomic_variable.u).
* Heap-allocated box (box.u) and its nullable version (box_nullabe.u).
  Designed to store a value indirectly, which allows creating recursive data structures.
* Hash map - hash-based key-value container (hash_map.u).
* Hash set (adapter for hash map) (hash_set.u).
* Optional class that optionally stores in-place a value of specified type or no value (optional.u).
* Optional reference, that stores a reference to a value (mutable or immutable) or null (optional_ref.u).
* View for continuous sequence of single type values (mutable or immutable) (random_access_range.u).
* Result container, that stores a value of one of two provided types (result.u).
  Designed to be used as return value type for functions which may fail.
* Shared version of atomic variable container (shared_atomic_variable.u).
* Single-threaded shared pointers (shared_ptr.u).
  Provide interior shared mutability.
* Single-threaded shared pointers for immutable data (shared_ptr_final.u).
  Allow sharing the same piece of constant data across multiple owners.
* Multi-threaded shared pointers (shared_ptr_mt.u).
  Provide interior shared mutability with thread-safety.
* Multi-threaded shared pointers for immutable data (shared_ptr_mt_final.u).
  Allow sharing the same piece of constant data across multiple owners and threads.
* Multi-threaded shared pointers with mutex-based synchronization (shared_ptr_mt_mutex.u).
* Strings (string.u, string_base.u) and helper utilities.
  Strings are like vectors, but support only ``char`` types as elements and allow concatenation via overloaded ``+`` operators.
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
This includes containers like *box*, *box_nullable* *string*, *vector*, *hash_map* and all *shared_ptr* containers.
Also thread class uses heap allocation for its internal state.
So, you need to avoid usage of these functionality in a heapless environment.
