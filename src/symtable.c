/**
 * @file src_example.h
 * @brief example header
 * @author Name Surname, xlogin@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include "symtable.h"

/**
 * @brief private symbol data structure
 *
 * contains both the symbol data accesible to the user and internal data, that
 * the user should not change
 */
typedef struct SymbolPrivate {
    symbol publicData;
    size_t blockDepth;
    size_t dataMasked; // the data that this entry masked in the BST
} symbolPrivate;

symTable *initSymTable(void) { return NULL; }

int destructSymTable(symTable *table) {
    (void)table;
    return 0;
}

/*
 * end src_example.h
 */
