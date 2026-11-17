/** 
 * @file buildins.c
 * @brief Interface of the semantic analysis of the IFJ26 AST
 * @author Kristian Havranek, xhravrak00
 *
 * Project IFJ26 compiler
*/


#include <string.h>
#include "builtins.h"


static const BuiltinDef builtins[] = {
    {"print",       UNIT,   1,  {ANY}},
    {"readLine",    STRING, 0,  {UNIT}},
    {"readInt",     INT,    0,  {{UNIT}}},
    {"readDouble",  DOUBLE, 0,  {{UNIT}}},
    {"readBoolean", BOOL,   0,  {{UNIT}}},
    {"toStr",       STRING, 1,  {ANY}},
    {"toInt",       INT,    1,  {ANY}},
    {"strLength",   INT,    1,  {STRING}},
    {"subStr",      STRING, 3,  {STRING, INT, INT}},
    {"ord",         INT,    2,  {STRING, INT}},
    {"chr",         STRING, 1,  {INT}},
};

#define BUILTIN_COUNT (sizeof(builtins) / sizeof(builtins[0]))
/** @brief registers builtins */
int builtinsRegister(void) {
    paramData params[MAX_PARAMS];
    for (size_t i = 0; i < BUILTIN_COUNT; i++) {
        for (int j = 0; j < builtins[i].paramCount; j++) {
            params[j].id = names[p];
            params[j].dataType = builtins[i].paramTypes[j];
        }
        if (symTableInsertFun(builtins[i].name, builtins[i].retType, 
                              builtins[i].paramCount, params) != 0) {
            return -1;
        }
    }
    return 0; 
}

const BuiltinDef *builtinsFind(const char *name) {
    for (size_t i = 0; i < BUILTIN_COUNT; i++) {
        if (strcmp(builtins[i].name, name) == 0) {
            return &builtins[i];
        }
    }
    return NULL;
}