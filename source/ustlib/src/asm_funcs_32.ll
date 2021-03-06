%__U_void = type {}

;
; external functions from glibc.
;

declare %__U_void* @malloc( i32 )
declare void @free( %__U_void* )
declare %__U_void* @realloc( %__U_void*, i32 )
declare %__U_void* @memcpy( %__U_void*, %__U_void*, i32 )
declare i32 @memcmp( %__U_void*, %__U_void*, i32 )

;
; halt
;

declare void @__U_halt()

;
; stdlib functions for targets with 32-bit pointers.
;

; fn ust::ref_to_int( void& v ) : size_type;
$_ZN3ust10ref_to_intERKv = comdat any
define linkonce_odr i32 @_ZN3ust10ref_to_intERKv( %__U_void* nonnull %ref ) unnamed_addr comdat
{
	%1= ptrtoint %__U_void* %ref to i32
	ret i32 %1
}

; fn ust::int_to_ref( size_type size ) : void&;
$_ZN3ust10int_to_refEj = comdat any
define linkonce_odr %__U_void* @_ZN3ust10int_to_refEj( i32 %int ) unnamed_addr comdat
{
	%1= inttoptr i32 %int to %__U_void*
	ret %__U_void* %1
}

; fn ust::memory_allocate( size_type size_bytes ) : void &mut;
$_ZN3ust15memory_allocateEj = comdat any
define linkonce_odr %__U_void* @_ZN3ust15memory_allocateEj( i32 %size ) unnamed_addr comdat
{
	%1= call %__U_void* @malloc( i32 %size )
	%2= icmp ne %__U_void* %1, null
	br i1 %2, label %3, label %4
;<label>:3
	ret %__U_void* %1
;<label>:4
	call void @__U_halt()
	unreachable
}

; fn ust::memory_reallocate( void& mem, size_type new_size_bytes ) unsafe : void &mut;
$_ZN3ust17memory_reallocateERKvj = comdat any
define linkonce_odr %__U_void* @_ZN3ust17memory_reallocateERKvj( %__U_void* %ptr, i32 %new_size ) unnamed_addr comdat
{
	%1= call %__U_void* @realloc( %__U_void* %ptr, i32 %new_size )
	%2= icmp ne %__U_void* %1, null
	br i1 %2, label %3, label %4
;<label>:3
	ret %__U_void* %1
;<label>:4
	call void @__U_halt()
	unreachable
}

; fn ust::memory_free( void& mem );
$_ZN3ust11memory_freeERKv = comdat any
define linkonce_odr void @_ZN3ust11memory_freeERKv( %__U_void* %ptr ) unnamed_addr comdat
{
	call void @free( %__U_void* %ptr )
	ret void
}

; fn ust::memory_copy( void&mut dst, void & src, size_type size_bytes ) unsafe;
$_ZN3ust11memory_copyERvRKvj = comdat any
define linkonce_odr void @_ZN3ust11memory_copyERvRKvj( %__U_void* %dst, %__U_void* %src, i32 %size ) unnamed_addr comdat
{
	%1= call %__U_void* @memcpy( %__U_void* %dst, %__U_void* %src, i32 %size )
	ret void
}

; fn memory_equals( void& a, void& b, size_type size ) unsafe : bool;
$_ZN3ust13memory_equalsERKvS1_j = comdat any
define linkonce_odr i1 @_ZN3ust13memory_equalsERKvS1_j( %__U_void* %a, %__U_void* %b, i32 %size ) unnamed_addr comdat
{
	%1= call i32 @memcmp( %__U_void* %a, %__U_void* %b, i32 %size )
	%2= icmp eq i32 %1, 0
	ret i1 %2
}
