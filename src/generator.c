/**
 * @file generator.c
 * @brief implementation of IFJcode26 generator
 * @author Štěpán Dvořák, xdvoras00@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include "generator.h"
#include "ast.h"
#include "symtable.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** a macro for checking if the requirements set for codeGenBlock and codeGenFun
 * to work are met*/
#ifdef IFJ_GENERATOR_DEBUG
    #define GEN_CHECK(cond, retFail, str, ...)                                 \
        if (!(cond)) {                                                         \
            char buf[512];                                                     \
            sprintf(buf, str, ##__VA_ARGS__);                                  \
            fprintf(stderr, "GENERR: %s at %i: %s\n", __FUNCTION__, __LINE__,  \
                    buf);                                                      \
            return retFail;                                                    \
        }                                                                      \
        (void)0;
#else
    #define GEN_CHECK(cond, str, ...) (void)0
#endif

codeGenState *codeGenInit() {
    static codeGenState state = {NULL, NULL, NULL, NULL};
    return &state;
}

void codeGenDestruct(codeGenState *state) {
    codeGenBufNode *next;
    for (codeGenBufNode *curr = state->firstBuf; curr != NULL; curr = next) {
        next = curr->nextBuf;
        codeGenDestructBuf(curr);
    }
    state->firstBuf = state->lastBuf = NULL;

    codeGenDiscard(state);
}

int codeGenBlock(codeGenState *state, astNode *blockRoot) {
    if (!state || !blockRoot) {
        return 1;
    }
    GEN_CHECK(blockRoot->type == AST_BLOCK, -1,
              "The node must be a block. Type is %i", blockRoot->type);

    // create a new buffer for generating
    codeGenBufNode *buffer = codeGenCreateBuf();
    if (!buffer) {
        return -1;
    }

    // generate all the statements in the block into the created buffer
    if (codeGenStatementArr(blockRoot->data.block.statements,
                            blockRoot->data.block.count, buffer) != 0) {
        return -1;
    }

    return 0;
}

int codeGenFunction(codeGenState *state, astNode *funRoot) {
    (void)state;
    (void)funRoot;
    return 0;
}

int codeGenOutCode(codeGenState *state) {
    outBufNode *next;
    int len = 0;

    for (outBufNode *curr = state->firstFun; curr != NULL; curr = next) {
        puts(curr->buf); // output the whole buffer to stdout

        len += curr->bufSize;
        next = curr->nextBuf;

        free(curr); // free the buffers as we go
    }
    state->firstFun = state->lastFun = NULL;

    return len;
}

void codeGenDiscard(codeGenState *state) {
    outBufNode *next;

    for (outBufNode *curr = state->firstFun; curr != NULL; curr = next) {
        next = curr->nextBuf;

        free(curr); // free the buffers as we go
    }
    state->firstFun = state->lastFun = NULL;
}

/*
 * --------------------
 *  INTERNAL FUNCTIONS
 * --------------------
 */

codeGenBufNode *codeGenCreateBuf() {
    codeGenBufNode *tempBufP = malloc(sizeof(codeGenBufNode));
    if (!tempBufP) {
        return NULL;
    }

    bufInsertNode *tempNodeP = malloc(sizeof(bufInsertNode));
    if (!tempNodeP) {
        goto err_node;
    }

    bufSection *tempSectionP = malloc(sizeof(bufSection));
    if (!tempSectionP) {
        goto err_area;
    }

    tempBufP->firstNode = tempBufP->lastNode = tempNodeP;
    tempBufP->firstSection = tempBufP->lastSection = tempSectionP;
    tempBufP->nextBuf = NULL;

    return tempBufP;

err_area:
    free(tempNodeP);
err_node:
    free(tempBufP);
    return NULL;
}

void codeGenDestructBuf(codeGenBufNode *buf) {
    // free all the metadata
    bufInsertNode *nextNode;
    for (bufInsertNode *curr = buf->firstNode; curr != NULL; curr = nextNode) {
        nextNode = curr->next;
        free(curr);
    }

    // free all the text areas
    bufSection *nextSection;
    for (bufSection *curr = buf->firstSection; curr != NULL;
         curr = nextSection) {
        nextSection = curr->next;
        free(curr);
    }

    free(buf);
}

int codeGenStatementArr(astNode **nodes, size_t count, codeGenBufNode *buffer) {
    for (size_t i = 0; i < count; i++) {
        if (codeGenStatement(nodes[i], buffer) != 0) {
            return -1;
        }
    }
    return 0;
}

int codeGenStatement(astNode *node, codeGenBufNode *buffer) {
    switch (node->type) {
    case AST_VAR_DEF:
        codeGenStmtVarDef(node, buffer);
        break;

    case AST_ASSIGN:
        codeGenStmtAssign(node, buffer);
        break;

    case AST_IF:
        codeGenStmtIf(node, buffer);
        break;

    case AST_WHILE:
        codeGenStmtWhile(node, buffer);
        break;

    case AST_FOR:
        codeGenStmtFor(node, buffer);
        break;

    case AST_FUNC_CALL:
        codeGenStmtFuncCall(node, buffer);
        break;

    case AST_RETURN:
        codeGenStmtReturn(node, buffer);
        break;

    case AST_BINOP:
        codeGenStmtBinOp(node, buffer);
        break;

    case AST_UNOP:
        codeGenStmtUnOp(node, buffer);
        break;

    case AST_LITERAL:
        codeGenStmtLiteral(node, buffer);
        break;

    case AST_IDENTIFIER:
        codeGenStmtIdentif(node, buffer);
        break;

    case AST_BLOCK:
        codeGenStmtBlock(node, buffer);
        break;

    default:
        GEN_CHECK(false, "Unsupported ast node in tree (PROGRAMM or FUNC_DEF)");
        return -1;
    }
    return 0;
}

int codeGenStmtVarDef(astNode *varDef, codeGenBufNode *buffer) {
    (void)varDef;
    (void)buffer;
    return 0;
}

int codeGenStmtAssign(astNode *assign, codeGenBufNode *buffer) {
    (void)assign;
    (void)buffer;
    return 0;
}

int codeGenStmtIf(astNode *ifStmt, codeGenBufNode *buffer) {
    (void)ifStmt;
    (void)buffer;
    return 0;
}

int codeGenStmtWhile(astNode *whileLoop, codeGenBufNode *buffer) {
    (void)whileLoop;
    (void)buffer;
    return 0;
}

int codeGenStmtFor(astNode *forLoop, codeGenBufNode *buffer) {
    (void)forLoop;
    (void)buffer;
    return 0;
}

int codeGenStmtFuncCall(astNode *funcCall, codeGenBufNode *buffer) {
    (void)funcCall;
    (void)buffer;
    return 0;
}

int codeGenStmtReturn(astNode *ret, codeGenBufNode *buffer) {
    (void)ret;
    (void)buffer;
    return 0;
}

int codeGenStmtBinOp(astNode *binOp, codeGenBufNode *buffer) {
    (void)binOp;
    (void)buffer;
    return 0;
}

int codeGenStmtUnOp(astNode *unOp, codeGenBufNode *buffer) {
    (void)unOp;
    (void)buffer;
    return 0;
}

int codeGenStmtLiteral(astNode *lit, codeGenBufNode *buffer) {
    (void)lit;
    (void)buffer;
    return 0;
}

int codeGenStmtIdentif(astNode *identif, codeGenBufNode *buffer) {
    (void)identif;
    (void)buffer;
    return 0;
}

int codeGenStmtBlock(astNode *block, codeGenBufNode *buffer) {
    (void)block;
    (void)buffer;
    return 0;
}

/*
 * end generator.c
 */
