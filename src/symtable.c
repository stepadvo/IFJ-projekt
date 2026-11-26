/**
 * @file symtable.c
 * @brief implementation of the symbol table
 * @author Štěpán Dvořák, xdvoras00@stud.fit.vutbr.cz
 */

#include "symtable.h"
#include "bst.h"
#include "compiler.h"
#include <stdbool.h>

/**
 * @brief private variable symbol data structure
 *
 * contains both the symbol data accesible to the user and internal data, that
 * the user should not change
 */
typedef struct {
    varData pubData;

    int dataMasked; // the data that this entry masked in the BST, -1 if no data
                    // is masked
    // maybe something for unset data type checking
    int uniqueId;
} varDataPriv;

/** @brief a structure of the local variable symbol table for each function */
typedef struct {
    bst_t *tree;

    int blockNum;              /** acts as a top for the blockTops */
    int blockTops[MAX_BLOCKS]; /** array of indexes in varArr, where the top of
                                  the block is highest index not occupied by the
                                  block */
    varDataPriv *varArr;
    int varArrCapacity;
    // varArrSize is blockTops[blockNum]

    int uqIdsUsed;
} vSymTable;

/** @brief private function symbol data (contains both private and public data)
 */
typedef struct {
    funData pubData;
    vSymTable localTable;
} funDataPriv;

/** @brief a structure for the global symbol table */
typedef struct {
    bst_t *tree;

    funDataPriv *active;

    funDataPriv *funArr;
    int funArrSize;
    int funArrCapacity;
} fSymTable;

fSymTable gFunctionTable;

// helper function declarations
/** @brief initializes the local vSymTable */
int locSymTableInit(vSymTable *table, int paramCount, paramData params[]);

/** @brief destructs the local vSymTable */
void locSymTableDestruct(vSymTable *table);

/**
 * @brief if active == true, checks if a function is active, else finds function
 * with matching id
 *
 * @return NULL when active==true and no function is active or active==false and
 * function with id isn't found
 */
funDataPriv *getFun(bool active, const char *id);

/**
 * @brief inserts the variable with pubData and mask into the varArr of the
 * local table
 * @return the index of the inserted var in varArr or -1 on failure
 */
int locTableInsertVar(vSymTable *locTable, varDataPriv data);

int symTableInit(void) {
    gFunctionTable.tree = BSTInit();
    if (!gFunctionTable.tree) {
        return -1;
    }

    gFunctionTable.funArr = gFunctionTable.active = NULL;
    gFunctionTable.funArrSize = gFunctionTable.funArrCapacity = 0;
    return 0;
}

void symTableDestruct(void) {
    for (int i = 0; i < gFunctionTable.funArrSize; i++) {
        locSymTableDestruct(&gFunctionTable.funArr[i].localTable);
    }
    BSTDestruct(gFunctionTable.tree);
    free(gFunctionTable.funArr);
}

void symTableClear(void) {
    for (int i = 0; i < gFunctionTable.funArrSize; i++) {
        locSymTableDestruct(&gFunctionTable.funArr[i].localTable);
    }
    BSTClear(gFunctionTable.tree);
    gFunctionTable.active = NULL;
    gFunctionTable.funArrSize = 0;
}

