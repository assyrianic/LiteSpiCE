#ifndef REALTYPE_H_INCLUDED
#	define REALTYPE_H_INCLUDED

#include <stdbool.h>
#include <inttypes.h>

#define RATIONAL_EXPORT    static inline

#	ifdef TICE_H
#include <ti/real.h>
typedef real_t rat_t;

/** Unary Operations */
RATIONAL_EXPORT rat_t rat_pos1(void) {
	return os_Int24ToReal(1);
}
RATIONAL_EXPORT rat_t rat_neg1(void) {
	return os_Int24ToReal(-1);
}
RATIONAL_EXPORT rat_t rat_zero(void) {
	return os_Int24ToReal(0);
}
RATIONAL_EXPORT rat_t rat_from_int(int const a) {
	return os_Int24ToReal(a);
}
RATIONAL_EXPORT rat_t rat_frac(rat_t const a) {
	return os_RealFrac(&a);
}
RATIONAL_EXPORT rat_t rat_int(rat_t const a) {
	return os_RealInt(&a);
}
RATIONAL_EXPORT rat_t rat_neg(rat_t const a) {
	return os_RealNeg(&a);
}
RATIONAL_EXPORT rat_t rat_ln(rat_t const a) {
	return os_RealLog(&a);
}
RATIONAL_EXPORT rat_t rat_exp(rat_t const a) {
	return os_RealExp(&a);
}
RATIONAL_EXPORT rat_t rat_recip(rat_t const a) {
	return os_RealInv(&a);
}
RATIONAL_EXPORT rat_t rat_floor(rat_t const a) {
	return os_RealFloor(&a);
}
RATIONAL_EXPORT rat_t rat_rad_to_deg(rat_t const a) {
	return os_RealRadToDeg(&a);
}
RATIONAL_EXPORT rat_t rat_deg_to_rad(rat_t const a) {
	return os_RealDegToRad(&a);
}
RATIONAL_EXPORT rat_t rat_sin(rat_t const a) {
	return os_RealSinRad(&a);
}
RATIONAL_EXPORT rat_t rat_cos(rat_t const a) {
	return os_RealCosRad(&a);
}
RATIONAL_EXPORT rat_t rat_tan(rat_t const a) {
	return os_RealTanRad(&a);
}
RATIONAL_EXPORT rat_t rat_asin(rat_t const a) {
	return os_RealAsinRad(&a);
}
RATIONAL_EXPORT rat_t rat_acos(rat_t const a) {
	return os_RealAcosRad(&a);
}
RATIONAL_EXPORT rat_t rat_atan(rat_t const a) {
	return os_RealAtanRad(&a);
}
RATIONAL_EXPORT rat_t rat_pi(void) {
	return rat_acos(rat_neg1());
}

/** Binary Operations */
RATIONAL_EXPORT rat_t rat_add(rat_t const a, rat_t const b) {
	return os_RealAdd(&a, &b);    /// a + b
}
RATIONAL_EXPORT rat_t rat_sub(rat_t const a, rat_t const b) {
	return os_RealSub(&a, &b);    /// a - b
}
RATIONAL_EXPORT rat_t rat_mul(rat_t const a, rat_t const b) {
	return os_RealMul(&a, &b);    /// a * b
}
RATIONAL_EXPORT rat_t rat_div(rat_t const a, rat_t const b) {
	return os_RealDiv(&a, &b);    /// a / b
}
RATIONAL_EXPORT rat_t rat_mod(rat_t const a, rat_t const b) {
	return os_RealMod(&a, &b);    /// a % b
}
RATIONAL_EXPORT rat_t rat_pow(rat_t const a, rat_t const b) {
	return os_RealPow(&a, &b);    /// a^b
}
RATIONAL_EXPORT rat_t rat_root(rat_t const a, rat_t const b) {
	rat_t const r = os_RealInv(&b);
	return os_RealPow(&a, &r);    /// a^(1/b)
}
RATIONAL_EXPORT rat_t rat_min(rat_t const a, rat_t const b) {
	return os_RealMin(&a, &b);    /// a < b? a : b;
}
RATIONAL_EXPORT rat_t rat_max(rat_t const a, rat_t const b) {
	return os_RealMax(&a, &b);    /// a < b? b : a;
}
RATIONAL_EXPORT int rat_cmp(rat_t const a, rat_t const b) {
	return os_RealCompare(&a, &b);
}
RATIONAL_EXPORT rat_t rat_log_base(rat_t const a, rat_t const b) {
	return rat_div(rat_ln(a), rat_ln(b));
}
RATIONAL_EXPORT int rat_lt(rat_t const a, rat_t const b) {
	return rat_cmp(a, b) < 0;
}
RATIONAL_EXPORT rat_t rat_abs(rat_t const a) {
	return rat_lt(a, rat_zero())? rat_neg(a) : a;
}
RATIONAL_EXPORT int rat_ge(rat_t const a, rat_t const b) {
	return rat_cmp(a, b) >= 0;
}

RATIONAL_EXPORT rat_t rat_epsilon(void) {
	rat_t eps = rat_pos1();
	rat_t const one = rat_pos1();
	rat_t const two = rat_from_int(2);
	while( rat_lt(one, rat_add(eps, one)) ) {
		eps = rat_div(eps, two);
	}
	return rat_mul(eps, two);
}

/** Ternary Operations */
RATIONAL_EXPORT rat_t rat_clamp(rat_t const val, rat_t const min, rat_t const max) {
	return rat_max(min, rat_min(val, max));
}
RATIONAL_EXPORT int rat_eq(rat_t const a, rat_t const b, rat_t const eps) {
	return rat_lt(rat_abs(rat_sub(a, b)), eps);
}

