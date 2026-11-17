/** 
 * @file semantic.c
 * @brief Interface of the semantic analysis of the IFJ26 AST
 * @author Kristian Havranek, xhravrak00
 *
 * Project IFJ26 compiler
*/
#include <stdio.h>
#include <stdlib.h>
#include "semantics.h"
#include "ast.h"
#include "symtable.h"
#include "builtins.h"
#include "collectFunctionHeaders.h"

typedef struct {
    astNode *currentFunc;
} SemCtx;

static void semError(int code, const char *msg)
{
    fprintf(stderr, "Semantic error (%d): %s\n", code, msg);
    exit(code);
}
static void analyzeFunction(SemCtx *ctx, astNode *func);
static void analyzeBlock(SemCtx *ctx, astNode *block);
static void analyzeStatement(SemCtx *ctx, astNode *stmt);
static varType analyzeExpr(SemCtx *ctx, astNode *expr);

static void registerBuiltins(SemCtx *ctx);
static  void collectFunctionHeaders(SemCtx *ctx, astNode *root);
/** @brief analyze one function def */
static void analyzeFunction(SemCtx *ctx, astNode *func)
{

    ctx->currentFunc = func;
    // TODO
    ctx->currentFunc = NULL;
    
}

/** @brief analyzes all statements of a block */
static void analyzeBlock(SemCtx *ctx, astNode *block)
{

    (void)ctx;
    (void)block; // TODO

}

/** @brief analyzes statement */
static void analyzeStatement(SemCtx *ctx, astNode *stmt)
{

    (void)ctx;
    (void)stmt; // TODO

}

/** @brief analyzes expression and returns its type*/
static varType analyzeExpr(SemCtx *ctx, astNode *expr)
{

    (void)ctx;
    (void)expr; // TODO
    return UNKNOWN;

}

int semanticAnalyze(astNode *root)
{

    SemCtx ctx = {0};

    if (symTableInit() != 0) {
        semError(99, "cannot allocate symbol table");
    }

    if (builtinsRegister() != 0) {
        semError(99, "cannot register built-in functions");
    }
    collectFunctionHeaders(root);
    
    for (size_t i = 0; i < root->data.program.count; i++)
    {
        analyzeFunction(&ctx, root->data.program.functions[i]);
    }

    return 0;
}

