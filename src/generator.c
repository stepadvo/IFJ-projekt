/**
 * @file generator.c
 * @brief implementation of IFJcode26 generator
 * @author Štěpán Dvořák, xdvoras00@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include "generator.h"
#include "ast.h"
#include "compiler.h"
#include "symtable.h"
#include "token.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IFJ_GEN_DEBUG
/** a macro for checking if the requirements set for codeGenBlock and codeGenFun
 * to work are met*/
#ifdef IFJ_GEN_DEBUG
    #define GEN_CHECK_PRINT(cond, retFail, str, ...)                           \
        if (!(cond)) {                                                         \
            char buf[512];                                                     \
            sprintf(buf, str, ##__VA_ARGS__);                                  \
            fprintf(stderr, "GENERR: %s at %i: %s\n", __FUNCTION__, __LINE__,  \
                    buf);                                                      \
            return retFail;                                                    \
        }                                                                      \
        (void)0;

    #define GEN_CHECK(cond, retFail)                                           \
        if (!(cond))                                                           \
        return retFail
#else
    #define GEN_CHECK(cond, str, ...) (void)0
    #define GEN_CHECK(cond, retFail) (void)0
#endif

#define INSERT_AND_RETURN(txt, buffer)                                         \
    if (codeGenInsertTxt(txt, buffer) != 0) {                                  \
        return -1;                                                             \
    }                                                                          \
    return 0

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
    GEN_CHECK_PRINT(blockRoot->type == AST_BLOCK, -1,
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

    // set the ptr to the created generated buffer in ast
    blockRoot->data.block.bufferPtr = (void *)buffer;
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
        setErrCode(99);
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

    tempBufP->totalSize = 0;

    return tempBufP;

err_area:
    free(tempNodeP);
err_node:
    free(tempBufP);
    return NULL;
}

void codeGenDestructBuf(codeGenBufNode *buf) {
    if (!buf) {
        return;
    }
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

int codeGenInsertTxt(const char *txt, codeGenBufNode *buffer) {
    size_t txtLen = strlen(txt);
    bufSection *potentialDest = buffer->lastSection;

    if (txtLen + potentialDest->size >= IFJ_GEN_BUF_SIZE - 1) {
        // the last section's capacity is not sufficient
        potentialDest = malloc(sizeof(bufSection));
        if (!potentialDest) {
            setErrCode(99);
            return -1;
        }

        potentialDest->next = NULL;
        potentialDest->size = 0;

        // append new section to the end
        buffer->lastSection->next = potentialDest;
        buffer->lastSection = potentialDest;
    }
    // else potentialDest stays as the last section

    // copy the text into the destination buffer
    // dest - ptr to the first unused character in the ->data (null character)
    // len - copy the text and the null character
    memcpy(&(potentialDest->data[potentialDest->size]), txt, txtLen + 1);

    // update the size
    potentialDest->size += txtLen;
    buffer->totalSize += txtLen;
    return 0;
}

int codeGenInsertBufToTxt(codeGenBufNode *destBuf,
                          codeGenBufNode *insertedBuf) {
    bufInsertNode *potentialDest = destBuf->lastNode;
    if (potentialDest->size >= IFJ_GEN_BUF_INSERT_NUM) {
        // the last node's capacity is full - we need to create a new one
        potentialDest = malloc(sizeof(bufInsertNode));
        if (!potentialDest) {
            setErrCode(99);
            return -1;
        }

        potentialDest->next = NULL;
        potentialDest->size = 0;

        // we append the new node
        destBuf->lastNode->next = potentialDest;
        destBuf->lastNode = potentialDest;
    }
    // else potentialDest can stay as the last node in the list

    // insert the new reference to the first viable position
    potentialDest->data[potentialDest->size++] =
        (bufInsert){.childBuff = insertedBuf, .textPos = destBuf->totalSize};

    return 0;
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
        GEN_CHECK_PRINT(false, -1,
                        "Unsupported ast node in tree (PROGRAMM or FUNC_DEF)");
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
    // just generate the RHS, it will leave its result on the stack
    codeGenStatement(ret->data.returnStmt.value, buffer);
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
    // identify the type of literal
    char buf[IFJ_GEN_MAX_ID_LEN];
    switch (lit->data.literal.litType) {
    case TOKEN_INT_LITERAL:
        sprintf(buf, "int@%li\n", lit->data.literal.attribute.intValue);
        break;
    case TOKEN_BOOL_LITERAL:
        sprintf(buf, "bool@%s\n", "true");
        break;
    case TOKEN_DOUBLE_LITERAL:
        sprintf(buf, "float@%a\n", lit->data.literal.attribute.doubleValue);
        break;
    case TOKEN_STRING_LITERAL:
        // call a function to handle the string literal
        return codeGenStmtStringLit(lit->data.literal.attribute.text.data,
                                    lit->data.literal.attribute.text.length,
                                    buffer);
    /*
    TODO inquire about unit literal
    case UNIT:
        memcpy(buf, "nil@nil", 4);
        break;
     */
    default:
        GEN_CHECK_PRINT(
            false, -1,
            "A literal at %p doesn't have the litType set as a literal", lit);
    }

    INSERT_AND_RETURN(buf, buffer);
}

int codeGenStmtStringLit(const char *str, size_t lenght,
                         codeGenBufNode *buffer) {
    // each character can be replaced with at most 4 characters of escape
    // sequence + lenght of the prefix
    size_t prefixLen = 7;
    char buf[lenght * 4 + prefixLen + 1];

    // insert the prefix
    strcpy(buf, "string@");
    size_t bufIdx = prefixLen;

    for (size_t i = 0; i < lenght; i++) {
        // convert the character to an escape sequence if needed
        char c = str[i];
        if (c <= 32 || c == 35 || c == 92) {
            // set the first character as backslash
            buf[bufIdx] = '\\';

            // convert to decimal
            buf[bufIdx + 3] = c % 10;
            c /= 10;
            buf[bufIdx + 2] = c % 10;
            c /= 10;
            buf[bufIdx + 1] = c;
            bufIdx += 4;
        }
        // else just copy the character
        else {
            buf[bufIdx++] = c;
        }
    }

    INSERT_AND_RETURN(buf, buffer);
}

int codeGenStmtIdentif(astNode *identif, codeGenBufNode *buffer) {
    // translate the name to IFJcode26
    const char *name = codeGenGetVarName(identif->data.identifier.name);
    GEN_CHECK(name != NULL, -1);

    // assemble the final string and insert to buffer
    char buf[IFJ_GEN_MAX_ID_LEN + 7];
    sprintf(buf, "PUSHS %s\n", name);

    INSERT_AND_RETURN(buf, buffer);
}

int codeGenStmtBlock(astNode *block, codeGenBufNode *buffer) {
    // check if the child block has been generated
    GEN_CHECK_PRINT(block->data.block.bufferPtr != NULL, -1,
                    "A child block at %p wasn't generated yet", block);

    // insert a reference to the child block
    if (codeGenInsertBufToTxt(buffer, block->data.block.bufferPtr) != 0) {
        return -1;
    }
    return 0;
}

const char *codeGenGetVarName(const char *id) {
    static char nameBuf[IFJ_GEN_MAX_ID_LEN];
    // find the name in symTable
    varData nameData = symTableFindVar(id);
    GEN_CHECK_PRINT(nameData.id != NULL, NULL,
                    "The variable name %s not found in symtable\n", id);

    // construct the name in IFJ26
    sprintf(nameBuf, "LF@%s$%i", id, nameData.blockDepth);
    return nameBuf;
}

/*
 * end generator.c
 */
