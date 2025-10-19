/** @copyright 2025 Sean Kasun */

#include "mpextras.h"
#include <gmp.h>
#include <mpfr.h>

// num - floor(num / den) * den
void mpf_tdiv_r(mpf_ptr rem, mpf_srcptr num, mpf_srcptr den) {
  mpf_t tmp;
  mpf_init(tmp);
  mpf_div(tmp, num, den);
  mpf_floor(tmp, tmp);
  mpf_mul(tmp, tmp, den);
  mpf_sub(rem, num, tmp);
  mpf_clear(tmp);
}

void mpf_pow(mpf_ptr result, mpf_srcptr base, mpf_srcptr exp) {
  mpfr_t b, e, r;
  mpfr_init2(r, mpf_get_prec(result));
  mpfr_init_set_f(b, base, MPFR_RNDN);
  mpfr_init_set_f(e, exp, MPFR_RNDN);

  mpfr_pow(r, b, e, MPFR_RNDN);

  mpfr_get_f(result, r, MPFR_RNDN);

  mpfr_clear(b);
  mpfr_clear(e);
  mpfr_clear(r);
}

void mpf_sqrt(mpf_ptr result, mpf_srcptr op) {
  mpfr_t r;
  mpfr_t o;
  mpfr_init2(r, mpf_get_prec(result));
  mpfr_init_set_f(o, op, MPFR_RNDN);

  mpfr_sqrt(r, o, MPFR_RNDN);
  
  mpfr_get_f(result, r, MPFR_RNDN);

  mpfr_clear(o);
  mpfr_clear(r);
}

void mpf_cos(mpf_ptr result, mpf_srcptr op) {
  mpfr_t r;
  mpfr_t o;
  mpfr_init2(r, mpf_get_prec(result));
  mpfr_init_set_f(o, op, MPFR_RNDN);

  mpfr_cos(r, o, MPFR_RNDN);
  
  mpfr_get_f(result, r, MPFR_RNDN);

  mpfr_clear(o);
  mpfr_clear(r);
}

void mpf_sin(mpf_ptr result, mpf_srcptr op) {
  mpfr_t r;
  mpfr_t o;
  mpfr_init2(r, mpf_get_prec(result));
  mpfr_init_set_f(o, op, MPFR_RNDN);

  mpfr_sin(r, o, MPFR_RNDN);
  
  mpfr_get_f(result, r, MPFR_RNDN);

  mpfr_clear(o);
  mpfr_clear(r);
}

void mpf_tan(mpf_ptr result, mpf_srcptr op) {
  mpfr_t r;
  mpfr_t o;
  mpfr_init2(r, mpf_get_prec(result));
  mpfr_init_set_f(o, op, MPFR_RNDN);

  mpfr_tan(r, o, MPFR_RNDN);
  
  mpfr_get_f(result, r, MPFR_RNDN);

  mpfr_clear(o);
  mpfr_clear(r);
}

void mpf_round(mpz_ptr result, mpf_srcptr op) {
  mpfr_t o;
  mpfr_init_set_f(o, op, MPFR_RNDN);

  mpfr_get_z(result, o, MPFR_RNDN);

  mpfr_clear(o);
}
