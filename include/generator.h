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

#define IFJ_GEN_BUF_SIZE 1024 /** the size of a single buffer */
#define IFJ_GEN_BUF_INSERT_NUM 64
#define IFJ_GEN_MAX_ID_LEN                                                     \
    1024 /** the maximum size of an identifier (whether in IFJ26 or            \
           IFJcode26)*/

#include "ast.h"
#include <stdbool.h>

typedef enum {
    FUN_ORD,
    FUN_SUBSTR,
    FUN_READINT,
    FUN_READDOUBLE,
    FUN_BOOLTOSTR,
    FUN_NUM
} builtinFuns;

/** node holding one 4kiB sectioin of text for generated code */
typedef struct bufSection {
    char data[IFJ_GEN_BUF_SIZE]; /** for holding the generated text */
    uint16_t size; /** how much of the data is used (the section doesn't need to
                      be full for the next section to exist) */

    struct bufSection *next; /** ptr to the next node */
} bufSection;

/** @brief buffer for keeping the generated code blocks */
typedef struct codeGenBufNode {
    /** linked list of sections */
    bufSection *firstSection;
    bufSection *lastSection;
    int totalSize; /** redundant data. Useful for getting the total size faster
                    */
    int labelNum;  /** the number given to a statement needing labels so the
                      label names are identical (works if one buffer is analogous
                      to one source code block) */

    struct codeGenBufNode *nextBuf;
    struct codeGenBufNode *prevBuf;
} codeGenBufNode;

/** @brief an output buffer data structure (one node holds one function)*/
typedef struct outBufNode {
    char *buf;
    size_t bufSize;
    struct outBufNode *nextBuf;
} outBufNode;

/**
 * @brief internal state of the code generator (keeps all the buffers)
 * do not change this structure when using the generator
 * */
typedef struct {
    /** a linked list of buffers*/
    codeGenBufNode *firstBuf;
    codeGenBufNode *lastBuf;

    /** a linked list of output buffers for all the functions */
    outBufNode *firstFun;
    outBufNode *lastFun;

    char mainFunName[IFJ_GEN_MAX_ID_LEN]; /** ptr to the name of the main
                                             function in the ast */
    char *builtinFuns[FUN_NUM]; /** ptrs to strings containing the code for
                                   builtin functions, that are called and not
                                   inserted in place */

    bool builtinNeeded[FUN_NUM];
} codeGenState;

/*
 * --------------------
 *  EXTERNAL FUNCTIONS
 * --------------------
 */

/**
 * @brief initializes the state
 *
 * the .builtinFuns will be set
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
 * be known and the function must be in the symbol table
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
 * @param funRoot ptr to the node with .type=AST_FUNC_DEF, that roots the
 * function tree
 *
 * @return 0 on success, 1 if state or funRoot are NULL, -1 on memory allocation
 * err
 *
 * the buffers holding blocks belonging to the generated function will be
 * destroyed
 *
 * the function works only if these conditions are met:
 *  - the body block of the function has to be generated
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
 * the buffer will have one bufSection allocated and will be inserted at the end
 * of the list of buffers
 */
codeGenBufNode *codeGenCreateAndInsertBuf(codeGenState *state);

/**
 * @brief destroys a buffer structure
 */
void codeGenDestructBuf(codeGenBufNode *buffer);

/**
 * @brief removes and frees the buffer from the list in state witouth touching
 * the sections (they must be moved elsewhere before calling this function)
 */
void codeGenRemoveBuf(codeGenState *state, codeGenBufNode *buffer);

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
 * @param insertedNode a non NULL ptr to node in ast symbolizing the inserted
 * block (its ptr to the buffer will be changed to NULL)
 *
 * @return 0 on success, -1 on memeory allocation err
 *
 * the reference will represent, that at this positon in the text, the whole
 * text of insertedBuf is supposed to be inserted
 */
int codeGenInsertBufToTxt(codeGenState *state, codeGenBufNode *destBuf,
                          codeGenBufNode *insertedBuf, astNode *insertedNode);

/**
 * @brief outputs the contents of the buffer
 *
 * @param outBuf output string, where the data is to be saved
 * @param buffer the buffer, that should be outputted in a plain string
 *
 * @return the number of outputted bytes
 */
int codeGenOutputBuf(char *outBuf, codeGenBufNode *buffer);

