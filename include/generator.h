/**
 * @file generator.h
 * @brief header file for the generator of IFJcode26 from an AST
 * @author Štěpán Dvořák, xdvoras00@stud.fit.vutbr.cz
 *
 * detailed description
 */

#ifndef IFJ_GENERATOR_H
#define IFJ_GENERATOR_H

// #define IFJ_GENERATOR_DEBUG

#define IFJ_GENERATOR_BUF_SIZE 4096 /** the size of a single buffer */
#define IFJ_GENERATOR_BUF_METADATA_SIZE 64

#include "ast.h"

#include <stdio.h>

/** structure for holding info about where other buffers are to be inserted */
typedef struct bufInsert {
    uint16_t textPos;
    struct codeGenBufNode
        *childBuff; /** ptr to a buffer holding the generated child block*/
} bufInsert;

/** node holding the metadata for the generated text */
typedef struct bufInsertNode {
    bufInsert data[IFJ_GENERATOR_BUF_METADATA_SIZE];
    uint8_t size; /** how much of data is used */

    struct bufInsertNode *next; /** ptr to the next node */
} bufInsertNode;

/** node holding one area for generated code */
typedef struct bufSection {
    char data[IFJ_GENERATOR_BUF_SIZE]; /** for holding the generated text */
    uint16_t size;                     /** how much of the data is used */

    struct bufSection
        *next; /** ptr to the next node (continuation of the text)*/
} bufSection;

/** @brief buffer for keeping the generated code blocks */
typedef struct codeGenBufNode {
    bufInsertNode *firstNode;
    bufInsertNode *lastNode;

    bufSection *firstSection;
    bufSection *lastSection;

    struct codeGenBufNode *nextBuf;
} codeGenBufNode;

/** @brief an output buffer data structure (one node holds one function)*/
typedef struct outBufNode {
    char *buf;
    size_t bufSize;
    struct outBufNode *nextBuf;
} outBufNode;

/** @brief internal state of the code generator (keeps all the buffers)*/
typedef struct {
    /** a linked list of buffers*/
    codeGenBufNode *firstBuf;
    codeGenBufNode *lastBuf;

    /** a linked list of output buffers for all the functions */
    outBufNode *firstFun;
    outBufNode *lastFun;
} codeGenState;

/*
 * --------------------
 *  EXTERNAL FUNCTIONS
 * --------------------
 */

/**
 * @brief initializes the state
 *
 * @return ptr to the local state (a ptr to a local static variable)
 */
codeGenState *codeGenInit();

/**
 * @brief destroys the state of the code generator
 */
void codeGenDestruct(codeGenState *state);

/**
 * @brief a function for generating a code block
 *
 * @param blockRoot ptr to the node with .type=AST_BLOCK, that roots the block
 *
 * @return 0 on success, 1 if state or blockRoot are NULL, -1 on memory
 * allocation err
 *
 * the function will generate a block in IFJcode26 and put it inside a buffer
 *
 * the function works only if these conditions are met:
 *  - if the blocks contains any sub blocks, they have to be already generated
 *  - if any function is called inside the block, its return type already has to
 * be known
 *  - no variables in the ast can have a data type as UNKNOWN or ANY
 *  - the corresponding function has to be activated in the symbol table and its
 * block stack has to correspond:
 *      - all the local variables of the block have to be in the current block
 *      - all the non local variables accessed have to be in the symbol table
 * (not in current block)
 *
 * the function reads the symbol table but doesn't modify it in any way
 *
 * when IFJ_GENERATOR_DEBUG is defined the function will check if these
 * conditions are met and print out a debugging message to stderr otherwise the
 * function will assume that they are met
 *
 * if IJF_GENERATOR_DEBUG is not defined and the conditions aren't met, it will
 * result in undefined behaviour
 */
int codeGenBlock(codeGenState *state, astNode *blockRoot);

/**
 * @brief a function for generating a function
 *
 * @param funRoot ptr to the node with .type=AST_FUNC_DEF, that roots the block
 *
 * @return 0 on success, 1 if state or funRoot are NULL, -1 on memory allocation
 * err
 *
 * the function will generate the function overhead in IFJcode26 and append it
 * to a output buffer
 *
 * the function works only if these conditions are met:
 *  - the body block of the function has to be generated
 *  - the function symbol has to be in the symbol table
 *
 * the function reads the symbol table but doesn't modify it in any way
 *
 * when IFJ_GENERATOR_DEBUG is defined the function will check if these
 * conditions are met and print out a debugging message to stderr otherwise the
 * function will assume that they are met
 *
 * if IJF_GENERATOR_DEBUG is not defined and the conditions aren't met, it will
 * result in undefined behaviour
 */
int codeGenFunction(codeGenState *state, astNode *funRoot);

/**
 * @brief outputs the generated code
 *
 * @return lenght of the generated output in Bytes
 *
 * the funciton outputs the output buffer to stdout and clears the output buffer
 */
int codeGenOutCode(codeGenState *state);

/**
 * @brief discards the generated code in the output buffer
 */
void codeGenDiscard(codeGenState *state);

/*
 * --------------------
 *  INTERNAL FUNCTIONS
 * --------------------
 */

codeGenBufNode *codeGenCreateBuf();

void codeGenDestructBuf(codeGenBufNode *buf);

int codeGenStatementArr(astNode **nodes, size_t count, codeGenBufNode *buffer);

int codeGenStatement(astNode *node, codeGenBufNode *buffer);

int codeGenStmtVarDef(astNode *varDef, codeGenBufNode *buffer);

int codeGenStmtAssign(astNode *assign, codeGenBufNode *buffer);

int codeGenStmtIf(astNode *ifStmt, codeGenBufNode *buffer);

int codeGenStmtWhile(astNode *whileLoop, codeGenBufNode *buffer);

int codeGenStmtFor(astNode *forLoop, codeGenBufNode *buffer);

int codeGenStmtFuncCall(astNode *funcCall, codeGenBufNode *buffer);

int codeGenStmtReturn(astNode *ret, codeGenBufNode *buffer);

int codeGenStmtBinOp(astNode *binOp, codeGenBufNode *buffer);

int codeGenStmtUnOp(astNode *unOp, codeGenBufNode *buffer);

int codeGenStmtLiteral(astNode *lit, codeGenBufNode *buffer);

int codeGenStmtIdentif(astNode *identif, codeGenBufNode *buffer);

int codeGenStmtBlock(astNode *block, codeGenBufNode *buffer);

#endif

/*
 * end generator.h
 */
