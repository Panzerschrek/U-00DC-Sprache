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
* Binary search routines (binary_seach.u)
* Math functions with overflow checks (checked_math.u)
* Comparators for sorting and binary search (compare.u)
* Helpers for arrays and tuples construction (composite.u)
* Various container utils (container_utils.u)
* Coroutines helpers (coro.u)
* Directory iterator (directory_iterator.u)
* Functions for conversion of enums into strings and strings into enums (enum_string_conversions.u)
* File classes (file.u)
* Additional functions for files reading/writing (file_helpers.u)
* File metadata structures (file_metadata.u)
* Filesystem-related functions (filesystem.u)
* Hashing functions (hash.u)
* Various helpers (helpers.u)
* Functions for decimal integers parsing (integer_parsing.u)
* Iterator class template (iterator.u)
* A helper wrapper macro for executable entry function (main_wrapper.u)
* Math functions (math.u)
* Memory helpers (memory.u)
* Minimum/maximum functions (minmax.u)
* Mixins-related helpers (mixin_utils.u)
* Filesystem path type definition (path.u)
* Filesystem paths manipulation functions (path_utils.u)
* Functions for polymorph objects manipulation (polymorph.u)
* Reference notation helpers (reference_notation.u)
* Helper classes and macros for scoped arrays creation (scoped_array.u)
* Sorting routines (sort.u)
* stdin support (stdin.u)
* stdout support (stdout.u)
* Number to string conversion utilities (string_conversions.u)
* System time class (system_time.u)
* Thread class (thread.u)
* Various helpers for types manipulation/type checks (type_traits.u)
* Some useful macros for effective usage of some containers (ustlib_macros.u)
* Functions for conversions between different strings representations (UTF-8, UTF-16, UTF-32) (utf.u)
* Volatile memory operations support (volatile.u)


### Components requiring building

Some ustlib functionality is implemented externally - in files within *src* directory.
Normally the build system builds necessary files from this directory during projects building.
In case of manual compiler invocation they should be built (somehow) too, if this functionality is used.
Some files are platform-dependent - they are located in platform-dependent subdirectories.


### Compiler built-ins

Some ustlib functions are implemented by compiler itself.
These functions are implemented in *ll* files in *compiler_builtins* subdirectory.
These *ll* files are compiled during Ü Compiler compilation and embedded in it.
So, in order to change these implementations, you need to recompile compiler itself.

But it is still possible to change some built-in functionality.
Ü Compiler has an option to disable built-in allocation functionality.
Thus you can use this option and provide your own allocation/deallocation functions implementations.

List of built-in functionality:

* Allocation functionality (alloc_libc_32.ll, alloc_libc_64.ll, alloc_winapi_32.ll, alloc_winapi_64.ll, alloc_dummy.ll)
* Atomic operations (atomic.ll)
* Math functions with overflow checks (checked_math.ll)
* Coroutines helpers (coro.ll)
* Halt handlers (halt_abort.ll, halt_configurable.ll, halt_trap.ll, halt_unreachable.ll)
* Math functions (math.ll)
* Memory helpers (memory_32.ll, memory_64.ll)

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
