/**
 * @file collectFunctionHeaders.c
 * @brief Implementation of the function header collection for
 *        the semantic analysis of the IFJ 26 AST
 * @author Kristian Havranek, xhravrak00
 *
 * Project IFJ26 compiler
 */
#include "collectFunctionHeaders.h"
#include "ast.h"
#include "compiler.h"
#include "semantics.h"
#include "symtable.h"
#include <stdlib.h>
#include <string.h>

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
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        paramList[i] =
            (paramData){.id = params[i].name, .dataType = params[i].type};
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

    if (mainCount == 0) {
        setErrCode(3);
        semError(3, "@main not found");
        return;
    }
    if (mainCount > 1) {
        setErrCode(4);
        semError(4, "multiple @mains found");
        return;
    }

    if (mainFunc->data.funcDef.paramCount != 0) {
        setErrCode(5);
        semError(5, "@main has a parameter");
    }

    if (mainFunc->data.funcDef.explicitReturnType != UNKNOWN &&
        mainFunc->data.funcDef.explicitReturnType != UNIT) {
        setErrCode(6);
        semError(6, "@main has invalid return type");
    }
}
/** @brief collects all function headers */
void collectFunctionHeaders(astNode *root) {
    if (root == NULL) {
        setErrCode(99);
        semError(99, "AST root is NULL");
        return;
    }
    for (size_t i = 0; i < root->data.program.count; i++) {
        const astNode *func = root->data.program.functions[i];
        if (func == NULL) {
            setErrCode(99);
            semError(99, "NULL function node in AST");
            return;
        }

        const size_t paramCount = func->data.funcDef.paramCount;

        if (hasDuplicateParams(func->data.funcDef.params, paramCount)) {
            setErrCode(4);
            semError(4, "duplicate func params");
        }
        paramData *params = NULL;

        if (paramCount > 0) {
            params = buildParamData(func->data.funcDef.params, paramCount);
            if (params == NULL) {
                setErrCode(99);
                semError(99, "failed to allocate parameter list");
                return;
            }
        }

        int result = symTableInsertFun(func->data.funcDef.name,
                                       func->data.funcDef.explicitReturnType,
                                       (int)paramCount, params);

        free(params);

        if (result == 1) {
            setErrCode(4);
            semError(4, "function redefinition");
        } else if (result != 0) {
            setErrCode(99);
            semError(99, "symtable instert function failed");
        }
    }

    checkMain(root);
}