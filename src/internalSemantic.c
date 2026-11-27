/**
 * @file internalSemantic.c
 * @brief Inicialization of the internal semantic analysis of the IFJ26 AST
 * @author Kristian Havranek, xhravrak00
 *
 * Project IFJ26 compiler
 */
#include "internalSemantic.h"
#include "ast.h"
#include "compiler.h"
#include "generator.h"
#include "semantics.h"
#include "symtable.h"
#include <string.h>

static int findFuncIndex(SemCtx *ctx, const char *name);
static bool endReturn(astNode *node);
extern codeGenState *state;
static astNode *wrapInt2Double(astNode *operand);
/** @brief abalyzes a signle funtion */
void analyzeFunction(SemCtx *ctx, astNode *func) {
    const char *name = func->data.funcDef.name;

    int idx = findFuncIndex(ctx, name);
    if (idx < 0) {
        semError(99, "function not found in program");
        return;
    }
    if (ctx->inferState[idx] != INF_NONE) {
        return;
    }
    ctx->inferState[idx] = INF_RUNNING;

    astNode *prevFunc = ctx->currentFunc;
    ctx->currentFunc = func;

    if (symTableActivateFun(func->data.funcDef.name) != 0) {
        semError(99, "function is missing in symbol table");
        ctx->currentFunc = prevFunc;
        return;
    }
    astNode *body = func->data.funcDef.body;
    if (body->type != AST_BLOCK) {
        astNode *blk = createBlockNode();
        addStatementToBlock(blk, body);
        func->data.funcDef.body = blk;
        body = blk;
    }
    beginBlock();
    varType bodyType = analyzeBlock(ctx, body);
    if (ERRCODE == 0) {
        varType ret = symTableFindFun(name).retType;
        bool returns = endReturn(body);

        if (ret == UNKNOWN) {
            ret = bodyType;
            if (symTableSetNewRetType(true, NULL, ret) != 0) {
                semError(99, "cannot set return type");
            }
        } else if (ret != UNIT && !returns) {
            if (!isCompatible(ret, bodyType))
                semError(6, "function body type does not match return type");
            else if (ret == DOUBLE && bodyType == INT &&
                     body->data.block.count > 0) {
                size_t last = body->data.block.count - 1;
                body->data.block.statements[last] =
                    wrapInt2Double(body->data.block.statements[last]);
            }
        }

        if (ERRCODE == 0) {
            bool leave = (ret != UNIT) && !returns;
            codeGenBlock(state, body, leave);
        }
    }

    endBlock();

    if (ERRCODE == 0) {
        codeGenFunction(state, func);
    }
    ctx->inferState[idx] = INF_DONE;
    ctx->currentFunc = prevFunc;
}
/** @brief analyzes single block*/
varType analyzeBlock(SemCtx *ctx, astNode *block) {
    if (block == NULL)
        return UNIT;
    if (block->type != AST_BLOCK) {
        return analyzeStatement(ctx, block);
    }
    varType last = UNIT;
    for (size_t i = 0; i < block->data.block.count; i++) {
        if (ERRCODE != 0)
            break;
        last = analyzeStatement(ctx, block->data.block.statements[i]);
    }
    return last;
}
/** @brief analyzes single statement*/
varType analyzeStatement(SemCtx *ctx, astNode *stmt) {
    if (stmt == NULL) {
        semError(6, "missing statement");
        return UNIT;
    }
    switch (stmt->type) {
    case AST_VAR_DEF:
        analyzeVarDef(ctx, stmt);
        return UNIT;
    case AST_ASSIGN:
        analyzeAssign(ctx, stmt);
        return UNIT;
    case AST_IF:
        return analyzeIf(ctx, stmt);
    case AST_WHILE:
        analyzeWhile(ctx, stmt);
        return UNIT;
    case AST_FOR:
        analyzeFor(ctx, stmt);
        return UNIT;
    case AST_RETURN:
        analyzeReturn(ctx, stmt);
        return UNIT;
    default:
        return analyzeExpr(ctx, stmt);
    }
}
/** @brief analyzes single expression*/
varType analyzeExpr(SemCtx *ctx, astNode *expr) {
    if (expr == NULL)
        return UNIT;

    switch (expr->type) {

    case AST_LITERAL: {
        switch (expr->data.literal.litType) {
        case TOKEN_INT_LITERAL:
            expr->resultType = INT;
            break;
        case TOKEN_DOUBLE_LITERAL:
            expr->resultType = DOUBLE;
            break;
        case TOKEN_STRING_LITERAL:
            expr->resultType = STRING;
            break;
        case TOKEN_BOOL_LITERAL:
            expr->resultType = BOOL;
            break;
        case TOKEN_LPAREN:
            expr->resultType = UNIT;
            break;
        default:
            semError(99, "unknown literal");
            expr->resultType = UNKNOWN;
            break;
        }
        return expr->resultType;
    }

    case AST_IDENTIFIER: {
        const char *name = expr->data.identifier.name;
        int uniqueId = 1;
        varData variable = symTableFindVar(name, &uniqueId);
        if (variable.id == NULL) {
            semError(3, "undeclared ID");
            expr->resultType = UNKNOWN;
            return UNKNOWN;
        } else {
            expr->resultType = variable.dataType;
        }
        return expr->resultType;
    }
    case AST_BINOP: {
        varType leftType = analyzeExpr(ctx, expr->data.binop.left);
        varType rightType = analyzeExpr(ctx, expr->data.binop.right);
        if (leftType == UNKNOWN || rightType == UNKNOWN) {
            expr->resultType = UNKNOWN;
            return UNKNOWN;
        }
        switch (expr->data.binop.op) {
        case PLUS: {
            if (leftType == STRING && rightType == STRING) {
                expr->data.binop.op = CONCATENATION;
                expr->resultType = STRING;
            } else if (rightType == INT && leftType == DOUBLE) {
                expr->data.binop.right = wrapInt2Double(expr->data.binop.right);
                expr->resultType = DOUBLE;
            } else if (rightType == DOUBLE && leftType == INT) {
                expr->data.binop.left = wrapInt2Double(expr->data.binop.left);
                expr->resultType = DOUBLE;
            } else if (rightType == DOUBLE && leftType == DOUBLE) {
                expr->resultType = DOUBLE;
            } else if (rightType == INT && leftType == INT) {
                expr->resultType = INT;
            } else {
                semError(6, "invalid bin. operand types");
            }
            break;
        }
        case MINUS:
        case DIVIDE:
        case MULTIPLY: {
            if (rightType == INT && leftType == DOUBLE) {
                expr->data.binop.right = wrapInt2Double(expr->data.binop.right);
                expr->resultType = DOUBLE;
            } else if (rightType == DOUBLE && leftType == INT) {
                expr->data.binop.left = wrapInt2Double(expr->data.binop.left);
                expr->resultType = DOUBLE;
            } else if (rightType == DOUBLE && leftType == DOUBLE) {
                expr->resultType = DOUBLE;
            } else if (rightType == INT && leftType == INT) {
                if (expr->data.binop.op == DIVIDE) {
                    expr->data.binop.op = I_DIVIDE;
                }
                expr->resultType = INT;
            } else {
                semError(6, "invalid bin. operand types");
            }
            break;
        }

        case LESS:
        case LESS_EQUAL:
        case GREATER:
        case GREATER_EQUAL: {
            if (isNumeric(leftType) && isNumeric(rightType)) {
                if (leftType == INT && rightType == DOUBLE)
                    expr->data.binop.left =
                        wrapInt2Double(expr->data.binop.left);
                if (leftType == DOUBLE && rightType == INT)
                    expr->data.binop.right =
                        wrapInt2Double(expr->data.binop.right);
                expr->resultType = BOOL;
            } else if (leftType == STRING && rightType == STRING) {
                expr->resultType = BOOL;
            } else {
                semError(6, "invalid comp. operand types");
            }
            break;
        }
        case EQUAL:
        case NOT_EQUAL:
            if (leftType == rightType && leftType != UNKNOWN) {
                expr->resultType = BOOL;
            } else if (isNumeric(leftType) && isNumeric(rightType)) {
                if (leftType == INT && rightType == DOUBLE)
                    expr->data.binop.left =
                        wrapInt2Double(expr->data.binop.left);
                if (leftType == DOUBLE && rightType == INT)
                    expr->data.binop.right =
                        wrapInt2Double(expr->data.binop.right);
                expr->resultType = BOOL;
            } else {
                semError(6, "comparing requires numericals");
            }
            break;
        case AND:
        case OR:
            if (leftType == BOOL && rightType == BOOL) {
                expr->resultType = BOOL;
            } else {
                semError(6, "operands of && and || must be Boolean");
            }
            break;
        default:
            semError(6, "unknown binary operator");
        }
        return expr->resultType;
    }
    case AST_UNOP: {
        varType operandType = analyzeExpr(ctx, expr->data.unop.operand);
        if (operandType == UNKNOWN) {
            expr->resultType = UNKNOWN;
            return UNKNOWN;
        }

        if (expr->data.unop.op == UNARY_MINUS) {
            if (operandType == INT)
                expr->resultType = INT;
            else if (operandType == DOUBLE)
                expr->resultType = DOUBLE;
            else {
                semError(6, "unary minus needs a number");
            }
        } else if (expr->data.unop.op == NOT) {
            if (operandType == BOOL)
                expr->resultType = BOOL;
            else {
                semError(6, "! needs Boolean");
            }
        } else {
            semError(10, "unknown unary operator");
        }
        return expr->resultType;
    }
    case AST_FUNC_CALL: {
        const char *name = expr->data.funcCall.funcName;

        funData fn = symTableFindFun(name);
        if (fn.id == NULL) {
            semError(3, "undefined function");
            expr->resultType = UNKNOWN;
            return UNKNOWN;
        }
        if (fn.retType == UNKNOWN) {
            int idx = findFuncIndex(ctx, name);
            if (idx >= 0) {
                if (ctx->inferState[idx] == INF_RUNNING) {
                    semError(6, "recursion requires explicit return type");
                    expr->resultType = UNKNOWN;
                    return UNKNOWN;
                }
                const char *outer = symTableGetActive().id;
                analyzeFunction(ctx, ctx->root->data.program.functions[idx]);
                symTableActivateFun(outer);
                fn = symTableFindFun(name);
                if (fn.retType == UNKNOWN) {
                    expr->resultType = UNKNOWN;
                    return UNKNOWN;
                }
            }
        }
        if (expr->data.funcCall.argCount != (size_t)fn.paramCount) {
            semError(5, "wrong number of arguments");
            expr->resultType = UNKNOWN;
            return UNKNOWN;
        }
        for (size_t i = 0; i < expr->data.funcCall.argCount; i++) {
            varType argType =
                analyzeExpr(ctx, expr->data.funcCall.arguments[i]);
            if (argType == UNKNOWN) {
                expr->resultType = UNKNOWN;
                return UNKNOWN;
            }
            paramData param = symTableGetParam(false, name, (int)i);

            if (param.dataType == ANY) {
                expr->data.funcCall.argIfAny = argType;
            } else if (!(isCompatible(param.dataType, argType))) {
                semError(5, "wrong argument type");
                expr->resultType = UNKNOWN;
                return UNKNOWN;
            } else if (param.dataType == DOUBLE && argType == INT) {
                expr->data.funcCall.arguments[i] =
                    wrapInt2Double(expr->data.funcCall.arguments[i]);
            }
        }
        expr->resultType = fn.retType;
        return expr->resultType;
    }
    case AST_IF:
        return analyzeIf(ctx, expr);
    default:
        semError(10, "unknown expression type");
        expr->resultType = UNKNOWN;
        return UNKNOWN;
    }
}
/** @brief analyzes variable definition */
void analyzeVarDef(SemCtx *ctx, astNode *stmt) {
    varType initType = analyzeExpr(ctx, stmt->data.varDef.initializer);

    if (initType == UNKNOWN) {
        stmt->resultType = UNKNOWN;
        return;
    }

    varType finalType = stmt->data.varDef.explicitType;
    if (finalType == UNKNOWN) {
        finalType = initType;

    } else if (!isCompatible(finalType, initType)) {
        semError(6, "init type mismatch");
        stmt->resultType = UNKNOWN;
        return;
    }

    if (finalType == ANY) {
        semError(6, "cannot infer type");
        stmt->resultType = UNKNOWN;
        return;
    }

    int insert = symTableInsertVar(stmt->data.varDef.name, finalType,
                                   stmt->data.varDef.isConstant);
    if (insert == 1) {
        semError(4, "variable redefinition");
        stmt->resultType = UNKNOWN;
        return;
    } else if (insert != 0) {
        semError(99, "cannot insert value");
        stmt->resultType = UNKNOWN;
        return;
    }

    stmt->resultType = finalType;
}
/** @brief analyzes assign */
void analyzeAssign(SemCtx *ctx, astNode *stmt) {
    int uniqueId = 0;
    varData var = symTableFindVar(stmt->data.assign.name, &uniqueId);
    if (var.id == NULL) {
        semError(3, "assignment to undef var");
        return;
    }

    if (var.isConst) {
        semError(7, "assigment to const var");
        return;
    }

    varType valueType = analyzeExpr(ctx, stmt->data.assign.value);
    if (valueType == UNKNOWN) {
        return;
    }
    if (!isCompatible(var.dataType, valueType)) {
        semError(6, "assigned value has wrong type");
        return;
    }
    if (var.dataType == DOUBLE && valueType == INT) {
        stmt->data.assign.value = wrapInt2Double(stmt->data.assign.value);
    }
    stmt->resultType = var.dataType;
}
/** @brief analyzes if statements */
varType analyzeIf(SemCtx *ctx, astNode *stmt) {
    varType condType = analyzeExpr(ctx, stmt->data.ifStmt.condition);
    if (condType != BOOL && condType != UNKNOWN) {
        semError(6, "if condition must be Boolean");
    }
    astNode *thenBlock = stmt->data.ifStmt.thenBlock;
    astNode *elseBlock = stmt->data.ifStmt.elseBlock;
    beginBlock();
    varType thenType = analyzeBlock(ctx, stmt->data.ifStmt.thenBlock);
    if (ERRCODE == 0 && thenBlock->type == AST_BLOCK)
        codeGenBlock(state, thenBlock, thenType != UNIT);
    endBlock();

    varType elseType = UNIT;
    if (elseBlock != NULL) {
        beginBlock();
        elseType = analyzeBlock(ctx, elseBlock);
        if (ERRCODE == 0 && elseBlock->type == AST_BLOCK)
            codeGenBlock(state, elseBlock, elseType != UNIT);
        endBlock();
    }

    if (stmt->data.ifStmt.elseBlock == NULL) {
        stmt->resultType = UNIT;
    } else if (thenType == elseType) {
        stmt->resultType = thenType;
    } else if (isNumeric(thenType) && isNumeric(elseType)) {
        stmt->resultType = DOUBLE;
    } else {
        stmt->resultType = UNIT;
    }
    return stmt->resultType;
}
/** @brief analyzes while loops */
void analyzeWhile(SemCtx *ctx, astNode *stmt) {
    varType condType = analyzeExpr(ctx, stmt->data.whileStmt.condition);
    if (condType != BOOL && condType != UNKNOWN) {
        semError(6, "while condition must be Boolean");
    }
    astNode *body = stmt->data.whileStmt.bodyBlock;
    if (body == NULL) {
        return;
    }

    beginBlock();
    analyzeBlock(ctx, body);
    if (ERRCODE == 0 && body->type == AST_BLOCK) {
        codeGenBlock(state, body, false);
    }
    endBlock();
}
/** @brief analyzes for loops */
void analyzeFor(SemCtx *ctx, astNode *stmt) {

    varType startType = analyzeExpr(ctx, stmt->data.forStmt.start);
    varType endType = analyzeExpr(ctx, stmt->data.forStmt.end);

    if (stmt->data.forStmt.step == NULL) {
        Token t = {.type = TOKEN_INT_LITERAL};
        t.attribute.intValue = 1;
        stmt->data.forStmt.step = createLiteralNode(&t);
    }
    if (stmt->data.forStmt.step == NULL) {
        semError(99, "cannot create step literal");
        return;
    }
    astNode *step = stmt->data.forStmt.step;
    varType stepType = analyzeExpr(ctx, stmt->data.forStmt.step);

    if (startType != UNKNOWN && startType != INT)
        semError(6, "for start must be Int");
    if (endType != UNKNOWN && endType != INT)
        semError(6, "for end must be Int");
    if (stepType != UNKNOWN && stepType != INT)
        semError(6, "for step must be Int");

    if (step->type == AST_LITERAL &&
        step->data.literal.litType == TOKEN_INT_LITERAL &&
        step->data.literal.attribute.intValue == 0) {
        semError(10, "for step must not be zero");
    }

    beginBlock();

    int insert = symTableInsertVar(stmt->data.forStmt.iteratorName, INT, true);
    if (insert != 0) {
        semError(insert == 1 ? 4 : 99, "cannot insert variable");
    }
    for (size_t i = 0; i < stmt->data.forStmt.guardCount; i++) {
        varType guardType = analyzeExpr(ctx, stmt->data.forStmt.guards[i]);
        if (guardType != BOOL && guardType != UNKNOWN) {
            semError(6, "for guard must be Boolean");
        }
    }

    astNode *body = stmt->data.forStmt.body;
    if (body != NULL) {
        analyzeBlock(ctx, body);
        if (ERRCODE == 0)
            codeGenBlock(state, body, false);
    }

    endBlock();
}
void analyzeReturn(SemCtx *ctx, astNode *stmt) {
    if (stmt == NULL) {
        semError(6, "missing return statement");
        return;
    }

    funData activeFun = symTableGetActive();
    varType expected = activeFun.retType;

    if (expected == UNKNOWN) {
        semError(6, "return requires explicit return type");
        return;
    }

    if (stmt->data.returnStmt.value != NULL) {
        varType actual = analyzeExpr(ctx, stmt->data.returnStmt.value);
        if (actual == UNKNOWN) {
            return;
        }
        if (expected != UNIT) {
            if (!isCompatible(expected, actual)) {
                semError(6, "return val type mismatch");
                return;
            }
            if (expected == DOUBLE && actual == INT) {
                stmt->data.returnStmt.value =
                    wrapInt2Double(stmt->data.returnStmt.value);
            }
        }

        stmt->resultType = expected;
    } else {
        if (expected != UNIT && expected != UNKNOWN) {
            semError(6, "missing return val");
            return;
        }
        stmt->resultType = UNIT;
    }
}
/** @brief checks of 2 types are compatible */
bool isCompatible(varType expected, varType actual) {
    if ((expected == actual) || (expected == DOUBLE && actual == INT)) {
        return true;
    }
    return false;
}
/** @brief checks if number is numeric */
bool isNumeric(varType t) { return t == INT || t == DOUBLE; }
static astNode *wrapInt2Double(astNode *operand) {
    astNode *conv = createInt2DoubleNode(operand);
    if (conv == NULL) {

        semError(99, "failed to create AST_INT2DOUBLE node");
        return operand;
    }
    conv->resultType = DOUBLE;
    return conv;
}
/** @brief index of func in program sorted by name, -1 if nonexistent */
static int findFuncIndex(SemCtx *ctx, const char *name) {
    for (size_t i = 0; i < ctx->root->data.program.count; i++) {
        if (strcmp(ctx->root->data.program.functions[i]->data.funcDef.name,
                   name) == 0) {
            return (int)i;
        }
    }
    return -1;
}
/** @brief checks if all blocks in func end with return */
static bool endReturn(astNode *node) {
    if (node == NULL)
        return false;
    if (node->type == AST_RETURN)
        return true;
    if (node->type == AST_BLOCK) {
        if (node->data.block.count == 0)
            return false;
        return endReturn(
            node->data.block.statements[node->data.block.count - 1]);
    }
    if (node->type == AST_IF) {
        return node->data.ifStmt.elseBlock != NULL &&
               endReturn(node->data.ifStmt.thenBlock) &&
               endReturn(node->data.ifStmt.elseBlock);
    }
    return false;
}