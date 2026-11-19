/**
 * @file builtins.h
 * @brief registers built-in functions for the semantic analysis
 *        of the IFJ26 AST
 * @author Kristian Havranek, xhravrak00
 *
 * Project IFJ26 compiler
 */
#ifndef BUILTINS_H
#define BUILTINS_H

#include "symtable.h"
#include <stdbool.h>

#define MAX_PARAMS 3

typedef struct {
    const char *name;
    varType retType;
    int paramCount;
    varType paramTypes[MAX_PARAMS];
} BuiltinDef;

int builtinsRegister(void);

const BuiltinDef *builtinsFind(const char *name);

#endif