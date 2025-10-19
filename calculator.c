/** @copyright 2025 Sean Kasun */
#include "calculator.h"
#include "btree.h"
#include "mpextras.h"
#include <ctype.h>
#include <float.h>
#include <gmp.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum {
  OR, XOR, AND, SHL, SHR, ADD, SUB, MUL, DIV, MOD, NEG, POS, NOT, POW, SQRT, COS, SIN, TAN, FLOOR, CEIL, ROUND,
};
enum {
  Left, Right, Unary,
};

struct Op {
  int prec;
  int assoc;
  int output;
};

struct List {
  const char *token;
  int len;
  struct List *next;
};

struct Tree {
  struct Op *op;
  struct Tree *left;
  struct Tree *right;
  struct Value leaf;
};

struct Reader {
  const char *p;
  const char *end;
};

struct Token {
  const char *start;
  int len;
};

static struct BTreeNode *unaries = NULL, *binaries = NULL;
static struct List *terminators = NULL;
static char *errorMsg;

static void init();
static struct Tree *parse(int prec, struct Reader *reader, struct Value prev);
static struct Tree *primary(struct Reader *reader, struct Value prev);
static struct Token next(struct Reader *reader);
static struct Value eval(struct Tree *tree);
static void consume(struct Reader *reader, struct Token token);
static int findIndexOf(const char *needle, const char *haystack);
static bool expect(struct Reader *reader, char c);
static struct Tree *branch(struct Op *op, struct Tree *left, struct Tree *right);
static struct Tree *leaf(struct Reader *reader, struct Value prev);
static struct Tree *parseChar(struct Reader *reader);
static void freeTree(struct Tree *t);

struct Value calculate(const char *expression, struct Value prev) {
  if (!terminators) {
    init();
  }
  errorMsg = NULL;
  struct Reader reader = {
    expression,
    expression + strlen(expression),
  };
  struct Tree *tree = parse(0, &reader, prev);
  mpf_clear(prev.f);
  mpz_clear(prev.z);
  struct Value v;
  mpf_init(v.f);
  mpz_init(v.z);
  if (tree == NULL) {
    return v;
  }
  if (reader.p != reader.end) {
    freeTree(tree);
    errorMsg = "Expected operator";
    return v;
  }
  prev = eval(tree);
  v.isF = prev.isF;
  if (prev.isF) {
    mpf_set(v.f, prev.f);
  } else {
    mpz_set(v.z, prev.z);
  }

  freeTree(tree);  // will destroy prev
  return v;
}

static uint32_t djb2(const char *str, int len) {
  uint32_t hash = 5381;
  for (int i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + str[i];
  }
  return hash;
}

static void addTerminator(const char *token) {
  struct List *terminator = malloc(sizeof(struct List));
  terminator->token = token;
  terminator->len = strlen(token);
  terminator->next = terminators;
  terminators = terminator;
}

static void add(const char *token, int prec, int assoc, int output) {
  struct Op *op = malloc(sizeof(struct Op));
  op->assoc = assoc;
  op->prec = prec;
  op->output = output;
  uint32_t key = djb2(token, strlen(token));
  if (assoc == Unary) {
    bTreeInsert(&unaries, key, op);
  } else {
    bTreeInsert(&binaries, key, op);
  }
  addTerminator(token);
}

static void init() {
  add("|", 0, Left, OR);
  add("^", 1, Left, XOR);
  add("&", 2, Left, AND);
  add("<<", 3, Left, SHL);
  add(">>", 3, Left, SHR);
  add("+", 4, Left, ADD);
  add("-", 4, Left, SUB);
  add("*", 5, Left, MUL);
  add("/", 5, Left, DIV);
  add("%", 5, Left, MOD);
  add("-", 5, Unary, NEG);
  add("+", 5, Unary, POS);
  add("~", 6, Unary, NOT);
  add("**", 7, Right, POW);
  add("sqrt", 8, Unary, SQRT);
  add("cos", 8, Unary, COS);
  add("sin", 8, Unary, SIN);
  add("tan", 8, Unary, TAN);
  add("floor", 8, Unary, FLOOR);
  add("ceil", 8, Unary, CEIL);
  add("round", 8, Unary, ROUND);
  addTerminator("(");
  addTerminator(")");
  addTerminator("'");
}

