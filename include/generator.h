/**
 * @file generator.h
 * @brief header file for the generator of IFJcode26 from an AST
 * @author Štěpán Dvořák, xdvoras00@stud.fit.vutbr.cz
 *
 * detailed description
 */

#ifndef IFJ_GEN_H
#define IFJ_GEN_H

// #define IFJ_GEN_DEBUG

#define IFJ_GEN_BUF_SIZE 4096 /** the size of a single buffer */
#define IFJ_GEN_BUF_INSERT_NUM 64
#define IFJ_GEN_MAX_ID_LEN                                                     \
    256 /** the maximum size of an identifier (whether in IFJ26 or             \
           IFJcode26)*/

#include "ast.h"

#include <stdio.h>

/** structure for holding info about where other buffers are to be inserted */
typedef struct bufInsert {
    uint16_t textPos;
    struct codeGenBufNode
        *childBuff; /** ptr to a buffer holding the generated child block*/
} bufInsert;

/** node holding references to other buffers */
typedef struct bufInsertNode {
    bufInsert data[IFJ_GEN_BUF_INSERT_NUM];
    uint8_t size; /** how much of data is used */

    struct bufInsertNode *next; /** ptr to the next node */
} bufInsertNode;

/** node holding one 4kiB sectioin of text for generated code */
typedef struct bufSection {
    char data[IFJ_GEN_BUF_SIZE]; /** for holding the generated text */
    uint16_t size; /** how much of the data is used (the section doesn't need to
                      be full for the next section to exist) */

    struct bufSection *next; /** ptr to the next node */
} bufSection;

/** @brief buffer for keeping the generated code blocks */
typedef struct codeGenBufNode {
    bufInsertNode *firstNode;
    bufInsertNode *lastNode;

    bufSection *firstSection;
    bufSection *lastSection;
    int totalSize; /** redundant data. Useful for getting the total size faster
                    */

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
 * when IFJ_GEN_DEBUG is defined the function will check if these
 * conditions are met and print out a debugging message to stderr otherwise the
 * function will assume that they are met
 *
 * if IJF_GEN_DEBUG is not defined and the conditions aren't met, it will
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
 * when IFJ_GEN_DEBUG is defined the function will check if these
 * conditions are met and print out a debugging message to stderr otherwise the
 * function will assume that they are met
 *
 * if IJF_GEN_DEBUG is not defined and the conditions aren't met, it will
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

/**
 * @brief creates a new buffer for keeping text
 *
 * @return ptr to the created buffer of NULL on memory allocatino err
 *
 * the buffer will have one bufSectoin and one bufInsertNode allocated
 */
codeGenBufNode *codeGenCreateBuf();

/**
 * @brief destroys a buffer
 */
void codeGenDestructBuf(codeGenBufNode *buf);

/**
 * @brief inserts the text into the buffer
 *
 * @param txt non NULL ptr to the text
 * @param buffer a non NULL ptr to the buffer with at least one bufSection and
 * one bufInsertNode
 *
 * @return 0 on success, -1 on memory allocation err
 */
int codeGenInsertTxt(const char *txt, codeGenBufNode *buffer);

/**
 * @brief inserts a reference to insertedBuf at the current position in destBuf
 *
 * @param destBuf a non NULL ptr to the buffer with at least one bufSection and
 * one bufInsertNode, where the reference is to be inserted
 * @param insertedBuf a non NULL ptr to the buffer, that is to be inserted
 *
 * @return 0 on success, -1 on memeory allocation err
 *
 * the reference will represent, that at this positon in the text, the whole
 * text of insertedBuf is supposed to be inserted
 */
int codeGenInsertBufToTxt(codeGenBufNode *destBuf, codeGenBufNode *insertedBuf);

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

int codeGenStmtStringLit(const char *str, size_t lenght,
                         codeGenBufNode *buffer);

int codeGenStmtIdentif(astNode *identif, codeGenBufNode *buffer);

int codeGenStmtBlock(astNode *block, codeGenBufNode *buffer);

/**
 * @brief gets the name of a variable in the assembler
 *
 * @param id the name in IFJ26 to be translated
 *
 * @return ptr to the name in IFJcode26
 *
 * the resulting string being longer, than the size of dst, will result in
 * undefined behaviour
 */
const char *codeGenGetVarName(const char *id);

#endif

/*
 * end generator.h
 */