int symTableInsertFun(const char *id, varType retType, int paramCount,
                      paramData params[]) {
    // insert the id into the BST
    char *funId; // for keeping the ptr to the id
    int inserted =
        BSTInsert(gFunctionTable.tree, id, gFunctionTable.funArrSize, &funId);
    if (inserted == -2) {
        return 1; // the key is duplicate
    }
    if (inserted != 0) {
        return -1; // memory allocation err (also other values expecp success to
                   // be sure)
    }

    // ensure the funArr is big enough
    if (gFunctionTable.funArrSize >= gFunctionTable.funArrCapacity) {
        funDataPriv *newArr;
        int newCap;
        if (gFunctionTable.funArrCapacity == 0) {
            // initialize the array to the size of 32, so most smaller programms
            // don't need reallocing
            newCap = 32;
            newArr = malloc(sizeof(funDataPriv) * 32);
        } else {
            // double the capacity
            newCap = gFunctionTable.funArrCapacity * 2;
            newArr =
                realloc(gFunctionTable.funArr, sizeof(funDataPriv) * newCap);
        }

        if (!newArr) {
            // memory allocation err
            logErr(ERR_INTERNAL, LOGERR_NO_POS, LOGERR_NO_POS,
                   LOGERR_NO_DETMSG);
            // remove the function from the tree
            BSTRemove(gFunctionTable.tree, id, NULL);
            return -1;
        }

        gFunctionTable.funArr = newArr;
        gFunctionTable.funArrCapacity = newCap;
    }

    funDataPriv *fun = gFunctionTable.funArr + gFunctionTable.funArrSize++;

    // initialize the local var symbol table and insert the parameters
    if (locSymTableInit(&fun->localTable, paramCount, params) != 0) {
        return -1;
    }

    // insert the function data into the array
    fun->pubData =
        (funData){.id = funId, .retType = retType, .paramCount = paramCount};

    return 0;
}

int symTableSetNewRetType(bool active, const char *id, varType newRetType) {
    funDataPriv *fun = getFun(active, id);

    // check if the funtion is found
    if (!fun) {
        return 1;
    }

    if (fun->pubData.retType != UNKNOWN) {
        return 2; // the return type has been set
    }

    fun->pubData.retType = newRetType;
    return 0;
}

funData symTableFindFun(const char *id) {
    size_t *foundFun = BSTSearch(gFunctionTable.tree, id);

    if (!foundFun) {
        return (funData){.id = NULL};
    }

    return gFunctionTable.funArr[*foundFun].pubData;
}

int symTableActivateFun(const char *id) {
    size_t *foundFun = BSTSearch(gFunctionTable.tree, id);

    if (!foundFun) {
        gFunctionTable.active = NULL;
        return -1;
    }

    gFunctionTable.active = gFunctionTable.funArr + *foundFun;
    return 0;
}

funData symTableGetActive(void) {
    if (!gFunctionTable.active) {
        return (funData){.id = NULL};
    }
    return gFunctionTable.active->pubData;
}

int symTableInsertVar(const char *id, varType dataType, bool isConst) {
    if (!gFunctionTable.active) {
        return 2;
    }

    vSymTable *locTable = &gFunctionTable.active->localTable;

    // get an unique id inside the local table
    int uniqueId = locTable->uqIdsUsed++;

    if (locTable->blockNum <= 0) {
        return 4;
    }

    if (dataType == UNKNOWN || dataType == ANY) {
        return 3;
    }

    size_t *foundVarIdxP = BSTSearch(locTable->tree, id);

    // if a variable with the same id is found
    if (foundVarIdxP) {
        if (locTable->varArr[*foundVarIdxP].pubData.blockDepth ==
                locTable->blockNum ||
            locTable->blockNum == 1) {
            // the found variable is in the current block or we are in the first
            // block and it is a parameter
            return 1;
        }

        // else create a new variable and mask the old one
        int varIdx = locTableInsertVar(
            locTable,
            (varDataPriv){{.id = locTable->varArr[*foundVarIdxP]
                                     .pubData.id, // copy the id from the data
                                                  // we are masking
                           .dataType = dataType,
                           .isConst = isConst,
                           .blockDepth = locTable->blockNum},
                          *foundVarIdxP,
                          uniqueId});
        if (varIdx < 0) {
            return -1;
        }
        *foundVarIdxP = varIdx;
    } else {
        // else create a new variable
        int varIdx = locTableInsertVar(
            locTable,
            (varDataPriv){{.id = NULL, // for now, it will be replaced soon
                           .dataType = dataType,
                           .isConst = isConst,
                           .blockDepth = locTable->blockNum},
                          -1,
                          uniqueId});
        if (varIdx < 0) {
            return -1;
        }

        char *idPtr;
        if (BSTInsert(locTable->tree, id, varIdx, &idPtr) != 0) {
            return -1;
        }
        locTable->varArr[varIdx].pubData.id = idPtr;
    }
    return 0;
}