static struct Tree *parse(int prec, struct Reader *reader, struct Value prev) {
  struct Tree *t = primary(reader, prev);
  if (t == NULL) {
    return NULL;
  }
  struct Token token = next(reader);
  struct Op *op;
  while ((op = bTreeSearch(binaries, djb2(token.start, token.len))) != NULL && op->prec >= prec) {
    consume(reader, token);
    int subprec = op->prec;
    if (op->assoc == Left) {
      subprec++;
    }
    struct Tree *r = parse(subprec, reader, prev);
    if (r == NULL) {
      freeTree(t);
      return NULL;
    }
    t = branch(op, t, r);
    token = next(reader);
  }
  return t;
}

static struct Tree *primary(struct Reader *reader, struct Value prev) {
  // either starts with a unary or a leaf
  struct Token token = next(reader);
  if (token.len == 0) {
    errorMsg = "Unexpected end";
    return NULL;
  }
  struct Op *op = bTreeSearch(unaries, djb2(token.start, token.len));
  if (op) {
    consume(reader, token);
    struct Tree *t = parse(op->prec, reader, prev);
    if (!t) {
      return NULL;
    }
    return branch(op, t, NULL);
  }
  if (*token.start == '(') {
    consume(reader, token);
    struct Tree *t = parse(0, reader, prev);
    if (!t) {
      return NULL;
    }
    if (!expect(reader, ')')) {
      return NULL;
    }
    return t;
  }
  if (*token.start == '\'') {
    consume(reader, token);
    struct Tree *t = parseChar(reader);
    if (!t) {
      return NULL;
    }
    if (!expect(reader, '\'')) {
      return NULL;
    }
    return t;
  }
  struct Tree *t = leaf(reader, prev);
  if (!t) {
    return NULL;
  }
  return t;
}

static struct Token next(struct Reader *reader) {
  struct Token token;
  // skip whitespace
  while (reader->p < reader->end && isspace(*reader->p)) {
    reader->p++;
  }
  token.start = reader->p;
  token.len = 0;
  // ended?
  if (reader->p >= reader->end) {
    return token;
  }
  int tokenPos = -1;
  for (struct List *t = terminators; t != NULL; t = t->next) {
    int ofs = findIndexOf(t->token, reader->p);
    if (ofs > -1) {
      if (tokenPos == -1 || tokenPos >= ofs) {  // we want the earliest
        if (tokenPos == ofs) {  // we want the longest
          if (t->len > token.len) {
            token.len = t->len;
          }
        } else {
          tokenPos = ofs;
          token.len = t->len;
        }
      }
    }
  }
  if (tokenPos < 0) {  // no terminators found, return remaining string
    token.len = reader->end - reader->p;
  }
  return token;
}

static void consume(struct Reader *reader, struct Token token) {
  reader->p += token.len;
}

static int findIndexOf(const char *needle, const char *haystack) {
  int needleLen = strlen(needle);
  int end = strlen(haystack) - needleLen;
  for (int i = 0; i <= end; i++) {
    bool found = true;
    for (int j = 0; j < needleLen; j++) {
      if (haystack[i + j] != needle[j]) {
        found = false;
        break;
      }
    }
    if (found) {
      return i;
    }
  }
  return -1;
}

static bool expect(struct Reader *reader, char c) {
  static char expected[20];
  if (*reader->p != c) {
    const char *e = "Expected '?'";
    memcpy(expected, e, strlen(e));
    char *sub = strchr(expected, '?');
    if (sub) {
      *sub = c;
    }
    errorMsg = expected;
    return false;
  }
  reader->p++;
  return true;
}

static struct Tree *branch(struct Op *op, struct Tree *left, struct Tree *right) {
  struct Tree *t = calloc(sizeof(struct Tree), 1);
  t->op = op;
  t->left = left;
  t->right = right;
  return t;
}

