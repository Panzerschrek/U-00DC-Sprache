### About

This directory contains Ü standard library files.

Containers:

* Constainer for storing a value of an (almost) arbitrary type (any.iu)
* Arena-allocated array container (arena_allocated_array.iu)
* Arena-allocated box (arena_allocated_box.iu) and its nullable version (arena_allocated_box_nullable.iu)
* Atomic variable container, which wraps raw atomic operations (atomic_variable.iu)
* Heap-allocated box (box.iu) and its nullable version (box_nullabe.iu)
* Hash map (key-value container) (hash_map.iu)
* Hash set (adapter for hash map) (hash_set.iu)
* Optional class that optionally stores in-place a value of specified type (optional.iu)
* Optional reference, that stores a reference to value (mutable or immutable) or null (optional_ref.iu)
* View for continuous sequence of single type values (mutable or immutable) (random_access_range.iu)
* Result container, that stores a value of one of two provided types (result.iu)
* Shared version of atomic variable container (shared_atomic_variable.iu)
* Single-threaded shared pointers (shared_ptr.iu)
* Single-threaded shared pointers for immutable data (shared_ptr_final.iu)
* Multi-threaded shared pointers (shared_ptr_mt.iu)
* Multi-threaded shared pointers for immutable data (shared_ptr_mt_final.iu)
* Multi-threaded shared pointers with mutex-based synchronization (shared_ptr_mt_mutex.iu)
* Strings (string.u, string_base.iu)
* Variant container, that can store in-place a value of one of specified types (variant.iu)
* Vector - growable sequential container (vector.iu)

Other functionality:

* Aligned storage, that is used internally for some containers (aligned_storage.iu)
* Allocation/deallocation (alloc.iu)
* Arena allocator class (arena_allocator.iu)
* Asserts (assert.iu)
* Atomic operations support (atomic.iu)
* Barrier synchronization primitive (barrier.iu)
* Binary heap routines and heap sorting (binary_heap.iu)
* Binary search routines (binary_seach.iu)
* Bit math functions (bit_math.iu)
* Math functions with overflow checks (checked_math.iu)
* Comparators for sorting and binary search (compare.iu)
* Helpers for arrays and tuples construction (composite.iu)
* Condition variable synchronization primitive (condition_variable.iu)
* Various container utils (container_utils.iu)
* Coroutines helpers (coro.iu)
* Directory iterator (directory_iterator.iu)
* Duration class (duration.iu)
* Functions for conversion of enums into strings and strings into enums (enum_string_conversions.iu)
* File classes (file.iu)
* Additional functions for files reading/writing (file_helpers.iu)
* File metadata structures (file_metadata.iu)
* Filesystem-related functions (filesystem.iu)
* Hashing functions (hash.iu)
* Various helpers (helpers.iu)
* Ip address and socket address structures (inet_address.iu)
* Inet address resolution function declaration (inet_address_resolve.iu)
* IO result types definition (io_result.iu)
* Functions for decimal integers parsing (integer_parsing.iu)
* Iterator class template (iterator.iu)
* A helper wrapper macro for executable entry function (main_wrapper.iu)
* Math functions (math.iu)
* Memory helpers (memory.iu)
* Minimum/maximum functions (minmax.iu)
* Mixins-related helpers (mixin_utils.iu)
* Native socket type definition (native_socket.iu)
* Filesystem path type definition (path.iu)
* Filesystem paths manipulation functions (path_utils.iu)
* Functions for polymorph objects manipulation (polymorph.iu)
* Reference notation helpers (reference_notation.iu)
* Helper classes and macros for scoped arrays creation (scoped_array.iu)
* Semaphore synchronization primitive (semaphore.iu)
* Shared version of barrier synchronization primitive (shared_barrier.iu)
* Shared version of condition variable synchronization primitive (shared_condition_variable.iu)
* Sorting routines (sort.iu)
* stdin support (stdin.iu)
* stdout support (stdout.iu)
* Number to string conversion utilities (string_conversions.iu)
* System time class (system_time.iu)
* TCP listener class (tcp_listener.iu)
* TCP stream class (tcp_stream.iu)
* Thread class (thread.iu)
* Various helpers for types manipulation/type checks (type_traits.iu)
* UDP socket class (udp_socket.iu)
* Some useful macros for effective usage of some containers (ustlib_macros.iu)
* Functions for conversions between different strings representations (UTF-8, UTF-16, UTF-32) (utf.iu)
* Volatile memory operations support (volatile.iu)


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
* Atomic operations (atomic.ll, atomic_32.ll, atomic_64.ll)
* Math functions with overflow checks (checked_math.ll)
* Coroutines helpers (coro.ll)
* Halt handlers (halt_abort.ll, halt_configurable.ll, halt_trap.ll, halt_unreachable.ll)
* Math functions (math.ll)
* Memory helpers (memory_32.ll, memory_64.ll)
* Volatile operations (volatile.ll)

Some built-ins are implemented via C standard library functions, like malloc/free, stdout print or some math functions.
Thus you need to link against C standard library in order to use them.


### Allocation functionality

Some functionality of ustlib is allocation-free and can be used in environment without a heap.

But some functionality uses heap.
This includes containers like *box*, *box_nullable* *string*, *vector*, *hash_map* and all *shared_ptr* containers.
Thread class and all synchronization primitive classes use heap too.
Also functions, which interact with operationg system (filesystem functions, environment variable functions, stdin/stdout, etc.) may allocate.
So, you need to avoid usage of these functionality in heapless environment.

Note that compiler itself may use heap allocations - for coroutines.
So, you can't use coroutines if you can't use heap.
