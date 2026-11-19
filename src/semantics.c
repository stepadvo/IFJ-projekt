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
#include <stdio.h>
#include <stdlib.h>

codeGenState *state = NULL;

void semError(int code, const char *msg) {
    fprintf(stderr, "Semantic error (%d): %s\n", code, msg);
    exit(code);
}

int semanticAnalyze(astNode *root) {

    if (root == NULL) {
        return 0;
    }

    if (symTableInit() != 0) {
        semError(99, "cannot allocate symbol table");
    }

    if (builtinsRegister() != 0) {
        semError(99, "cannot register built-in functions");
    }
    state = codeGenInit();
    if (state == NULL) {
        semError(99, "cannot initialize code generator");
    }
    collectFunctionHeaders(root);

    SemCtx ctx = {.currentFunc = NULL};

    for (size_t i = 0; i < root->data.program.count; i++) {
        analyzeFunction(&ctx, root->data.program.functions[i]);
        if (ERRCODE != 0) {
            codeGenDestruct(state);
            return ERRCODE;
        }
    }
    if (ERRCODE == 0) {
        codeGenOutCode(state);
    }

    codeGenDestruct(state);
    state = NULL;
    return ERRCODE;
}
