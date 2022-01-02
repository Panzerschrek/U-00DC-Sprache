;
; llvm intrinsics
;

declare float  @llvm.sqrt.f32(float  %x)
declare double @llvm.sqrt.f64(double %x)
declare float  @llvm.pow.f32(float  %x, float  %exp)
declare double @llvm.pow.f64(double %x, double %exp)
declare float  @llvm.exp.f32(float  %x)
declare double @llvm.exp.f64(double %x)
declare float  @llvm.exp2.f32(float  %x)
declare double @llvm.exp2.f64(double %x)
declare float  @llvm.log.f32(float  %x)
declare double @llvm.log.f64(double %x)
declare float  @llvm.log2.f32(float  %x)
declare double @llvm.log2.f64(double %x)
declare float  @llvm.sin.f32(float  %x)
declare double @llvm.sin.f64(double %x)
declare float  @llvm.cos.f32(float  %x)
declare double @llvm.cos.f64(double %x)

declare float  @llvm.floor.f32(float  %x)
declare double @llvm.floor.f64(double %x)
declare float  @llvm.ceil.f32(float  %x)
declare double @llvm.ceil.f64(double %x)
declare float  @llvm.round.f32(float  %x)
declare double @llvm.round.f64(double %x)
declare float  @llvm.trunc.f32(float  %x)
declare double @llvm.trunc.f64(double %x)

;
; C99 math.h functions
;

declare float  @expm1f(float  %x)
declare double @expm1 (double %x)
declare float  @log1pf(float  %x)
declare double @log1p (double %x)
declare float  @tanf(float  %x)
declare double @tan (double %x)
declare float  @asinf(float  %x)
declare double @asin (double %x)
declare float  @acosf(float  %x)
declare double @acos (double %x)
declare float  @atanf(float  %x)
declare double @atan (double %x)
declare float  @atan2f(float  %y, float  %x)
declare double @atan2 (double %y, double %x)

;
; common math
;

;fn sqrt( f32 x ) : f32;
$_ZN3ust4sqrtEf = comdat any
define linkonce_odr float @_ZN3ust4sqrtEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.sqrt.f32( float %x )
	ret float %1
}

;fn sqrt( f64 x ) : f64;
$_ZN3ust4sqrtEd = comdat any
define linkonce_odr double @_ZN3ust4sqrtEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.sqrt.f64( double %x )
	ret double %1
}

;fn pow( f32 x, f32 exp ) : f32;
$_ZN3ust3powEff = comdat any
define linkonce_odr float @_ZN3ust3powEff( float %x, float %exp ) unnamed_addr comdat
{
	%1= call float @llvm.pow.f32( float %x, float %exp )
	ret float %1
}

;fn pow( f64 x, f64 exp ) : f32;
$_ZN3ust3powEdd = comdat any
define linkonce_odr double @_ZN3ust3powEdd( double %x, double %exp ) unnamed_addr comdat
{
	%1= call double @llvm.pow.f64( double %x, double %exp )
	ret double %1
}

;fn exp( f32 x ) : f32;
$_ZN3ust3expEf = comdat any
define linkonce_odr float @_ZN3ust3expEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.exp.f32( float %x )
	ret float %1
}

;fn exp( f64 x ) : f64;
$_ZN3ust3expEd = comdat any
define linkonce_odr double @_ZN3ust3expEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.exp.f64( double %x )
	ret double %1
}

;fn exp2( f32 x ) : f32;
$_ZN3ust4exp2Ef = comdat any
define linkonce_odr float @_ZN3ust4exp2Ef( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.exp2.f32( float %x )
	ret float %1
}

;fn exp2( f64 x ) : f64;
$_ZN3ust4exp2Ed = comdat any
define linkonce_odr double @_ZN3ust4exp2Ed( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.exp2.f64( double %x )
	ret double %1
}

;fn expm1( f32 x ) : f32;
$_ZN3ust5expm1Ef = comdat any
define linkonce_odr float @_ZN3ust5expm1Ef( float %x ) unnamed_addr comdat
{
	%1= call float @expm1f( float %x )
	ret float %1
}

;fn expm1( f64 x ) : f64;
$_ZN3ust5expm1Ed = comdat any
define linkonce_odr double @_ZN3ust5expm1Ed( double %x ) unnamed_addr comdat
{
	%1= call double @expm1 ( double %x )
	ret double %1
}

;fn log( f32 x ) : f32;
$_ZN3ust3logEf = comdat any
define linkonce_odr float @_ZN3ust3logEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.log.f32( float %x )
	ret float %1
}

;fn log( f64 x ) : f64;
$_ZN3ust3logEd = comdat any
define linkonce_odr double @_ZN3ust3logEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.log.f64( double %x )
	ret double %1
}

;fn log2( f32 x ) : f32;
$_ZN3ust4log2Ef = comdat any
define linkonce_odr float @_ZN3ust4log2Ef( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.log2.f32( float %x )
	ret float %1
}

;fn log2( f64 x ) : f64;
$_ZN3ust4log2Ed = comdat any
define linkonce_odr double @_ZN3ust4log2Ed( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.log2.f64( double %x )
	ret double %1
}

;fn log1p( f32 x ) : f32;
$_ZN3ust5log1pEf = comdat any
define linkonce_odr float @_ZN3ust5log1pEf( float %x ) unnamed_addr comdat
{
	%1= call float @log1pf( float %x )
	ret float %1
}

