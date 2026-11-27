/**
 * @file semantic.c
 * @brief Interface of the semantic analysis of the IFJ26 AST
 * @author Kristian Havranek, xhravrak00
 *
 * Project IFJ26 compiler
 */
#include "semantics.h"
#include "ast.h"
#include "builtins.h"
#include "collectFunctionHeaders.h"
#include "generator.h"
#include "internalSemantic.h"
#include "symtable.h"
#include <stdlib.h>

codeGenState *state = NULL;
/** @brief sets Error code and writes error msg in stderr */
void semError(int code, const char *msg) {
    setErrCode(code);
    logErr(code, 0, 0, msg);
}
/** @brief analysis of semantic aspect of code */
int semanticAnalyze(astNode *root) {

    if (root == NULL) {
        return 0;
    }

    if (symTableInit() != 0) {
        semError(99, "cannot allocate symbol table");
        return ERRCODE;
    }

    if (builtinsRegister() != 0) {
        semError(99, "cannot register built-in functions");
        symTableDestruct();
        return ERRCODE;
    }
    state = codeGenInit();
    if (state == NULL) {
        semError(99, "cannot initialize code generator");
        symTableDestruct();
        return ERRCODE;
    }
    collectFunctionHeaders(root);
    if (ERRCODE != 0) {
        codeGenDestruct(state);
        state = NULL;
        symTableDestruct();
        return ERRCODE;
    }

    SemCtx ctx = {.currentFunc = NULL, .root = root, .inferState = NULL};
    ctx.inferState = calloc(root->data.program.count + 1, sizeof(InferState));
    if (ctx.inferState == NULL) {
        semError(99, "out of memory");
        codeGenDestruct(state);
        return ERRCODE;
    }

    for (size_t i = 0; i < root->data.program.count; i++) {

        if (ERRCODE != 0) {
            codeGenDestruct(state);
            free(ctx.inferState);
            symTableDestruct();
            return ERRCODE;
        }
        analyzeFunction(&ctx, root->data.program.functions[i]);
    }
    if (ERRCODE == 0) {
        codeGenOutCode(state);
    }
    free(ctx.inferState);
    symTableDestruct();
    codeGenDestruct(state);
    state = NULL;
    return ERRCODE;
}
