;
; WinAPI functions
;

declare dllimport i8* @GetProcessHeap()
declare dllimport i8* @HeapAlloc( i8*, i32, i64 )
declare dllimport i8* @HeapReAlloc( i8*, i32, i8*, i64 )
declare dllimport i32 @HeapFree( i8*, i32, i8* )

; Store process heap pointer in a global variable.
; Retrieve it exactly once, using approach like with C++ global variables.
; Doing so we ensure that we always use the same heap instance.
; Also we slightly optimize the code by avoiding calling "GetProcessHeap" on each allocation - reading a global variable is a little bit cheaper.
; Initialization function is called by C runtime libraries once (before entering "main" function)
; Since no Ü code should be ever executed before "main", it should be safe.

@g_process_heap = dso_local global i8* zeroinitializer, comdat
$g_process_heap = comdat any

@llvm.global_ctors = appending global [ 1 x { i32, ptr, ptr } ] [ { i32, ptr, ptr } { i32 65535, ptr @init_process_heap_pointer, ptr null } ]

define private void @init_process_heap_pointer()
{
	%heap = call i8* @GetProcessHeap()
	store i8* %heap, i8* @g_process_heap
	ret void
}

; Impl functions

$ust_memory_allocate_impl = comdat any
define linkonce_odr hidden fastcc i8* @ust_memory_allocate_impl( i64 %size ) unnamed_addr comdat
{
	%heap = load i8*, ptr @g_process_heap
	%res = call i8* @HeapAlloc( i8* %heap, i32 0, i64 %size )
	ret i8* %res
}

$ust_memory_reallocate_impl = comdat any
define linkonce_odr hidden fastcc i8* @ust_memory_reallocate_impl( i8* %ptr, i64 %size ) unnamed_addr comdat
{
	%heap = load i8*, ptr @g_process_heap
	%res = call i8* @HeapReAlloc( i8* %heap, i32 0, i8* %ptr, i64 %size )
	ret i8* %res
}

$ust_memory_free_impl = comdat any
define linkonce_odr hidden fastcc void @ust_memory_free_impl( i8* %ptr ) unnamed_addr comdat
{
	%heap = load i8*, ptr @g_process_heap
	%free_res = call i32 @HeapFree( i8* %heap, i32 0, i8* %ptr )
	; For now ignore result of "HeapFree"
	ret void
}

attributes #0 = { argmemonly nounwind }