varData symTableFindVar(const char *id, int *uniqueId) {
    if (!gFunctionTable.active) {
        return (varData){NULL, UNKNOWN, false, 0};
    }

    vSymTable *locTable = &gFunctionTable.active->localTable;
    size_t *varIdxP = BSTSearch(locTable->tree, id);

    // check if it is found
    if (!varIdxP) {
        return (varData){NULL, UNKNOWN, true, 0};
    }

    // if the user wants the unique id
    if (uniqueId) {
        *uniqueId = locTable->varArr[*varIdxP].uniqueId;
    }

    return locTable->varArr[*varIdxP].pubData;
}

varData symTableFindVarCurrBlock(const char *id) {
    if (!gFunctionTable.active) {
        return (varData){NULL, UNKNOWN, false, 0};
    }

    vSymTable *locTable = &gFunctionTable.active->localTable;
    size_t *varIdxP = BSTSearch(locTable->tree, id);

    // check if it is found
    if (!varIdxP) {
        return (varData){NULL, UNKNOWN, true, 0};
    }

    varData foundVar = locTable->varArr[*varIdxP].pubData;
    // check if it is in the current block (works for all blocks excepth block
    // #1)
    if (foundVar.blockDepth == locTable->blockNum) {
        // return the public data
        return foundVar;
    }

    // chekc if we are in block #1 and the foundVar is a parameter
    if (locTable->blockNum == 1 && foundVar.blockDepth == 0) {
        return foundVar;
    }

    // else foundVar is not in current block
    return (varData){NULL, UNKNOWN, true, 0};
}

paramData symTableGetParam(bool active, const char *id, int number) {
    // check if the function exists
    funDataPriv *fun = getFun(active, id);

    if (!fun) {
        return (paramData){NULL, UNKNOWN};
    }

    // check if the parameter exists
    if (number >= fun->pubData.paramCount) {
        return (paramData){NULL, ANY};
    }

    varData param = fun->localTable.varArr[number].pubData;
    return (paramData){param.id, param.dataType};
}

int beginBlock(void) {
    if (!gFunctionTable.active) {
        return -1;
    }

    vSymTable *locTable = &gFunctionTable.active->localTable;

    // check if number of blocks exceeded (return the maximum value)
    if (locTable->blockNum >= MAX_BLOCKS - 1) {
        return MAX_BLOCKS - 1;
    }

    // set the top of the new block as the top of the previous block
    locTable->blockTops[locTable->blockNum + 1] =
        locTable->blockTops[locTable->blockNum];
    locTable->blockNum++;
    return locTable->blockNum;
}

int endBlock(void) {
    if (!gFunctionTable.active) {
        return -1;
    }

    vSymTable *locTable = &gFunctionTable.active->localTable;

    // check if there are no more blocks to end
    if (locTable->blockNum <= 0) {
        return 0;
    }

    // remove all the variables from the BST
    size_t *masked;
    for (int i = locTable->blockTops[locTable->blockNum - 1];
         i < locTable->blockTops[locTable->blockNum]; i++) {
        varDataPriv varInBlock = locTable->varArr[i];
        if (varInBlock.dataMasked >= 0) {
            // if the variable masked anything
            // find the entry in the BST and replace the index of the variable
            // in block with the masked index
            masked = BSTSearch(locTable->tree, varInBlock.pubData.id);
            *masked = varInBlock.dataMasked;
        } else {
            // remove the variable from the BST
            BSTRemove(locTable->tree, varInBlock.pubData.id, NULL);
        }
    }

    // decrease the number of blocks
    locTable->blockNum--;
    return locTable->blockNum;
}

int symTableBlockDepth(void) {
    if (!gFunctionTable.active) {
        return -1;
    }

    return gFunctionTable.active->localTable.blockNum;
}

