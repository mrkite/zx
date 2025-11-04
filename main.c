/** @copyright 2025 Sean Kasun */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gmp.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "calculator.h"

#define VERSION "1.1"

static void printHelp() {
  fprintf(stdout, "Calculator usage\n"
          "5 / 2 : integer math, results are truncated\n"
          "5. / 2 : floating point math\n"
          "5 %% 2 : integer modulo\n"
          "5 %% 2.5 : floating point remainder\n"
          "5 ** 2 - exponential\n"
          "sqrt 5 - square root\n"
          "sin 0.5 - sine function\n"
          "cos 0.5 - cosine function\n"
          "tan 0.5 - tangent function\n"
          "floor 1.9 - round down\n"
          "ceil 1.4 - round up\n"
          "round 0.5 - round to nearest\n"
          "0x20 | 7 - bitwise OR\n"
          "61 & 0xf - bitwise AND\n"
          "61 ^ 0x55 - bitwise XOR\n"
          "~0xff - bitwise NOT\n"
          "1 << 4 - bitwise shift left\n"
          "0x10 >> 4 - bitwise shift right\n"
          "help - this help\n"
          "=d - output decimal\n"
          "=h - output hex\n"
          "=o - output octal\n"
          "=b - output binary\n"
          "=u - output result as unicode character\n"
          );
}

struct State {
  int base;
  bool unicode;
  struct Value prev;
};

static void printBase(int base) {
  switch (base) {
    case 16:
      fwrite("0x", 1, 2, stdout);
      break;
    case 2:
      fwrite("0b", 1, 2, stdout);
      break;
    case 8:
      fwrite("0o", 1, 2, stdout);
      break;
  }
}

static void printValue(struct Value val, struct State *state) {
  const char *err = calcError();
  if (err) {
    fprintf(stderr, "error: %s\n", err);
    return;
  }
  if (state->unicode) {
    uint32_t v = 0;
    if (val.isF) {  // truncate floats
      v = mpf_get_ui(val.f);
    } else {
      v = mpz_get_ui(val.z);
    }
    uint8_t utf[5] = {0};
    if (v < 0x80) {
      utf[0] = v;
    } else if (v < 0x800) {
      utf[0] = 0xc0 | (v >> 6);
      utf[1] = 0x80 | (v & 0x3f);
    } else if (v < 0x10000) {
      utf[0] = 0xe0 | (v >> 12);
      utf[1] = 0x80 | ((v >> 6) & 0x3f);
      utf[2] = 0x80 | (v & 0x3f);
    } else {
      utf[0] = 0xf0 | (v >> 18);
      utf[1] = 0x80 | ((v >> 12) & 0x3f);
      utf[2] = 0x80 | ((v >> 6) & 0x3f);
      utf[3] = 0x80 | (v & 0x3f);
    }
    printf("'%s' ", utf);
  }
  if (val.isF) {
    mp_exp_t exp;
    char *s = mpf_get_str(NULL, &exp, state->base, 0, val.f);
    int len = strlen(s);
    char *p = s;
    if (*p == '-') {
      fputc('-', stdout);
      p++;
      len--;
    }
    printBase(state->base);
    if (exp == 0 && len == 0) {
      fputc('0', stdout);
    }
    if (exp - len > 8 || exp - len < -8) {
      fputc(*p++, stdout);
      len--;
      exp--;
      fputc('.', stdout);
      fwrite(p, 1, len, stdout);
      if (exp) {
        fprintf(stdout, "e%ld", exp);
      }
    } else if (exp < 0) {
      fwrite("0.", 1, 2, stdout);
      while (exp++ < 0) {
        fputc('0', stdout);
      }
      fwrite(p, 1, len, stdout);
    } else {
      int out = exp;
      if (out > len) {
        out = len;
      }
      fwrite(p, 1, out, stdout);
      p += exp;
      len -= exp;
      while (len < 0) {
        fputc('0', stdout);
        len++;
      }
      fputc('.', stdout);
      if (len > 0) {
        fwrite(p, 1, len, stdout);
      }
    }
    free(s);
  } else {
    char *s = mpz_get_str(NULL, state->base, val.z);
    int len = strlen(s);
    char *p = s;
    if (*p == '-') {
      fputc('-', stdout);
      p++;
      len--;
    }
    printBase(state->base);
    fwrite(p, 1, len, stdout);
    free(s);
  }
  fputc('\n', stdout);
}

bool handleLine(struct State *state, char *line) {
  // trim spaces and dashes for checking arguments
  char *start = line;
  while (*start && (isspace(*start) || *start == '-')) {
    start++;
  }
  if (*start == '?' || !memcmp(start, "help", 4)) {
    printHelp();
    return true;
  }
  if (*start == '=') {
    state->base = 10;
    state->unicode = false;
    switch (start[1]) {
      case 'b':
        state->base = 2;
        break;
      case 'o':
        state->base = 8;
        break;
      case 'h':
        state->base = 16;
        break;
      case 'u':
        state->unicode = true;
        break;
    }
    return true;
  }
  if (!memcmp(start, "quit", 4) || !memcmp(start, "exit", 4)) {
    return false;
  }
  state->prev = calculate(line, state->prev);
  printValue(state->prev, state);
  return true;
}

int main(int argc, char **argv) {
  struct State state;
  state.unicode = false;
  state.base = 10;
  mpf_init(state.prev.f);
  mpz_init(state.prev.z);
  state.prev.isF = false;
  
  // if we have args, join them together as a single input
  if (argc > 1) {
    int len = 1;  // include eos
    for (int i = 1; i < argc; i++) {
      len += strlen(argv[i]) + 1;  // arg + space
    }
    char *line = calloc(len, 1);
    int pos = 0;
    for (int i = 1; i < argc; i++) {
      int l = strlen(argv[i]);
      memcpy(line + pos, argv[i], l);
      pos += l;
      if (i != argc - 1) {
        line[pos++] = ' ';
      }
    }
    handleLine(&state, line);
    free(line);
  } else {
    const char *prompt = NULL;
    // no args, so keep reading lines from stdin
    if (isatty(fileno(stdin))) {
      // we print a header if we're not piping
      fprintf(stdout, "zx version %s\n© Copyright 2025 Sean Kasun\nType \"quit\" to quit\n", VERSION);
      prompt = ": ";
    }
    char *line = NULL;
    using_history();
    while ((line = readline(prompt)) != NULL) {
      add_history(line);
      if (!handleLine(&state, line)) {
        break;
      }
      free(line);
    }
  }
  return 0;
}