RATIONAL_EXPORT int rat_to_str(rat_t const a, size_t const len, char buffer[const static len]) {
	return os_RealToStr(buffer, &a, len, 0, -1) > 0;
}

RATIONAL_EXPORT rat_t str_to_rat(char const cstr[const static 1]) {
	char *end = NULL;
	return os_StrToReal(cstr, &end);
}
#	else
#include <math.h>
typedef double rat_t;
/** Unary Operations */
RATIONAL_EXPORT rat_t rat_pos1(void) {
	return 1.;
}
RATIONAL_EXPORT rat_t rat_neg1(void) {
	return -1.;
}
RATIONAL_EXPORT rat_t rat_zero(void) {
	return 0;
}
RATIONAL_EXPORT rat_t rat_from_int(int const a) {
	return a + 0.0;
}
RATIONAL_EXPORT rat_t rat_frac(rat_t const a) {
	return a - floor(a);
}
RATIONAL_EXPORT rat_t rat_int(rat_t const a) {
	return floor(a);
}
RATIONAL_EXPORT rat_t rat_neg(rat_t const a) {
	return -a;
}
RATIONAL_EXPORT rat_t rat_abs(rat_t const a) {
	return fabs(a);
}
RATIONAL_EXPORT rat_t rat_ln(rat_t const a) {
	return log(a);
}
RATIONAL_EXPORT rat_t rat_exp(rat_t const a) {
	return exp(a);
}
RATIONAL_EXPORT rat_t rat_recip(rat_t const a) {
	return 1.0 / a;
}
RATIONAL_EXPORT rat_t rat_floor(rat_t const a) {
	return floor(a);
}
RATIONAL_EXPORT rat_t rat_rad_to_deg(rat_t const a) {
	return a * (180.0 / acos(-1.0));
}
RATIONAL_EXPORT rat_t rat_deg_to_rad(rat_t const a) {
	return a * (acos(-1.0) / 180.0);
}
RATIONAL_EXPORT rat_t rat_sin(rat_t const a) {
	return sin(a);
}
RATIONAL_EXPORT rat_t rat_cos(rat_t const a) {
	return cos(a);
}
RATIONAL_EXPORT rat_t rat_tan(rat_t const a) {
	return tan(a);
}
RATIONAL_EXPORT rat_t rat_asin(rat_t const a) {
	return asin(a);
}
RATIONAL_EXPORT rat_t rat_acos(rat_t const a) {
	return acos(a);
}
RATIONAL_EXPORT rat_t rat_atan(rat_t const a) {
	return atan(a);
}
RATIONAL_EXPORT rat_t rat_pi(void) {
	return acos(-1.0);
}

/** Binary Operations */
RATIONAL_EXPORT rat_t rat_add(rat_t const a, rat_t const b) {
	return a + b;
}
RATIONAL_EXPORT rat_t rat_sub(rat_t const a, rat_t const b) {
	return a - b;
}
RATIONAL_EXPORT rat_t rat_mul(rat_t const a, rat_t const b) {
	return a * b;
}
RATIONAL_EXPORT rat_t rat_div(rat_t const a, rat_t const b) {
	return a / b;
}
RATIONAL_EXPORT rat_t rat_mod(rat_t const a, rat_t const b) {
	return fmod(a, b);
}
RATIONAL_EXPORT rat_t rat_pow(rat_t const a, rat_t const b) {
	return pow(a,b);
}
RATIONAL_EXPORT rat_t rat_min(rat_t const a, rat_t const b) {
	return a < b? a : b;
}
RATIONAL_EXPORT rat_t rat_max(rat_t const a, rat_t const b) {
	return a < b? b : a;
}
RATIONAL_EXPORT int rat_cmp(rat_t const a, rat_t const b) {
	rat_t const c = a - b;
	return c<0? -1 : c>0? 1 : 0;
}
RATIONAL_EXPORT rat_t rat_log_base(rat_t const a, rat_t const b) {
	return log(a) / log(b);
}
RATIONAL_EXPORT int rat_lt(rat_t const a, rat_t const b) {
	return a < b;
}
RATIONAL_EXPORT int rat_ge(rat_t const a, rat_t const b) {
	return a >= b;
}
RATIONAL_EXPORT rat_t rat_epsilon(void) {
	rat_t eps = 1.0;
	while( eps+1.0 > 1.0 ) {
		eps *= 0.5;
	}
	return eps * 2.0;
}

/** Ternary Operations */
RATIONAL_EXPORT rat_t rat_clamp(rat_t const val, rat_t const min, rat_t const max) {
	return rat_max(min, rat_min(val, max));
}
RATIONAL_EXPORT int rat_eq(rat_t const a, rat_t const b, rat_t const eps) {
	return fabs(a-b) < eps;
}

RATIONAL_EXPORT rat_t rat_root(rat_t const a, rat_t const b) {
	if( rat_eq(b, 2.0, rat_epsilon()) ) {
		return sqrt(a);
	} else if( rat_eq(b, 3.0, rat_epsilon()) ) {
		return cbrt(a);
	}
	return pow(a, 1/b);
}

RATIONAL_EXPORT int rat_to_str(rat_t const a, size_t const len, char buffer[const static len]) {
	return snprintf(buffer, len, "%f", a);
}

RATIONAL_EXPORT rat_t str_to_rat(char const cstr[const static 1]) {
	return strtod(cstr, NULL);
}
#	endif
#endif