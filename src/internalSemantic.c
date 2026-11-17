#include "ast.h"
#include "symtable.h"

/** @brief analyzes single function */
void analyzeFunction(astNode *func)
{
    ctx->currentFunc = func;
    astNode *body = func->data.funcDef.body;
    if(symTableActivateFun(func->data.funcDef.name) != 0) {
        semError(99, "function is missing in symbol table");
    }
    beginBlock();
    if (body->type == AST_BLOCK)
        analyzeBlock(ctx, body);
    else
        analyzeStatement(ctx, body);
    endBlock();

    ctx ->currentFunc = NULL;
}
void analyzeBlock(SemCtx *ctx, astNode *block)
{
    if (block == NULL)
        return;
    if (block->type != AST_BLOCK) {
        analyzeStatement(ctx, block);
        return;
    }
    for (size_t i = 0; i < block->data.block.count; i++) {
        analyzeStatement(ctx, block->data.block.statements[i]);
    }
}
void analyzeStatement(SemCtx *ctx, astNode *stmt)
{
    if (stmt == NULL)
        return;
    switch (stmt->type) {
        case VAR_DEF:
            analyzeVarDef(ctx, stmt);
            break;
        case AST_ASSIGN:
            analyzeAssign(ctx, stmt);
            break;
        case AST_IF:
            analyzeIf(ctx, stmt);
            break;
        case AST_WHILE:
            analyzeWhile(ctx, stmt);
            break;
        case AST_FOR:
            analyzeFor(ctx, stmt);
            break;
        case AST_RETURN:
            analyzeReturn(ctx, stmt);
            break;
        default:
            analyzeExpression(ctx, stmt);
            break;
    }
}