static struct Tree *leaf(struct Reader *reader, struct Value prev) {
  struct Value v;
  mpz_init(v.z);
  mpf_init(v.f);
  if (*reader->p == '$') {
    reader->p++;
    v.isF = prev.isF;
    mpf_set(v.f, prev.f);
    mpz_set(v.z, prev.z);
  } else if (*reader->p == 'p' && reader->p + 1 < reader->end && reader->p[1] == 'i') {
    reader->p += 2;
    v.isF = true;
    mpf_set_d(v.f, M_PI);
  } else if (*reader->p == '0' && reader->p + 1 < reader->end &&
        (reader->p[1] == 'b' || reader->p[1] == 'o')) {  // binary or octal
    v.isF = false;
    mpz_set_si(v.z, 0);
    reader->p++;
    int base = *reader->p++ == 'b' ? 2 : 8;
    char ch;
    while ((ch = *reader->p) >= '0' && ch < '0' + base) {
      mpz_mul_ui(v.z, v.z, base);
      mpz_add_ui(v.z, v.z, ch - '0');
      reader->p++;
    }
  } else {
    char *end;
    v.isF = true;
    int len;
    if (!gmp_sscanf(reader->p, "%Ff%n", v.f, &len) || len == 0) {
      static char unknown[20];
      const char *e = "Unknown '?'";
      memcpy(unknown, e, strlen(e));
      char *sub = strchr(unknown, '?');
      if (sub) {
        *sub = *reader->p;
      }
      errorMsg = unknown;
      mpz_clear(v.z);
      mpf_clear(v.f);
      return NULL;
    }
    bool forcedFloat = false;
    for (int i = 0; i < len; i++) {
      if (*reader->p++ == '.') {
        forcedFloat = true;
      }
    }
    if (mpf_integer_p(v.f) && !forcedFloat) {
      mpz_set_f(v.z, v.f);
      v.isF = false;
    }
  }
  struct Tree *t = calloc(sizeof(struct Tree), 1);
  t->leaf = v;
  return t;
}

static struct Tree *parseChar(struct Reader *reader) {
  struct Value v;
  v.isF = false;
  mpz_init(v.z);
  mpf_init(v.f);
  if (reader->p < reader->end && *reader->p != '\'') {
    if (*reader->p == '\\') {
      reader->p++;
      if (reader->p >= reader->end) {
        errorMsg = "Unclosed '";
        return NULL;
      }
      switch (*reader->p) {
        case 'a':
          mpz_set_ui(v.z, 7);
          reader->p++;
          break;
        case 'b':
          mpz_set_ui(v.z, 8);
          reader->p++;
          break;
        case 'f':
          mpz_set_ui(v.z, 0xc);
          reader->p++;
          break;
        case 'n':
          mpz_set_ui(v.z, 0xa);
          reader->p++;
          break;
        case 'r':
          mpz_set_ui(v.z, 0xd);
          reader->p++;
          break;
        case 't':
          mpz_set_ui(v.z, 9);
          reader->p++;
          break;
        case 'v':
          mpz_set_ui(v.z, 0xb);
          reader->p++;
          break;
        case '\\':
          mpz_set_ui(v.z, 0x5c);
          reader->p++;
          break;
        case '\'':
          mpz_set_ui(v.z, 0x27);
          reader->p++;
          break;
        case 'x':
        case 'u':
        case 'U':  // all have a bunch of hex follow
          {
            reader->p++;
            while (reader->p < reader->end) {
              char c = *reader->p;
              if (c >= '0' && c <= '9') {
                mpz_mul_ui(v.z, v.z, 16);
                mpz_add_ui(v.z, v.z, c - '0');
              } else if (c >= 'a' && c <= 'f') {
                mpz_mul_ui(v.z, v.z, 16);
                mpz_add_ui(v.z, v.z, c - 'a' + 10);
              } else if (c >= 'A' && c <= 'F') {
                mpz_mul_ui(v.z, v.z, 16);
                mpz_add_ui(v.z, v.z, c - 'A' + 10);
              } else {
                break;
              }
              reader->p++;
            }
          }
          break;
        default:  // might be octal
          while (reader->p < reader->end) {
            char c = *reader->p;
            if (c >= '0' && c <= '7') {
              mpz_mul_ui(v.z, v.z, 8);
              mpz_add_ui(v.z, v.z, c - '0');
              reader->p++;
            } else {
              break;
            }
          }
          break;
      }
    } else {
      uint32_t val = 0;
      // parse utf8
      switch (*reader->p & 0xf0) {
        case 0x00:  // ascii
        case 0x80:  // not utf8
          val = *reader->p++;
          break;
        case 0xc0:  // 2 byte utf8
          val = (*reader->p++ & 0x1f) << 6;
          val |= *reader->p++ & 0x3f;
          break;
        case 0xe0:  // 3 byte utf8
          val = (*reader->p++ & 0x1f) << 12;
          val |= (*reader->p++ & 0x3f) << 6;
          val |= *reader->p++ & 0x3f;
          break;
        case 0xf0:  // 4 byte utf8
          val = (*reader->p++ & 0xf) << 18;
          val |= (*reader->p++ & 0x3f) << 12;
          val |= (*reader->p++ & 0x3f) << 6;
          val |= *reader->p++ & 0x3f;
          break;
      }
      mpz_set_ui(v.z, val);
    }
  }
  struct Tree *t = calloc(sizeof(struct Tree), 1);
  t->leaf = v;
  return t;
}

