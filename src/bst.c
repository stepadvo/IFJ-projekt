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
 * @brief inserts a key to the key array of the tree
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

/**
 * @brief constructs a new leaf node on the heap
 * @return ptr to the node or NULL on memory allocation error
 */
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

/**
 * @brief rotates the subtree with root node to the right
 *
 * rotates according to the following diagram
 *    A          B
 *   / \        / \
 *  B  T3 ->   T1 A
 * / \        / \
 *T1 T2      T2 T3
 */
BSTNode *rotateRight(BSTNode *node) {
    BSTNode *lChild = node->left;
    BSTNode *lrChild = lChild->right;

    node->left = lrChild;
    lChild->right = node;

    // update heights
    node->height = 1 + MAX(getHeight(node->left), getHeight(node->right));
    lChild->height = 1 + MAX(getHeight(lChild->left), getHeight(lChild->right));

    return lChild;
}

/**
 * @brief rotates the subtree with root node to the left
 *
 * rotates according to the following diagram
 *   B           A
 *  / \         / \
 * T1 A   ->   B  T3
 *   / \      / \
 *  T2 T3    T1 T2
 */
BSTNode *rotateLeft(BSTNode *node) {
    BSTNode *rChild = node->left;
    BSTNode *rlChild = rChild->right;

    node->right = rlChild;
    rChild->left = node;

    // update heights
    node->height = 1 + MAX(getHeight(node->left), getHeight(node->right));
    rChild->height = 1 + MAX(getHeight(rChild->left), getHeight(rChild->right));

    return rChild;
}

/**
 * @brief checks if the tree is balanced, if not corrects it
 * @return pointer to the node if balanced, pointer to the node, that should
 * replace the root node in the tree if not balanced
 *
 * checks the balance at node
 * if the balance is not in interval <-1,1> corrects it by rotations
 *
 * works both for balancing after inserting and deleting
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

/**
 * @brief a helper function, inserts the node in the subtree rooted with root
 * @return pointer to new root of the subtree
 */
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

    // update the height
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

/**
 * @brief finds the righmost ("highest") key in the subtree rooted with node
 * @return pointer to the found node
 */
BSTNode *BSTMax(BSTNode *node) {
    if (!node) {
        return NULL;
    }

    BSTNode *lMax = BSTMax(node->left);
    BSTNode *rMax = BSTMax(node->right);
    if (lMax && rMax) {
        // return the maximum from both children and node
        BSTNode *childMax = strcmp(lMax->key, rMax->key) > 0 ? lMax : rMax;
        return strcmp(node->key, childMax->key) > 0 ? node : childMax;
    } else if (lMax) {
        // return maximum from left child and node
        return strcmp(node->key, lMax->key) > 0 ? node : lMax;
    } else if (rMax) {
        // return maximum from right child and node
        return strcmp(node->key, rMax->key) > 0 ? node : rMax;
    }
    // else this is a leaf node - return node
    return node;
}

/**
 * @brief a helper function, removes the node with key in the subtree rooted
 * with root
 * @return pointer to the new root of the subtree
 */
BSTNode *BSTRemoveNode(BSTNode *root, const char *key, size_t *oldData) {
    if (!root) {
        return NULL;
    }

    if (key < root->key) {
        root->left = BSTRemoveNode(root->left, key, oldData);
    } else if (key > root->key) {
        root->right = BSTRemoveNode(root->right, key, oldData);
    }
    // else found the node
    else {
        // both children
        if (root->left && root->right) {
            // find the maximum in left subtree, replace the root with it and
            // delete it
            BSTNode *maxLeft = BSTMax(root->left);
            root->key = maxLeft->key;
            root->data = maxLeft->data;
            size_t discard;
            root->left = BSTRemoveNode(root->left, maxLeft->key, &discard);
        }
        // only left child
        else if (root->left) {
            BSTNode *lChild = root->left;
            free(root);
            return lChild;

        }
        // only right child
        else if (root->right) {
            BSTNode *rChild = root->right;
            free(root);
            return rChild;

        }
        // no children
        else {
            free(root);
            return NULL;
        }
    }

    // update the height
    root->height = 1 + MAX(getHeight(root->right), getHeight(root->left));

    // check and correct the balance
    return BSTBalanceTree(root);
}

size_t BSTRemove(bSearchTree *tree, const char *key) {
    size_t removedData;
    BSTRemoveNode(tree->root, key, &removedData);
    return removedData;
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