/**
 * @brief generates an array of statements (nodes of ast) to a buffer
 *
 * @param nodes an array of ptrs to nodes with minimal size of count
 * @param number of nodes generated
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStatementArr(astNode **nodes, size_t count, codeGenBufNode *buffer,
                        codeGenState *state);

/**
 * @brief generates one node of the ast to buffer
 *
 * @param node a non NULL ptr to the node
 * @param buffer a non NULL ptr to the buffer
 * @param leaveVal whether the resulting value should be left on stack (if false
 * then statements only evaluating a value will be ignored)
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStatement(astNode *node, codeGenBufNode *buffer, bool leaveVal,
                     codeGenState *state);

/**
 * @brief generates a variable definition to buffer
 *
 * @param varDef a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtVarDef(astNode *varDef, codeGenBufNode *buffer,
                      codeGenState *state);

/**
 * @brief generates a assignment to buffer
 *
 * @param assign a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtAssign(astNode *assign, codeGenBufNode *buffer,
                      codeGenState *state);

/**
 * @brief generates an if statement to buffer
 *
 * @param ifStmt a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtIf(astNode *ifStmt, codeGenBufNode *buffer, codeGenState *state);

/**
 * @brief generates a while loop to buffer
 *
 * @param whileLoop a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtWhile(astNode *whileLoop, codeGenBufNode *buffer,
                     codeGenState *state);

/**
 * @brief generates a for loop to buffer
 *
 * @param forLoop a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtFor(astNode *forLoop, codeGenBufNode *buffer,
                   codeGenState *state);

/**
 * @brief generates a function call to buffer
 *
 * @param funcCall a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 * @param leaveVal whether the returned value should be left on stack (if the
 * result of called function is used)
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtFuncCall(astNode *funcCall, codeGenBufNode *buffer,
                        bool leaveVal, codeGenState *state);

/**
 * @brief generates a return statement to buffer (if retval is not Unit, leaves
 * the return value on stack)
 *
 * @param ret a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtReturn(astNode *ret, codeGenBufNode *buffer,
                      codeGenState *state);

/**
 * @brief generates a binary operation to buffer (leaves the result on stack)
 *
 * @param binOp a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtBinOp(astNode *binOp, codeGenBufNode *buffer,
                     codeGenState *state);

/**
 * @brief generates a unary operation to buffer (leaves the result on stack)
 *
 * @param unOp a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtUnOp(astNode *unOp, codeGenBufNode *buffer, codeGenState *state);

/**
 * @brief generates a literal to buffer (leaves the value on stack)
 *
 * @param lit a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtLiteral(astNode *lit, codeGenBufNode *buffer);

/**
 * @brief generates a string literal to the buffer (leaves the string literal on
 * stack)
 *
 * @param str a non NULL ptr to the normal string with at least lenght + 1
 * characters
 * @param lenght the lenght of the string
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtStringLit(const char *str, size_t lenght,
                         codeGenBufNode *buffer);

/**
 * @brief generates a value from identifier to buffer (leaves the value of the
 * variable on stack)
 *
 * @param identif a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtIdentif(astNode *identif, codeGenBufNode *buffer);

/**
 * @brief adds a reference to another buffer (block)
 *
 * @param block a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return 0 on success, -1 on failure (memory allocation err if not debugging)
 */
int codeGenStmtBlock(astNode *block, codeGenBufNode *buffer,
                     codeGenState *state);

/**
 * @brief checks if the function called is a builtin function and genereates its
 * call
 *
 * @param funcCall a non NULL ptr to the ast node
 * @param buffer a non NULL ptr to the buffer
 *
 * @return  0 if the function was builtin
 *          1 if the function wasn't builtin or it can be handeled by a call to
 * the funciton (still needs to be generated) -1 on failure (memory allocation
 * err if not debugging)
 *
 * it is assumed, that putting the parameters on stack has been already
 * generated the generated function will leave its result on stack
 */
int codeGenBuiltinFunCall(astNode *funcCall, codeGenBufNode *buffer,
                          codeGenState *state);

/**
 * @brief assembles the name of a variable in IFJcode26
 *
 * @param id the name in IFJ26 to be translated
 *
 * @return ptr to the name in IFJcode26
 *
 * the function returns ptr to a local static array. The name is valid only
 * until the next call of the function
 *
 * local variables follow the naming convention:
 *  LF@< name in IFJ26 >$< unique ID from symtable >
 *
 * (the variable name is redundant, but simplifies debugging)
 */
const char *codeGenGetVarName(const char *id);

/**
 * @brief assembles a unique label name
 *
 * @param buffer non NULL ptr to the buffer the label will be placed into
 *
 * @return ptr ot the label id
 *
 * the function returns ptr to a local static array. The id is valid only until
 * the next call of the function
 *
 * label ids follow the naming convention:
 *  < function name >$< blockDepth >$< unique number in buffer (block) >
 */
const char *codeGenGetLabelId(codeGenBufNode *buffer);

#endif

/*
 * end generator.h
 */