static void freeTree(struct Tree *t) {
  if (t->left) {
    freeTree(t->left);
  }
  if (t->right) {
    freeTree(t->right);
  }
  if (t->op == NULL) {  // leaf node
    mpz_clear(t->leaf.z);
    mpf_clear(t->leaf.f);
  }
  free(t);
}

static struct Value eval(struct Tree *tree) {
  if (tree->op == NULL) {
    return tree->leaf;
  }
  struct Value l = eval(tree->left);
  struct Value r;
  if (tree->right) {
    r = eval(tree->right);
  }
  // it makes no sense to use most bitwise ops with floats...
  switch (tree->op->output) {
    case OR:
      if (l.isF) {
        mpz_set_f(l.z, l.f);
        l.isF = false;
      }
      if (r.isF) {
        mpz_set_f(r.z, r.f);
      }
      mpz_ior(l.z, l.z, r.z);
      return l;
    case XOR:
      if (l.isF) {
        mpz_set_f(l.z, l.f);
        l.isF = false;
      }
      if (r.isF) {
        mpz_set_f(r.z, r.f);
      }
      mpz_xor(l.z, l.z, r.z);
      return l;
    case AND:
      if (l.isF) {
        mpz_set_f(l.z, l.f);
        l.isF = false;
      }
      if (r.isF) {
        mpz_set_f(r.z, r.f);
      }
      mpz_and(l.z, l.z, r.z);
      return l;
    case SHL:
      if (l.isF) {
        mp_bitcnt_t p;
        if (r.isF) {
          p = mpf_get_si(r.f);
        } else {
          p = mpz_get_si(r.z);
        }
        mpf_mul_2exp(l.f, l.f, p);
      } else {
        mp_bitcnt_t p;
        if (r.isF) {
          p = mpf_get_si(r.f);
        } else {
          p = mpz_get_si(r.z);
        }
        mpz_mul_2exp(l.z, l.z, p);
      }
      return l;
    case SHR:
      if (l.isF) {
        mp_bitcnt_t p;
        if (r.isF) {
          p = mpf_get_si(r.f);
        } else {
          p = mpz_get_si(r.z);
        }
        mpf_div_2exp(l.f, l.f, p);
      } else {
        mp_bitcnt_t p;
        if (r.isF) {
          p = mpf_get_si(r.f);
        } else {
          p = mpz_get_si(r.z);
        }
        mpz_div_2exp(l.z, l.z, p);
      }
      return l;
    case ADD:
      if (l.isF) {
        if (!r.isF) {
          mpf_set_z(r.f, r.z);
        }
        mpf_add(l.f, l.f, r.f);
      } else if (r.isF) {
        mpf_set_z(l.f, l.z);
        l.isF = true;
        mpf_add(l.f, l.f, r.f);
      } else {
        mpz_add(l.z, l.z, r.z);
      }
      return l;
    case SUB:
      if (l.isF) {
        if (!r.isF) {
          mpf_set_z(r.f, r.z);
        }
        mpf_sub(l.f, l.f, r.f);
      } else if (r.isF) {
        mpf_set_z(l.f, l.z);
        l.isF = true;
        mpf_sub(l.f, l.f, r.f);
      } else {
        mpz_sub(l.z, l.z, r.z);
      }
      return l;
    case MUL:
      if (l.isF) {
        if (!r.isF) {
          mpf_set_z(r.f, r.z);
        }
        mpf_mul(l.f, l.f, r.f);
      } else if (r.isF) {
        mpf_set_z(l.f, l.z);
        l.isF = true;
        mpf_mul(l.f, l.f, r.f);
      } else {
        mpz_mul(l.z, l.z, r.z);
      }
      return l;
    case DIV:
      if (l.isF) {
        if (!r.isF) {
          mpf_set_z(r.f, r.z);
        }
        mpf_div(l.f, l.f, r.f);
      } else if (r.isF) {
        mpf_set_z(l.f, l.z);
        l.isF = true;
        mpf_div(l.f, l.f, r.f);
      } else {
        mpz_div(l.z, l.z, r.z);
      }
      return l;
    case MOD:
      if (l.isF) {
        if (!r.isF) {
          mpf_set_z(r.f, r.z);
        }
        mpf_tdiv_r(l.f, l.f, r.f);
      } else if (r.isF) {
        mpf_set_z(l.f, l.z);
        l.isF = true;
        mpf_tdiv_r(l.f, l.f, r.f);
      } else {
        mpz_tdiv_r(l.z, l.z, r.z);
      }
      return l;
    case NEG:
      if (l.isF) {
        mpf_neg(l.f, l.f);
      } else {
        mpz_neg(l.z, l.z);
      }
      return l;
    case POS:  // do nothing
      return l;
    case NOT:
      if (l.isF) {
        mpz_set_f(l.z, l.f);
        l.isF = false;
      }
      mpz_com(l.z, l.z);
      return l;
    case POW:
      if (!l.isF) {
        mpf_set_z(l.f, l.z);
        l.isF = true;
      }
      if (!r.isF) {
        mpf_set_z(r.f, r.z);
      }
      mpf_pow(l.f, l.f, r.f);
      return l;
    case SQRT:
      if (!l.isF) {
        mpf_set_z(l.f, l.z);
        l.isF = true;
      }
      mpf_sqrt(l.f, l.f);
      return l;
    case COS:
      if (!l.isF) {
        mpf_set_z(l.f, l.z);
        l.isF = true;
      }
      mpf_cos(l.f, l.f);
      return l;
    case SIN:
      if (!l.isF) {
        mpf_set_z(l.f, l.z);
        l.isF = true;
      }
      mpf_sin(l.f, l.f);
      return l;
    case TAN:
      if (!l.isF) {
        mpf_set_z(l.f, l.z);
        l.isF = true;
      }
      mpf_tan(l.f, l.f);
      return l;
    case FLOOR:
      if (l.isF) {
        mpf_floor(l.f, l.f);
        mpz_set_f(l.z, l.f);
        l.isF = false;
      }
      return l;
    case CEIL:
      if (l.isF) {
        mpf_ceil(l.f, l.f);
        mpz_set_f(l.z, l.f);
        l.isF = false;
      }
      return l;
    case ROUND:
      if (l.isF) {
        mpf_round(l.z, l.f);
        l.isF = false;
      }
      return l;
  }
  errorMsg = "Unknown operator";
  return l;
}

const char *calcError() {
  return errorMsg;
}
