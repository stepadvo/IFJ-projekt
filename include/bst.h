/**
 * @file bst.h
 * @brief header for the height balanced binary search tree
 * @author Štěpán Dvořák, xdvoras00@stud.fit.vutbr.cz
 */

#ifndef BST_H
#define BST_H

#include <stdlib.h>

/**
 * @brief a binary search tree implemented as an AVL tree
 * the data within the binary tree is represented by the data type of size_t
 */
typedef struct Bst_s bst_t;

/**
 * @brief initializes an empty tree
 * @return ptr to the tree to be initialized or NULL on memory allocation error
 */
bst_t *BSTInit(void);

/**
 * @brief destroys the tree data structure
 * @param tree the tree to destroy
 *
 * when tree is NULL, does nothing
 */
void BSTDestruct(bst_t *tree);

/**
 * @brief removes all nodes from the tree
 */
void BSTClear(bst_t *tree);

/**
 * @brief inserts a new node into the tree
 * @param data of the new node
 * @return  0 on successfull insert
 *          -1 on memory allocation error
 *          -2 when the key is duplicate
 *
 * if the key is duplicite, there will be no change to the tree
 */
int BSTInsert(bst_t *tree, const char *key, size_t data, char **keyPtr);

/**
 * @brief removes the element with the matching key from the tree
 * @param oldData will be set to the data of the removed node when a node with
 * matching key is found and the ptr is not NULL
 * @return  0 on successfull removal
 *          -1 on memory allocation error
 *          -2 when the key is not found
 */
int BSTRemove(bst_t *tree, const char *key, size_t *oldData);

/**
 * @brief searches the binary tree for the node with the matching key
 * @return pointer to the data or NULL when the key is not found
 */
size_t *BSTSearch(bst_t *tree, const char *key);

/**
 * @brief returns the number of nodes in the tree
 */
size_t BSTSize(bst_t *tree);

/**
 * @brief returns a non 0 value when the tree is balanced, 0 when it is not
 */
int BSTIsBalanced(bst_t *tree);

#endif

/*
 * end bst.h
 */
