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

/**
 * @brief structure for the tree nodes
 */
typedef struct bstNode {
    char *key; // ptr to the keyArr array
    size_t data;
    struct bstNode *left;
    struct bstNode *right;
    char weight;
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
        //TODO set errcode
        return NULL;
    }
    // start with the initial key array capacity of 4
    char *tempKeyArr = malloc(4);
    if (!tempKeyArr) {
        //TODO set errcode
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
    if (tree->keyArr) {
        free(tree->keyArr);
    }
    free(tree);
}

/**
 * @brief inserts a key to the
 */
char *BSTInsertKey(bSearchTree *tree, const char *key) {
    // check if the capacity is sufficient
    while (tree->keyArrSize + strlen(key) < tree->keyArrCapacity) {
        // repeat until the arr capacity is sufficient
        char *temp = realloc(tree->keyArr, tree->keyArrCapacity * 2);
        if (!temp) {
            //TODO set errcode
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

int BSTInsert(bSearchTree *tree, const char *key, size_t data) {
    BSTNode *temp = malloc(sizeof(BSTNode));
    if (!temp) {
        //TODO set errcode
        return -1;
    }
    temp->data = data;
    temp->key = BSTInsertKey(tree, key);
    // TODO insert the node
    return -1;
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