// helper function implementations
int locSymTableInit(vSymTable *table, int paramCount, paramData params[]) {
    table->tree = BSTInit();
    if (!table->tree) {
        // if the initialization is unsuccessfull
        table->varArr = NULL;
        table->varArrCapacity = 0;
        return -1;
    }

    // initialize the 0-th block (used for parameters)
    varDataPriv *paramArr;
    if (paramCount > 0) {
        paramArr = malloc(sizeof(varDataPriv) * paramCount);
        if (!paramArr) {
            BSTClear(table->tree);
            logErr(ERR_INTERNAL, LOGERR_NO_POS, LOGERR_NO_POS,
                   LOGERR_NO_DETMSG);
            return -1;
        }
    } else {
        paramArr = NULL;
    }

    // insert the parameters into the 0-th block
    char *key; // for holding the ptr to the id
    for (int i = 0; i < paramCount; i++) {
        if (BSTInsert(table->tree, params[i].id, i, &key) != 0) {
            // if an insert is unsuccessfull
            BSTClear(table->tree);
            free(paramArr);
            table->varArr = NULL;
            table->varArrCapacity = 0;
            return -1;
        }
        // set the unique ids as the parameter number
        varDataPriv param = {{key, params[i].dataType, true, 0}, -1, i};
        paramArr[i] = param;
    }

    table->blockNum = 0;
    table->blockTops[0] = paramCount;
    table->varArr = paramArr;
    table->varArrCapacity = paramCount;
    // set the uqIdsUsed as the number of parameters
    table->uqIdsUsed = paramCount;
    return 0;
}

void locSymTableDestruct(vSymTable *table) {
    if (!table) {
        return;
    }
    BSTDestruct(table->tree);
    if (table->varArr) {
        free(table->varArr);
    }
}

funDataPriv *getFun(bool active, const char *id) {
    // ptr to the function to search, default to the active function
    funDataPriv *fun = gFunctionTable.active;

    if (active) {
        // check if a function is active
        if (!fun) {
            return NULL;
        }
    } else {
        // check if funciton named id is found
        size_t *funIndex = BSTSearch(gFunctionTable.tree, id);
        if (!funIndex) {
            return NULL;
        }

        // set the fun ptr as a ptr to the (*funIndex)th element
        fun = gFunctionTable.funArr + (*funIndex);
    }
    return fun;
}

int locTableInsertVar(vSymTable *locTable, varDataPriv data) {
    // make sure the array is big enough
    int firstUnusedIdx = locTable->blockTops[locTable->blockNum];
    if (locTable->varArrCapacity <= firstUnusedIdx) {
        // resize the previous array to fit the new variable
        int newCap;
        varDataPriv *newArr;
        if (locTable->varArrCapacity == 0) {
            // start at 32 so short functions don't need any other allocation
            newCap = 32;
            newArr = malloc(sizeof(varDataPriv) * newCap);
        } else {
            newCap = locTable->varArrCapacity * 2;
            newArr = realloc(locTable->varArr, sizeof(varDataPriv) * newCap);
        }

        if (!newArr) {
            logErr(ERR_INTERNAL, LOGERR_NO_POS, LOGERR_NO_POS,
                   LOGERR_NO_DETMSG);
            return -1;
        }

        locTable->varArr = newArr;
        locTable->varArrCapacity = newCap;
    }

    // insert the new variable
    locTable->varArr[firstUnusedIdx] = data;
    locTable->blockTops[locTable->blockNum]++;
    return firstUnusedIdx;
}

int symTableGetFunNum() { return BSTSize(gFunctionTable.tree); }

int symTableGetVisVarNum() {
    if (!gFunctionTable.active) {
        return -1;
    }
    return BSTSize(gFunctionTable.active->localTable.tree);
}

int symTableGetVarNum() {
    if (!gFunctionTable.active) {
        return -1;
    }
    vSymTable locTable = gFunctionTable.active->localTable;
    return locTable.blockTops[locTable.blockNum];
}

bool symTableIsEmpty() { return BSTSize(gFunctionTable.tree) == 0; }

int symTableIsLocEmpty() {
    if (!gFunctionTable.active) {
        return -1;
    }
    return BSTSize(gFunctionTable.active->localTable.tree) == 0;
}

/*
 * end symtable.c
 */
