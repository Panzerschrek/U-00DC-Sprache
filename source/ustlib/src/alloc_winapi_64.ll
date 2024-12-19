;
; WinAPI functions
;

declare i8* @GetProcessHeap()
declare i8* @HeapAlloc( i8*, i32, i64 )
declare i8* @HeapReAlloc( i8*, i32, i8*, i64 )
declare void @HeapFree( i8*, i32, i8* )

; Impl functions

$ust_memory_allocate_impl = comdat any
define linkonce_odr hidden i8* @ust_memory_allocate_impl( i64 %size ) unnamed_addr comdat
{
	%heap = call i8* @GetProcessHeap()
	%res = call i8* @HeapAlloc( i8* %heap, i32 0, i64 %size )
	ret i8* %res
}

$ust_memory_reallocate_impl = comdat any
define linkonce_odr hidden i8* @ust_memory_reallocate_impl( i8* %ptr, i64 %size ) unnamed_addr comdat
{
	%heap = call i8* @GetProcessHeap()
	%res = call i8* @HeapReAlloc( i8* %heap, i32 0, i8* %ptr, i64 %size )
	ret i8* %res
}

$ust_memory_free_impl = comdat any
define linkonce_odr hidden void @ust_memory_free_impl( i8* %ptr ) unnamed_addr comdat
{
	%heap = call i8* @GetProcessHeap()
	call void @HeapFree( i8* %heap, i32 0, i8* %ptr )
	ret void
}

attributes #0 = { argmemonly nounwind }
