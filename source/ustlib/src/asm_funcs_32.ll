;
; external functions from glibc.
;

declare i8* @malloc( i32 )
declare void @free( i8* )
declare i8* @realloc( i8*, i32 )
declare i8* @memcpy( i8*, i8*, i32 )

;
; halt
;

declare void @__U_halt()

;
; stdlib functions for targets with 32-bit pointers.
;

; fn ust::ref_to_int( void& v ) : size_type;
$_ZN3ust10ref_to_intERKv = comdat any
define linkonce_odr i32 @_ZN3ust10ref_to_intERKv( i8* nonnull %ref ) unnamed_addr comdat
{
	%1= ptrtoint i8* %ref to i32
	ret i32 %1
}

; fn ust::int_to_ref( size_type size ) : void&;
$_ZN3ust10int_to_refEj = comdat any
define linkonce_odr i8* @_ZN3ust10int_to_refEj( i32 %int ) unnamed_addr comdat
{
	%1= inttoptr i32 %int to i8*
	ret i8* %1
}

; fn ust::memory_allocate( size_type size_bytes ) : void &mut;
$_ZN3ust15memory_allocateEj = comdat any
define linkonce_odr i8* @_ZN3ust15memory_allocateEj( i32 %size ) unnamed_addr comdat
{
	%1= call i8* @malloc( i32 %size )
	%2= icmp ne i8* %1, null
	br i1 %2, label %3, label %4
;<label>:3
	ret i8* %1
;<label>:4
	call void @__U_halt()
	unreachable
}

; fn ust::memory_reallocate( void& mem, size_type new_size_bytes ) unsafe : void &mut;
$_ZN3ust17memory_reallocateERKvj = comdat any
define linkonce_odr i8* @_ZN3ust17memory_reallocateERKvj( i8* %ptr, i32 %new_size ) unnamed_addr comdat
{
	%1= call i8* @realloc( i8* %ptr, i32 %new_size )
	%2= icmp ne i8* %1, null
	br i1 %2, label %3, label %4
;<label>:3
	ret i8* %1
;<label>:4
	call void @__U_halt()
	unreachable
}

; fn ust::memory_free( void& mem );
$_ZN3ust11memory_freeERKv = comdat any
define linkonce_odr void @_ZN3ust11memory_freeERKv( i8* %ptr ) unnamed_addr comdat
{
	call void @free( i8* %ptr )
	ret void
}

; fn ust::memory_copy( void&mut dst, void & src, size_type size_bytes ) unsafe;
$_ZN3ust11memory_copyERvRKvj = comdat any
define linkonce_odr void @_ZN3ust11memory_copyERvRKvj( i8* %dst, i8* %src, i32 %size ) unnamed_addr comdat
{
	%1= call i8* @memcpy( i8* %dst, i8* %src, i32 %size )
	ret void
}
