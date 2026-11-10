/**
 * @file bst.c
 * @brief implementation for the height balancec binary search tree
 * @author Štěpán Dvořák, xdvoras00@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include "bst.h"
#include "compiler.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define MAX(A, B) A > B ? A : B

/**
 * @brief a structure for a node of the avl tree
 */
typedef struct BstNode_s {
    char *key;
    size_t data;
    struct BstNode_s *left;
    struct BstNode_s *right;
    int height;
} bstNode_t;

/**
 * @brief structure for the avl tree
 */
struct Bst_s {
    bstNode_t *root;
};

bst_t *BSTInit(void) {
    bst_t *temp = malloc(sizeof(bst_t));
    if (!temp) {
        setErrCode(99);
        return NULL;
    }

    temp->root = NULL;
    return temp;
}

/**
 * @brief a recursive deletion of all nodes
 */
void BSTClear_R(bstNode_t *root) {
    if (!root) {
        return;
    }

    BSTClear_R(root->right);
    BSTClear_R(root->left);

    free(root->key);
    free(root);
}

void BSTDestruct(bst_t *tree) {
    if (tree) {
        BSTClear_R(tree->root);
        free(tree);
    }
}

void BSTClear(bst_t *tree) {
    if (tree) {
        BSTClear_R(tree->root);
        tree->root = NULL;
    }
}

/**
 * @brief constructs a new leaf node on the heap
 * @return ptr to the node or NULL on memory allocation error
 */
bstNode_t *createNode(const char *key, size_t data, int *err, char **keyPtr) {
    bstNode_t *temp = malloc(sizeof(bstNode_t));
    if (!temp) {
        setErrCode(99);
        *err = -1;
        return NULL;
    }

    char *tempKey = malloc(strlen(key) + 1);
    if (!tempKey) {
        setErrCode(99);
        free(temp);
        *err = -1;
        return NULL;
    }
    // return the pointer to the created id
    *keyPtr = tempKey;

    temp->key = tempKey;
    strcpy(temp->key, key);
    temp->data = data;
    temp->left = temp->right = NULL;
    temp->height = 1;
    *err = 0;
    return temp;
}

/**
 * @brief get the height of subtree with node as root
 */
static inline char getHeight(bstNode_t *node) {
    if (!node) {
        return 0;
    }
    return node->height;
}

/**
 * @brief get the balance of the specified node
 */
