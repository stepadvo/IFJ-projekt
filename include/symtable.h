/**
 * @file symtable.h
 * @brief interface declaration for the symbol table ADT
 * @author Štěpán Dvořák, xdvoras00
 *
 * Declarations of the functions working with the SymTable struct.
 */

#ifndef SYMTABLE_H
    #define SYMTABLE_H

    #include <stdbool.h>
    #include <stdlib.h>

/** @brief the symbol table abstract data type */
typedef struct SymTable symTable;

/** @brief enum of the possible data types in IFJ26 */
typedef enum { UNIT, INT, DOUBLE, STRING, BOOL, UNKNOWN } varType;

/** @brief enum of the types of symbols in the table */
//typedef enum { VAL, VAR, FUN } symType;

///**
// * @brief structure for data for each symbol in the table
// *
// * @details the structure will be created and allocated by function
// * `insert...Symbol` and can be altered by user
// */
//typedef struct Symbol {
//    symType symbolType;
//
//    union {
//        // if the symbolType is VAR or VAL
//        struct {
//            varType dataType;
//            bool isParam;
//        } VarData;
//
//        // if the symbolType is FUN
//        struct {
//            varType retType;
//            int paramCount;
//            struct Symbol **params;
//        } funData;
//
//    } symParams;
//} symbol;
//
///**
// * @brief initializes an empty symbol table
// * @return a pointer to the initialized symbol table or NULL on failure
// *
// * @details An initialized table contains no symbols and no blocks
// */
//symTable *initSymTable();
//
///** @brief destructs a symbol table with all inserted symbols */
//void destructSymTable(symTable *table);
//
///** @brief clears all symbols from the symbol table*/
//void clearSymTable(symTable *table);
//
///**
// * @brief creates a new "lowest" block
// * @param table table in which the block is to be created
// * @return new number of blocks (block depth) on success, -1 on failure (memory
// * allocation error)
// */
//int beginBlock(symTable *table);
//
///**
// * @brief ends the current block
// * @param table table in which the block is to ble ended
// * @return number of blocks (block depth) remaining on success, -1 on failure
// * (there are no blocks to end)
// *
// * @details Ends the block whether it is empty or not
// */
//int endBlock(symTable *table);
//
///**
// * @brief gets the number of blocks created
// * @param table the table
// */
//int blockDepth(symTable *table);
//
///**
// * @briefs adds an symbol to the current block of the symbol table
// * @param table the table
// * @param id the name of symbol, insertVarSymbol duplicates it to the symbol
// * structure
// * @param constant whether the value is constant (whether it is val or var)
// * @param varType data type of the symbol
// * @return ptr to the created symbol on success, NULL on failure (memory
// * allocation error/id already in use)
// *
// * @detailsThe id must be unique within a block. If an id is identical as an id
// * in a "higher" block, the id from the "higher" block is masked and can be used
// * when the current block is ended.
// */
//symbol *insertVarSymbol(symTable *table, const char *id, bool constant,
//                        varType varType);
//
///**
// * @briefs adds an symbol to the current block of the symbol table
// * @param table the table
// * @param id the name of symbol, insertVarSymbol duplicates it to the symbol
// * structure
// * @param varType return data type of the function
// * @param paramCount number of function parameters
// * @param paramTypes an array on heap of parameter types.
// * @param paramNames an array on heap of strings also on heap.
// * @return ptr to the created symbol on success, NULL on failure (memory
// * allocation error/id already in use)
// *
// * @details `insertFunSymbol` will take ownership of `paramTypes` and
// * `paramNames` (responsibility for free())
// * @details The id must be unique within a block. If an id is identical as an id
// * in a "higher" block, the id from the "higher" block is masked and can be used
// * when the current block is ended.
// */
//symbol *insertFunSymbol(symTable *table, const char *id, varType retType,
//                        int paramCount, varType *paramTypes, char **paramNames);
//
///**
// * @brief finds the symbol in the whole table starting from the current block
// * and continuing outwards
// * @param table the table
// * @param id identifier of the symbol to be found
// * @return parameters of the symbol found, NULL if not found
// */
//symbol *findSymbol(symTable *table, const char *id);
//
///**
// * @brief finds the symbol only within the current block
// * @param table the table
// * @param id identifier of the symbol to be found
// * @return ptr to the symbol if found, NULL if not found
// */
//symbol *findSymbolInCurrentBlock(symTable *table, const char *id);
//




