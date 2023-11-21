### About

This directory contains Ü standard library files.

Containers:

* Heap-allocated box (box.u) and its nullable version (box_nullabe.u)
* Optional class that optionally stores in-place a value of specified type (optional.u)
* Optional reference, that stores a reference to value (mutable or immutable) or null (optional_ref.u)
* View for continuous sequence of single type values (mutable or immutable) (random_access_range.u)
* Result container, that stores a value of one of two provided types (result.u)
* Single-threaded shared pointers (shared_ptr.u)
* Single-threaded shared pointers for immutable data (shared_ptr_final.u)
* Multi-threaded shared pointers (shared_ptr_mt.u)
* Multi-threaded shared pointers for immutable data (shared_ptr_mt_final.u)
* Strings (string.u, string_base.u)
* Unordered map (key-value container) (unordered_map.u)
* Unordered set (adapter for unordered map) (unordered_set.u)
* Variant container, that can store in-place a value of one of specified types (variant.u)
* Vector - growable sequential container (vector.u)

Other functionality:

* Aligned storage, that is used internally for some containers (aligned_storage.u)
* Allocation/deallocation (alloc.u)
* Asserts (assert.u)
* Atomic operations support (atomic.u)
* Math functions with overflow checks (checked_math.u)
* Helpers for arrays and tuples construction (composite.u)
* Various container utils (container_utils.u)
* Coroutines helpers (coro.u)
* Hashing functions (hash.u)
* Various helpers (helpers.u)
* Math functions (math.u)
* Memory helpers (memory.u)
* Minimum/maximum functions (minmax.u)
* Functions for polymorph objects manipulation (polymorph.u)
* Pthread library prototypes (used internally) (pthread.u)
* Reference notation helpers (reference_notation.u)
* Sorting routines (sort.u)
* stdout support (unfinished) (stdout.u)
* Number to string conversion utilities (string_conversions.u)
* Thread class (thread.u)
* Various helpers for types manipulation/type checks (type_traits.u)
* Volatile memory operations support (volatile.u)


### Compiler built-ins

Some ustlib functions are implemented by compiler itself.
These functions are implemented in *ll* files in *src* subdirectory.
These *ll* files are compiled during Ü Compiler compilation and embedded in it.
So, in order to change these implementations, you need to recompile compiler itself.

But it is still possible to change some built-in functionality.
Ü Compiler has an option to disable built-in allocation functionality. Thus you can use this option and provide your own allocation/deallocation functions implementations.

List of built-in functionality:

* Allocation functionality (alloc_32.ll, alloc_64.ll, alloc_dummy.ll)
* Atomic operations (atomic.ll)
* Math functions with overflow checks (checked_math.ll)
* Coroutines helpers (coro.ll)
* Halt handlers (halt_abort.ll, halt_configurable.ll, halt_trap.ll, halt_unreachable.ll)
* Math functions (math.ll)
* Memory helpers (memory_32.ll, memory_64.ll)
* Stdout print implementations (stdout ll files)

Some built-ins are implemented via C standard library functions, like malloc/free, stdout print or some math functions.
Thus you need to link against C standard library in order to use them.


### Allocation functionality

Some functionality of ustlib is allocation-free and can be used in environment without a heap.

But some functionality uses heap.
This includes containers like *box*, *box_nullable* *string*, *vector*, *unordered_map* and all *shared_ptr* containers.
Also thread class uses heap allocation for its internal state.
So, you need to avoid usage of these functionality in heapless environment.

Note that compiler itself may use heap allocations - for coroutines.
So, you can't use coroutines if you can't use heap.
