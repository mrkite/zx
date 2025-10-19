/** @copyright 2025 Sean Kasun */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#define BTREE_ORDER 4

struct BTreeNode {
  uint32_t numKeys;
  uint32_t keys[BTREE_ORDER - 1];
  void *data[BTREE_ORDER - 1];
  struct BTreeNode *children[BTREE_ORDER];
  bool leaf;
};

extern void bTreeInsert(struct BTreeNode **root, uint32_t key, void *data);
void *bTreeSearch(struct BTreeNode *root, uint32_t key);
