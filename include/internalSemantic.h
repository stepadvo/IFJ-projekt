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
typedef enum { INF_NONE, INF_RUNNING, INF_DONE } InferState;
typedef struct {
    astNode *currentFunc;
    astNode *root;
    InferState *inferState;
} SemCtx;
void analyzeFunction(SemCtx *ctx, astNode *func);
bool isCompatible(varType expected, varType actual);
bool isNumeric(varType t);
void analyzeVarDef(SemCtx *ctx, astNode *stmt);
void analyzeAssign(SemCtx *ctx, astNode *stmt);
varType analyzeIf(SemCtx *ctx, astNode *stmt);
void analyzeWhile(SemCtx *ctx, astNode *stmt);
void analyzeFor(SemCtx *ctx, astNode *stmt);
void analyzeReturn(SemCtx *ctx, astNode *stmt);
varType analyzeBlock(SemCtx *ctx, astNode *block);
varType analyzeStatement(SemCtx *ctx, astNode *stmt);
varType analyzeExpr(SemCtx *ctx, astNode *expr);
#endif