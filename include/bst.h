#ifndef BST_H
#define BST_H

#include <stdlib.h>

/**
 * @brief a binary search tree implemented as an AVL tree
 * the data within the binary tree is represented by the data type of size_t
 */
typedef struct BSearchTree bSearchTree;

/**
 * @brief initializes an empty tree
 * @return pointer to the tree
 */
bSearchTree *BVSinit();

/**
 * @brief destroys the tree data structure
 * @param tree the tree to destroy
 */
void BSTdestruct(bSearchTree *tree);

/**
 * @brief inserts a new node into the tree
 * @param tree the tree to insert into
 * @param key the key of the new node
 * @param data of the new node
 * @return 0 on success, -1 on failure
 *
 * if the key is duplicite, there will be no change to the tree
 */
int BSTInsert(bSearchTree *tree, const char *key, size_t data);

/**
 * @brief removes the element with the matching key from the tree
 * @return the data of the removed element
 */
size_t BSTRemove(bSearchTree *tree, const char *key);

/**
 * @brief searches the binary tree for the node with the matching key
 * @param tree the BST to search
 * @param key string
 * @return pointer to the data
 */
size_t *BSTSearch(bSearchTree *tree, const char *key);

#endif