typedef struct VSymTable vSymTable;

typedef struct FSymTable fSymTable;

extern fSymTable gFunctionTable;

typedef struct VarSymbol {
    char *id;
    varType dataType;
    bool isParam;
    bool isStatic;
} varSymbol;

typedef struct FunSymbol {
    char *id;
    varType retType;
    int paramCount;
    varSymbol **params;
    vSymTable *localTable;
} funSymbol;

/**
 * @brief initializes an empty var symbol table
 * @return a pointer to the var initialized symbol table or NULL on failure
 *
 * @details An initialized table contains no symbols and no blocks
 */
vSymTable *initVSymTable();

/** @brief destructs a var symbol table with all inserted symbols */
void destructVSymTable(vSymTable *table);

/** @brief clears all symbols from the var symbol table*/
void clearVSymTable(vSymTable *table);

/**
 * @brief creates a new "lowest" block
 * @param table var symbol table in which the block is to be created
 * @return new number of blocks (block depth) on success, -1 on failure (memory
 * allocation error)
 */
int beginBlock(vSymTable *table);

/**
 * @brief ends the current block
 * @param table var symbol table in which the block is to ble ended
 * @return number of blocks (block depth) remaining on success, -1 on failure
 * (there are no blocks to end)
 *
 * @details Ends the block whether it is empty or not
 */
int endBlock(vSymTable *table);

/**
 * @brief gets the number of blocks created
 * @param table the var symbol table
 */
int blockDepth(vSymTable *table);

/**
 * @briefs adds an symbol to the current block of the symbol table
 * @param table the var symbol table
 * @param id the name of symbol, insertVarSymbol duplicates it to the symbol
 * structure
 * @param constant whether the value is constant (whether it is val or var)
 * @param varType data type of the symbol
 * @return ptr to the created symbol on success, NULL on failure (memory
 * allocation error/id already in use)
 *
 * @detailsThe id must be unique within a block. If an id is identical as an id
 * in a "higher" block, the id from the "higher" block is masked and can be used
 * when the current block is ended.
 */
varSymbol *insertVarSymbol(vSymTable *table, const char *id, bool constant,
                        varType varType);

/**
 * @brief finds the symbol in the whole table starting from the current block
 * and continuing outwards
 * @param table the var symbol table
 * @param id identifier of the symbol to be found
 * @return parameters of the symbol found, NULL if not found
 */
varSymbol *findVarSymbol(vSymTable *table, const char *id);

/**
 * @brief finds the symbol only within the current block
 * @param table the var symbol table
 * @param id identifier of the symbol to be found
 * @return ptr to the symbol if found, NULL if not found
 */
varSymbol *findVarSymbolInCurrentBlock(vSymTable *table, const char *id);


/**
 * @brief initializes the global fun symbol table as empty
 */
void initFSymTable(void);

/** @brief destructs the global fun symbol table */
void destructFSymTable(void);

/** @brief clears all symbols from the global fun symbol table */
void clearFSymTable(void);

/**
 * @briefs adds an symbol to the current block of the symbol table
 * @param id the name of function, insertVarSymbol duplicates it to the symbol
 * structure
 * @param varType return data type of the function
 * @param paramCount number of function parameters
 * @return ptr to the created symbol on success, NULL on failure (memory
 * allocation error/id already in use)
 */
funSymbol *insertFunSymbol(const char *id, varType retType, int paramCount);

/**
 * @brief finds the symbol in the whole table starting from the current block
 * and continuing outwards
 * @param table the var symbol table
 * @param id identifier of the symbol to be found
 * @return parameters of the symbol found, NULL if not found
 */
varSymbol *findFunSymbol(const char *id);




#endif

/*
 * end symtable.h
 */
