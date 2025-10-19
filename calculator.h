/** @copyright 2025 Sean Kasun */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <gmp.h>

struct Value {
  bool isF;
  mpz_t z;
  mpf_t f;
};

extern struct Value calculate(const char *expression, struct Value prev);
extern const char *calcError();
