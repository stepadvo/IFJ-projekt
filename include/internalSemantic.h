#ifndef SEMANTIC_INTERNAL_H
#define SEMANTIC_INTERNAL_H

#include "ast.h"
#include "symtable.h"

typedef struct {
    astNode *currentFunc;
} SemCtx;

void analyzeFunction(SemCtx *ctx, astNode *func);
void analyzeBlock(SemCtx *ctx, astNode *block);
void analyzeStatement(SemCtx *ctx, astNode *stmt);
varType analyzeExpr(SemCtx *ctx, astNode *expr);

#endif