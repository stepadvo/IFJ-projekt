/** 
 * @file collectFunctionHeaders.c
 * @brief Implementation of the function header collection for 
 *        the semantic analysis of the IFJ 26 AST
 * @author Kristian Havranek, xhravrak00
 *
 * Project IFJ26 compiler 
*/
#include "symtable.h"
#include "ast.h"
#include "collectFunctionHeaders.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "errcode.h"


/** @brief checks for duplicate params */
static bool hasDuplicateParams(const astFuncParam *params, size_t count) {
    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (strcmp(params[i].name, params[j].name) == 0) {
                return true;
            }
        }
    }
    return false;
}
/** @brief builds parameter data list */
static paramData *buildParamData(const astFuncParam *params, size_t count) {
    paramData *paramList = malloc(count * sizeof(paramData));
    if (!paramList) {
        return NULL; // alloc failed
    }

    for (size_t i = 0; i < count; i++) {
        paramList[i] = (paramData){ .id = params[i].name, .dataType = params[i].type };
    }

    return paramList;
}
/** @brief checks for @main function */
static void checkMain(const astNode *root) {
    size_t mainCount = 0;
    const astNode *mainFunc = NULL;
    for (size_t i = 0; i < root->data.program.count; i++) {
        const astNode *func = root->data.program.functions[i];
        if (func->data.funcDef.isMain) {
            mainCount++;
            mainFunc = func;
        } 
    }

    if (mainCount == 0) semError(3, "missing @main function");
    if (mainCount > 1) semError(4, "multiple @main functions");
    if (mainFunc->data.funcDef.paramCount != 0)
        semError(5, "@main function must have no parameters");
    if (mainFunc->data.funcDef.explicitReturnType != UNKNOWN &&
        mainFunc->data.funcDef.explicitReturnType != UNIT)
        semError(6, "@main must return UNIT");
}
/** @brief collects all function headers */
void collectFunctionHeaders(astNode *root) {
    if (root == NULL) semError(99, "no AST");

    for (size_t i = 0; i < root->data.program.count; i++) {
        const astNode *func = root->data.program.functions[i];
        const size_t paramCount = func->data.funcDef.paramCount;

        if (hasDuplicateParams(func->data.funcDef.params, paramCount))
            semError(4, "duplicate parameter names in function");
        
        paramData *params = NULL;
        if (paramCount > 0) {
            params = buildParamData(func->data.funcDef.params, paramCount);
            if (params == NULL) semError(99, "out of memory");
        }
    }
    
    int result = symTableInsertFunctionHeaders(func->data.funcDef.name, 
                                               func->data.funcDef.explicitReturnType, 
                                               (int)paramCount, params);
    
    free(params);

    if (result == 1)
        semError(4, "duplicate function name");
    if (result != 0)
        semError(99, "cannot insert function into symbol table");

    checkMain(root);
}