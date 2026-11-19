/**
 * @file internalSemantic.h
 * @brief Interface of the internal part of
 *        semantic analysis of the IFJ26 AST
 * @author Kristian Havranek, xhravrak00
 *
 * Project IFJ26 compiler
 */
#ifndef SEMANTIC_INTERNAL_H
#define SEMANTIC_INTERNAL_H

#include "ast.h"
#include "symtable.h"

typedef struct {
    astNode *currentFunc;
} SemCtx;

void analyzeFunction(SemCtx *ctx, astNode *func);
varType analyzeBlock(SemCtx *ctx, astNode *block);
varType analyzeStatement(SemCtx *ctx, astNode *stmt);
varType analyzeExpr(SemCtx *ctx, astNode *expr);

#endif