/** @copyright 2025 Sean Kasun */
#include <stdlib.h>
#include "btree.h"

static struct BTreeNode *newNode(bool leaf) {
  struct BTreeNode *node = malloc(sizeof(struct BTreeNode));
  node->numKeys = 0;
  node->leaf = leaf;
  for (int i = 0; i < BTREE_ORDER; i++) {
    node->children[i] = NULL;
  }
  return node;
}

static void splitChild(struct BTreeNode *parent, int index) {
  struct BTreeNode *child = parent->children[index];
  struct BTreeNode *node = newNode(child->leaf);

  int split = BTREE_ORDER / 2;
  node->numKeys = split - 1;
  // move keys and children to new node
  for (int i = 0; i < split - 1; i++) {
    node->keys[i] = child->keys[i + split];
    node->data[i] = child->data[i + split];
  }
  if (!child->leaf) {
    for (int i = 0; i < split; i++) {
      node->children[i] = child->children[i + split];
    }
  }
  child->numKeys = split - 1;
  // shift parent's children to make space
  for (int i = parent->numKeys; i > index; i--) {
    parent->children[i + 1] = parent->children[i];
  }
  parent->children[index + 1] = node;
  // shift parent's keys to make space
  for (int i = parent->numKeys - 1; i >= index; i--) {
    parent->keys[i + 1] = parent->keys[i];
    parent->data[i + 1] = parent->data[i];
  }
  parent->keys[index] = child->keys[split - 1];
  parent->data[index] = child->data[split - 1];
  parent->numKeys++;
}

static void insert(struct BTreeNode *node, uint32_t key, void *data) {
  int i = node->numKeys - 1;
  if (node->leaf) {
    while (i >= 0 && node->keys[i] > key) {
      node->keys[i + 1] = node->keys[i];
      node->data[i + 1] = node->data[i];
      i--;
    }
    node->keys[i + 1] = key;
    node->data[i + 1] = data;
    node->numKeys++;
  } else {
    while (i >= 0 && node->keys[i] > key) {
      i--;
    }
    i++;
    if (node->children[i]->numKeys == BTREE_ORDER - 1) {
      splitChild(node, i);
      if (node->keys[i] < key) {
        i++;
      }
    }
    insert(node->children[i], key, data);
  }
}

void bTreeInsert(struct BTreeNode **root, uint32_t key, void *data) {
  if (*root == NULL) {
    struct BTreeNode *node = newNode(true);
    node->keys[0] = key;
    node->data[0] = data;
    node->numKeys = 1;
    *root = node;
  } else {
    if ((*root)->numKeys == BTREE_ORDER - 1) {
      struct BTreeNode *node = newNode(false);
      node->children[0] = *root;
      splitChild(node, 0);
      int i = 0;
      if (node->keys[0] < key) {
        i++;
      }
      insert(node->children[i], key, data);
      *root = node;
    } else {
      insert(*root, key, data);
    }
  }
}

void *bTreeSearch(struct BTreeNode *root, uint32_t key) {
  if (root == NULL) {
    return NULL;
  }
  int i = 0;
  while (i < root->numKeys && key > root->keys[i]) {
    i++;
  }
  if (i < root->numKeys && key == root->keys[i]) {
    return root->data[i];
  }
  if (root->leaf) {
    return NULL;
  }
  return bTreeSearch(root->children[i], key);
}