static inline char getBalance(bstNode_t *node) {
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
bstNode_t *rotateRight(bstNode_t *node) {
    bstNode_t *lChild = node->left;
    bstNode_t *lrChild = lChild->right;

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
bstNode_t *rotateLeft(bstNode_t *node) {
    bstNode_t *rChild = node->left;
    bstNode_t *rlChild = rChild->right;

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
bstNode_t *BSTBalanceTree(bstNode_t *node) {
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
 * @brief a recursive insertion of a node
 * @param err a non NULL ptr, is set to values analogous to the retval of @ref
 * BSTInsert
 * @return a ptr to the new root
 */
bstNode_t *BSTInsert_R(bstNode_t *root, const char *key, size_t data, int *err,
                       char **keyPtr) {
    if (!root) {
        return createNode(key, data, err, keyPtr);
    }
    int result = strcmp(key, root->key);
    if (result < 0) {
        root->left = BSTInsert_R(root->left, key, data, err, keyPtr);
    } else if (result > 0) {
        root->right = BSTInsert_R(root->right, key, data, err, keyPtr);
    } else {
        // duplicite data now allowed
        *err = -2;
        return root;
    }

    // update the height
    root->height = 1 + MAX(getHeight(root->right), getHeight(root->left));

    // check and correct the balance
    return BSTBalanceTree(root);
}

int BSTInsert(bst_t *tree, const char *key, size_t data, char **keyPtr) {
    int errcode;
    char *discard;
    if (!keyPtr)
        keyPtr = &discard;
    tree->root = BSTInsert_R(tree->root, key, data, &errcode, keyPtr);
    return errcode;
}

/**
 * @brief finds the righmost ("highest") key in the subtree rooted with node
 * @return pointer to the found node
 */
bstNode_t *BSTMax(bstNode_t *node) {
    if (!node) {
        return NULL;
    }

    bstNode_t *lMax = BSTMax(node->left);
    bstNode_t *rMax = BSTMax(node->right);
    if (lMax && rMax) {
        // return the maximum from both children and node
        bstNode_t *childMax = strcmp(lMax->key, rMax->key) > 0 ? lMax : rMax;
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
 * @brief a recursive removal of a node
 * @param oldData a non NULL ptr, *oldData is set to the data of the removed
 * node
 * @param err a non NULL ptr, is set to values analogous to the retval of @ref
 * BSTRemove
 * @return a ptr to the new root
 */
bstNode_t *BSTRemove_R(bstNode_t *root, const char *key, size_t *oldData,
                       int *err) {
    if (!root) {
        // node not found
        *err = -1;
        return NULL;
    }

    int result = strcmp(key, root->key);
    if (result < 0) {
        root->left = BSTRemove_R(root->left, key, oldData, err);
    } else if (result > 0) {
        root->right = BSTRemove_R(root->right, key, oldData, err);
    }
    // else found the node
    else {
        // both children
        if (root->left && root->right) {
            *err = 0;
            *oldData = root->data;
            // find the maximum in left subroot, replace the root with it and
            // delete it
            bstNode_t *maxLeft = BSTMax(root->left);
            root->key = maxLeft->key;
            root->data = maxLeft->data;

            size_t discard;
            int discardErr;
            root->left =
                BSTRemove_R(root->left, maxLeft->key, &discard, &discardErr);
        }
        // only left child
        else if (root->left) {
            bstNode_t *lChild = root->left;
            *err = 0;
            *oldData = root->data;
            free(root->key);
            free(root);
            return lChild;

        }
        // only right child
        else if (root->right) {
            bstNode_t *rChild = root->right;
            *err = 0;
            *oldData = root->data;
            free(root->key);
            free(root);
            return rChild;

        }
        // no children
        else {
            *err = 0;
            *oldData = root->data;
            free(root->key);
            free(root);
            return NULL;
        }
    }

    // update the height
    root->height = 1 + MAX(getHeight(root->right), getHeight(root->left));

    // check and correct the balance
    return BSTBalanceTree(root);
}

int BSTRemove(bst_t *tree, const char *key, size_t *oldData) {
    size_t discard;
    int errcode;
    if (!oldData) {
        oldData = &discard;
    }
    BSTRemove_R(tree->root, key, oldData, &errcode);
    return errcode;
}

size_t *BSTSearch(bst_t *tree, const char *key) {
    if (!tree) {
        return NULL;
    }
    bstNode_t *root = tree->root;
    while (true) {
        // check if the node is not NULL
        if (!root) {
            return NULL;
        }

        int result = strcmp(key, root->key);
        if (result == 0) {
            return &root->data;
        }
        if (result < 0) {
            root = root->left;
        }
        if (result > 0) {
            root = root->right;
        }
    }
}

/**
 * @brief a recursive finding of the size of the tree
 */
size_t BSTSize_R(bstNode_t *root) {
    if (!root) {
        return 0;
    }

    return 1 + BSTSize_R(root->left) + BSTSize_R(root->right);
}

size_t BSTSize(bst_t *tree) {
    if (tree) {
        return BSTSize_R(tree->root);
    }
    return 0;
}

/**
 * @brief a recursive check for balance
 * @return check @ref BSTIsBalanced
 */
int BSTIsBalanced_R(bstNode_t *root) {
    if (!root) {
        return 1;
    }
    char balance = getBalance(root);
    int balanced = balance < 2 && balance > -2;
    return balanced && BSTIsBalanced_R(root->left) &&
           BSTIsBalanced_R(root->right);
}

int BSTIsBalanced(bst_t *tree) {
    if (tree) {
        return BSTIsBalanced_R(tree->root);
    }
    return 1;
}

/*
 * end bst.c
 */
