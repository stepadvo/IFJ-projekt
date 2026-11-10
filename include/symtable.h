/**
 * @file symtable.h
 * @brief interface declaration for the symbol table ADT
 * @author Štěpán Dvořák, xdvoras00
 *
 * Declarations of the functions working with the SymTable struct.
 */

#ifndef SYMTABLE_H
    #define SYMTABLE_H

#include "compiler.h"
#include <stdbool.h>
#include <stdlib.h>

#define MAX_BLOCKS 128

/** @brief structe containing data about the var symbol (var or val) */
typedef struct {
    const char *id;
    varType dataType;
    bool isConst; /** for parameters set to true */
    int blockDepth;
} varData;

/** @brief structe containing data about the function symbol */
typedef struct {
    const char *id;
    varType retType;
    int paramCount;
} funData;

/** @brief structe containing data about the parameter symbol */
typedef struct {
    const char *id;
    varType dataType;
} paramData;

/**
 * @brief initializes the global fun symbol table as empty
 * @return 0 on success, -1 on memory allocation err
 */
int symTableInit(void);

/** @brief destructs the global fun symbol table */
void symTableDestruct(void);

/** @brief clears all symbols from the global fun symbol table */
void symTableClear(void);

/**
 * @briefs adds a function symbol to the global symbol table
 *
 * @param id the name of function, symTableInsertFun copies it into the symbol
 * structure
 * @param varType return data type of the function (UNKNOWN if not explicitely
 * set)
 * @param paramCount number of function parameters
 * @param params an array with the parameter data of size paramCount (inserted
 * from left to right)
 *
 * @return 0 on succesfull insert, 1 when the id is already in use, -1 on memory
 * allocation err
 *
 * @details when the id is already in use, the global table will not be changed
 * @details the parameters will be numbered with their index in the input array
 * @details use the ANY retType only for the built in functions
 */
int symTableInsertFun(const char *id, varType retType, int paramCount,
                      paramData params[]);

/**
 * @brief updates the return type
 *
 * @param active true when the updated function is the active function, false it
 * is the function with matching id
 * @param retType the new return type
 *
 * @return  0 when succesfully updated
 *          1 when no function is active/there is no function with matching id
 *          2 when the previous retType is not UNKNOWN (no changes)
 */
int symTableSetNewRetType(bool active, const char *id, varType newRetType);

/**
 * @brief finds the function in the global table
 *
 * @param id identifier of the symbol to be found
 *
 * @return data of the function, or {.id=NULL} when not found
 */
funData symTableFindFun(const char *id);

/**
 * @brief activates the function with matching id
 *
 * @return 0 on successfull find, -1 when the id is not found
 *
 * @details when no funciton is found, no function will be active
 */
int symTableActivateFun(const char *id);

/**
 * @brief gets of the active funciton
 *
 * @return {.id=NULL} if no function is active
 */
funData symTableGetActive(void);

/**
 * @brief adds an symbol to the current block of the local table of the active
 * function
 *
 * @param id the name of symbol, symTableInsertVar duplicates it to the symbol
 * structure
 * @param dataType data type of the symbol (don't use UNKNOWN or ANY)
 * @param isConst whether the value is constant (whether it is val or var)
 *
 * @return  0 on success
 *          1 when the id is already in use within the current block
 *          2 when no function is active
 *          3 when the dataType is UNKNOWN or ANY
 *          4 when there are no blocks in the function
 *          -1 on memory allocation error
 *
 * @details The id must be unique within a block. If an id is identical as an id
 * in a "higher" block, the id from the "higher" block is masked and can be used
 * when the current block is ended
 */
int symTableInsertVar(const char *id, varType dataType, bool isConst);

/**
 * @brief finds the symbol in the whole table int the active function starting
 * from the current block and continuing outwards (searches both local variables
 * and parameters. Parameters will behave as a local variable on the first
 * block)
 *
 * @param id identifier of the symbol to be found
 *
 * @return  data of the variable when found
 *          {.id = NULL, .dataType = UNKNOWN, .isConst = true} when not found
 *          {.id = NULL, .dataType = UNKNOWN, .isConst = false} when no function
 * is active
 */
varData symTableFindVar(const char *id);

/**
 * @brief finds the symbol only within the current block of the active function
 *
 * @param id identifier of the symbol to be found
 *
 * @return  data of the variable when found
 *          {.id = NULL, .dataType = UNKNOWN, .isConst = true} when not found
 *          {.id = NULL, .dataType = UNKNOWN, .isConst = false} when no function
 * is active
 *
 * @details when in the first block, both the local variables and parameters are
 * searched
 * @details when blockDepth < 1, only the parameters are searched
 */
varData symTableFindVarCurrBlock(const char *id);

/**
 * @brief gets the number-th parameter of a function
 *
 * @return  data of the variable when the param is found
 *          {.id = NULL, .dataType = UNKNOWN} when no function
 *              is active / no function with matching id exists
 *          {.id = NULL, .dataType = ANY} when the function doesn't have
 * parameter with this number
 *
 * @details if active == true, the active function will be searched and id will
 * not be taken into account
 * @details if active == false, the function with id will be found and searched
 */
paramData symTableGetParam(bool active, const char *id, int number);

/**
 * @brief creates a new "lowest" block in the active function
 *
 * @return  new number of blocks (block depth) on success
 *          -1 if no function is active
 *
 * @details no changes, when the MAX_BLOCKS is exceeded
 */
int beginBlock(void);

/**
 * @brief ends the current block in the active function
 *
 * @return  number of blocks (block depth) remaining on success
 *          -1 when no function is active
 *
 * @details ends the block whether it is empty or not
 * @details no changes, when there are no more blocks to end (blockDepth < 1)
 */
int endBlock(void);

/**
 * @brief gets the block depth of the active function
 *
 * @returns the block depth (positive number) or -1 when no function is active
 */
int symTableBlockDepth(void);

#endif

/*
 * end symtable.h
 */
