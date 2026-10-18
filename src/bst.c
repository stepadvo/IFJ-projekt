/**
 * @file src_example.h
 * @brief example header
 * @author Name Surname, xlogin@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include "bst.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX(A, B) A > B ? A : B

/**
 * @brief structure for the tree nodes
 */
typedef struct bstNode {
    char *key; // ptr to the keyArr array
    size_t data;
    struct bstNode *left;
    struct bstNode *right;
    char height;
} BSTNode;

struct BSearchTree {
    BSTNode *root;
    char *keyArr; // an array for all the keys to be stored in seperated by null
                  // characters
    size_t keyArrSize;     // index of the first unused byte
    size_t keyArrCapacity; // first index out of bounds
};

bSearchTree *BVSinit() {
    bSearchTree *temp = malloc(sizeof(bSearchTree));
    if (!temp) {
        // TODO set errcode
        return NULL;
    }

    // init the key array with capacity of 4
    char *tempKeyArr = malloc(4);
    if (!tempKeyArr) {
        // TODO set errcode
        free(temp);
        return NULL;
    }
    temp->keyArr = tempKeyArr;
    temp->keyArrSize = 0;
    temp->keyArrCapacity = 4;
    return temp;
}

/**
 * @brief destructs the node and all daugter nodes
 */
void BSTDestructNode(BSTNode *node) {
    if (!node) {
        return;
    }

    BSTDestructNode(node->right);
    BSTDestructNode(node->left);

    free(node);
}

void BSTDestruct(bSearchTree *tree) {
    BSTDestructNode(tree->root);

    // free the key array
    if (tree->keyArr) {
        free(tree->keyArr);
    }

    free(tree);
}

/**
 * @brief inserts a key to the
 */
char *BSTInsertKey(bSearchTree *tree, const char *key) {
    // repeat until the arr capacity is sufficient
    while (tree->keyArrSize + strlen(key) < tree->keyArrCapacity) {
        char *temp = realloc(tree->keyArr, tree->keyArrCapacity * 2);
        if (!temp) {
            // TODO set errcode
            return NULL;
        }

        tree->keyArr = temp;
        tree->keyArrCapacity *= 2;
    }

    // append the key to the end of the array
    char *ptr = strcpy(tree->keyArr + tree->keyArrSize, key);
    tree->keyArrSize += strlen(key) + 1;
    return ptr;
}

BSTNode *createBSTNode(bSearchTree *tree, const char *key, size_t data) {
    BSTNode *temp = malloc(sizeof(BSTNode));
    if (!temp) {
        // TODO set errcode
        return NULL;
    }

    temp->data = data;
    temp->key = BSTInsertKey(tree, key);
    temp->left = temp->right = NULL;
    temp->height = 1;
    return temp;
}

/**
 * @brief get the height of subtree with node as root
 */
static inline char getHeight(BSTNode *node) {
    if (!node) {
        return 0;
    }
    return node->height;
}

/**
 * @brief get the balance of the specified node
 */
static inline char getBalance(BSTNode *node) {
    if (!node) {
        return 0;
    }
    return getHeight(node->right) - getHeight(node->left);
}

BSTNode *rotateRight(BSTNode *node) {
    // TODO
    return node;
}

BSTNode *rotateLeft(BSTNode *node) {
    // TODO
    return node;
}

/**
 * @brief checks if the tree is balanced, if not corrects it
 * @return pointer to the node if balanced, pointer to the node, that should
 * replace the root node in the tree if not balanced
 *
 * checks the balance at node
 * if the balance is not within <-1,1> corrects it by rotations
 */
BSTNode *BSTBalanceTree(BSTNode *node) {
    char balance = getBalance(node);

    if (balance < -1) {
        if (getBalance(node->left) <= 0) {
            // LL situation
            return rotateRight(node);
        } else {
            // LR situation
            node->left = rotateLeft(node->left);
            return rotateRight(node);
        }
    }
    if (balance > 1) {
        if (getBalance(node->right) >= 0) {
            // RR situation
            return rotateLeft(node);
        } else {
            // RL situation
            node->right = rotateRight(node->left);
            return rotateLeft(node);
        }
    }
    return node;
}

BSTNode *BSTInsertNode(BSTNode *root, BSTNode *newNode) {
    if (!root) {
        return newNode;
    }
    if (newNode->key < root->key) {
        root->left = BSTInsertNode(root->left, newNode);
    } else if (newNode->key < root->key) {
        root->right = BSTInsertNode(root->right, newNode);
    } else {
        // duplicite data now allowed
        return root;
    }

    // update the height ()
    root->height = 1 + MAX(getHeight(root->right), getHeight(root->left));

    // check and correct the balance
    return BSTBalanceTree(root);
}

int BSTInsert(bSearchTree *tree, const char *key, size_t data) {
    BSTNode *newNode = createBSTNode(tree, key, data);
    if (!newNode) {
        return -1;
    }

    tree->root = BSTInsertNode(tree->root, newNode);
    return 0;
}

size_t BSTRemove(bSearchTree *tree, const char *key) {
    (void)tree;
    (void)key;
    // TODO
    return 1;
}

size_t *BSTSearch(bSearchTree *tree, const char *key) {
    BSTNode *node = tree->root;
    while (true) {
        // check if the node is not NULL
        if (!node) {
            return NULL;
        }

        int result = strcmp(key, node->key);
        if (result == 0) {
            return &node->data;
        }
        if (result < 0) {
            node = node->left;
        }
        if (result > 0) {
            node = node->right;
        }
    }
}

/*
 * end src_example.h
 */
