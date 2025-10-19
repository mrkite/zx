/** @copyright 2025 Sean Kasun */
#pragma once

#include <gmp.h>

extern void mpf_tdiv_r(mpf_ptr rem, mpf_srcptr num, mpf_srcptr den);
extern void mpf_pow(mpf_ptr result, mpf_srcptr base, mpf_srcptr exp);
extern void mpf_sqrt(mpf_ptr result, mpf_srcptr op);
extern void mpf_cos(mpf_ptr result, mpf_srcptr op);
extern void mpf_sin(mpf_ptr result, mpf_srcptr op);
extern void mpf_tan(mpf_ptr result, mpf_srcptr op);
extern void mpf_round(mpz_ptr result, mpf_srcptr op);
