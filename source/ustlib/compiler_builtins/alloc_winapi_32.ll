;
; WinAPI functions
;

declare dllimport x86_stdcallcc i8* @GetProcessHeap()
declare dllimport x86_stdcallcc i8* @HeapAlloc( i8*, i32, i32 )
declare dllimport x86_stdcallcc i8* @HeapReAlloc( i8*, i32, i8*, i32 )
declare dllimport x86_stdcallcc i32 @HeapFree( i8*, i32, i8* )

; Impl functions

$ust_memory_allocate_impl = comdat any
define linkonce_odr hidden i8* @ust_memory_allocate_impl( i32 %size ) unnamed_addr comdat
{
	%heap = call x86_stdcallcc i8* @GetProcessHeap()
	%res = call x86_stdcallcc i8* @HeapAlloc( i8* %heap, i32 0, i32 %size )
	ret i8* %res
}

$ust_memory_reallocate_impl = comdat any
define linkonce_odr hidden i8* @ust_memory_reallocate_impl( i8* %ptr, i32 %size ) unnamed_addr comdat
{
	%heap = call x86_stdcallcc i8* @GetProcessHeap()
	%res = call x86_stdcallcc i8* @HeapReAlloc( i8* %heap, i32 0, i8* %ptr, i32 %size )
	ret i8* %res
}

$ust_memory_free_impl = comdat any
define linkonce_odr hidden void @ust_memory_free_impl( i8* %ptr ) unnamed_addr comdat
{
	%heap = call x86_stdcallcc i8* @GetProcessHeap()
	%free_res = call x86_stdcallcc i32 @HeapFree( i8* %heap, i32 0, i8* %ptr )
	; For now ignore result of "HeapFree"
	ret void
}

attributes #0 = { argmemonly nounwind }