;fn log1p( f64 x ) : f64;
$_ZN3ust5log1pEd = comdat any
define linkonce_odr double @_ZN3ust5log1pEd( double %x ) unnamed_addr comdat
{
	%1= call double @log1p( double %x )
	ret double %1
}

;fn sin( f32 x ) : f32;
$_ZN3ust3sinEf = comdat any
define linkonce_odr float @_ZN3ust3sinEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.sin.f32( float %x )
	ret float %1
}

;fn sin( f64 x ) : f64;
$_ZN3ust3sinEd = comdat any
define linkonce_odr double @_ZN3ust3sinEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.sin.f64( double %x )
	ret double %1
}

;fn cos( f32 x ) : f32;
$_ZN3ust3cosEf = comdat any
define linkonce_odr float @_ZN3ust3cosEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.cos.f32( float %x )
	ret float %1
}

;fn cos( f64 x ) : f64;
$_ZN3ust3cosEd = comdat any
define linkonce_odr double @_ZN3ust3cosEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.cos.f64( double %x )
	ret double %1
}

;fn tan( f32 x ) : f32;
$_ZN3ust3tanEf = comdat any
define linkonce_odr float @_ZN3ust3tanEf( float %x ) unnamed_addr comdat
{
	%1= call float @tanf( float %x )
	ret float %1
}

;fn tan( f64 x ) : f64;
$_ZN3ust3tanEd = comdat any
define linkonce_odr double @_ZN3ust3tanEd( double %x ) unnamed_addr comdat
{
	%1= call double @tan( double %x )
	ret double %1
}

;fn asin( f32 x ) : f32;
$_ZN3ust4asinEf = comdat any
define linkonce_odr float @_ZN3ust4asinEf( float %x ) unnamed_addr comdat
{
	%1= call float @asinf( float %x )
	ret float %1
}

;fn asin( f64 x ) : f64;
$_ZN3ust4asinEd = comdat any
define linkonce_odr double @_ZN3ust4asinEd( double %x ) unnamed_addr comdat
{
	%1= call double @asin( double %x )
	ret double %1
}

;fn acos( f32 x ) : f32;
$_ZN3ust4acosEf = comdat any
define linkonce_odr float @_ZN3ust4acosEf( float %x ) unnamed_addr comdat
{
	%1= call float @acosf( float %x )
	ret float %1
}

;fn acos( f64 x ) : f64;
$_ZN3ust4acosEd = comdat any
define linkonce_odr double @_ZN3ust4acosEd( double %x ) unnamed_addr comdat
{
	%1= call double @acos( double %x )
	ret double %1
}

;fn atan( f32 x ) : f32;
$_ZN3ust4atanEf = comdat any
define linkonce_odr float @_ZN3ust4atanEf( float %x ) unnamed_addr comdat
{
	%1= call float @atanf( float %x )
	ret float %1
}

;fn atan( f64 x ) : f64;
$_ZN3ust4atanEd = comdat any
define linkonce_odr double @_ZN3ust4atanEd( double %x ) unnamed_addr comdat
{
	%1= call double @atan( double %x )
	ret double %1
}

;fn atan2( f32 y, f32 x ) : f32;
$_ZN3ust5atan2Eff = comdat any
define linkonce_odr float @_ZN3ust5atan2Eff( float %y, float %x ) unnamed_addr comdat
{
	%1= call float @atan2f( float %y, float %x )
	ret float %1
}

;fn atan2( f64 y, f64 x ) : f32;
$_ZN3ust5atan2Edd = comdat any
define linkonce_odr double @_ZN3ust5atan2Edd( double %y, double %x ) unnamed_addr comdat
{
	%1= call double @atan2( double %y, double %x )
	ret double %1
}

;fn floor( f32 x ) : f32;
$_ZN3ust5floorEf = comdat any
define linkonce_odr float @_ZN3ust5floorEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.floor.f32( float %x )
	ret float %1
}

;fn floor( f64 x ) : f64;
$_ZN3ust5floorEd = comdat any
define linkonce_odr double @_ZN3ust5floorEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.floor.f64( double %x )
	ret double %1
}

;fn ceil( f32 x ) : f32;
$_ZN3ust4ceilEf = comdat any
define linkonce_odr float @_ZN3ust4ceilEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.ceil.f32( float %x )
	ret float %1
}

;fn ceil( f64 x ) : f64;
$_ZN3ust4ceilEd = comdat any
define linkonce_odr double @_ZN3ust4ceilEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.ceil.f64( double %x )
	ret double %1
}

;fn round( f32 x ) : f32;
$_ZN3ust5roundEf = comdat any
define linkonce_odr float @_ZN3ust5roundEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.round.f32( float %x )
	ret float %1
}

;fn round( f64 x ) : f64;
$_ZN3ust5roundEd = comdat any
define linkonce_odr double @_ZN3ust5roundEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.round.f64( double %x )
	ret double %1
}

;fn trunc( f32 x ) : f32;
$_ZN3ust5truncEf = comdat any
define linkonce_odr float @_ZN3ust5truncEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.trunc.f32( float %x )
	ret float %1
}

;fn trunc( f64 x ) : f64;
$_ZN3ust5truncEd = comdat any
define linkonce_odr double @_ZN3ust5truncEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.trunc.f64( double %x )
	ret double %1
}
