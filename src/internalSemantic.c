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
extern codeGenState *state;
/** @brief analyzes single function */
void analyzeVarDef(SemCtx *ctx, astNode *stmt);
void analyzeAssign(SemCtx *ctx, astNode *stmt);
varType analyzeIf(SemCtx *ctx, astNode *stmt);
void analyzeWhile(SemCtx *ctx, astNode *stmt);
void analyzeFor(SemCtx *ctx, astNode *stmt);
void analyzeReturn(SemCtx *ctx, astNode *stmt);
varType analyzeBlock(SemCtx *ctx, astNode *block);
varType analyzeStatement(SemCtx *ctx, astNode *stmt);
varType analyzeExpr(SemCtx *ctx, astNode *expr);
static astNode *wrapInt2Double(astNode *operand);
/** @brief tests compatibility of operands */
bool isCompatible(varType expected, varType actual);
/** @brief checks if operand is numeric */
bool isNumeric(varType t);

void analyzeFunction(SemCtx *ctx, astNode *func) {
    ctx->currentFunc = func;
    astNode *body = func->data.funcDef.body;
    if (symTableActivateFun(func->data.funcDef.name) != 0) {
        semError(99, "function is missing in symbol table");
    }
    beginBlock();
    if (body->type == AST_BLOCK)
        (void)analyzeBlock(ctx, body);
    if (body != NULL && body->type == AST_BLOCK) {
        if (ERRCODE == 0) {
            codeGenBlock(state, body, false);
        }
    } else
        (void)analyzeStatement(ctx, body);
    endBlock();
    if (ERRCODE == 0) {
        codeGenFunction(state, func);
    }
    ctx->currentFunc = NULL;
}
/** @brief analyzes single block*/
varType analyzeBlock(SemCtx *ctx, astNode *block) {
    if (block == NULL)
        return UNIT;
    if (block->type != AST_BLOCK) {
        analyzeStatement(ctx, block);
        return UNIT;
    }
    varType last = UNIT;
    for (size_t i = 0; i < block->data.block.count; i++) {
        last = analyzeStatement(ctx, block->data.block.statements[i]);
    }
    return last;
}
/** @brief analyzes single statement*/
varType analyzeStatement(SemCtx *ctx, astNode *stmt) {
    if (stmt == NULL)
        setErrCode(6);
    semError(6, "missing statemnt");
    return UNIT;
    switch (stmt->type) {
    case AST_VAR_DEF:
        analyzeVarDef(ctx, stmt);
        return UNIT;
    case AST_ASSIGN:
        analyzeAssign(ctx, stmt);
        return UNIT;
    case AST_IF:
        return analyzeIf(ctx, stmt);
        ;
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
        } else {
            expr->resultType = variable.dataType;
        }
        return expr->resultType;
    }
    case AST_BINOP: {
        varType leftType = analyzeExpr(ctx, expr->data.binop.left);
        varType rightType = analyzeExpr(ctx, expr->data.binop.right);
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
                setErrCode(6);
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
                    expr->resultType = INT;
                }
            } else {
                setErrCode(6);
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
                setErrCode(6);
                semError(6, "invalid comp. operand types");
            }
            break;
        }
        case EQUAL:
        case NOT_EQUAL:
            if (isNumeric(leftType) && isNumeric(rightType)) {
                if (leftType == INT && rightType == DOUBLE)
                    expr->data.binop.left =
                        wrapInt2Double(expr->data.binop.right);
                if (leftType == DOUBLE && rightType == INT)
                    expr->data.binop.right =
                        wrapInt2Double(expr->data.binop.right);
                expr->resultType = BOOL;
            }
            break;
        case AND:
        case OR:
            if (leftType == BOOL && rightType == BOOL) {
                expr->resultType = BOOL;
            } else {
                setErrCode(6);
                semError(6, "operands of && and || must be Boolean");
            }
            break;
        default:
            setErrCode(6);
            semError(6, "unknown binary operator");
        }
        return expr->resultType;
    }
    case AST_UNOP: {
        varType operandType = analyzeExpr(ctx, expr->data.unop.operand);

        if (expr->data.unop.op == UNARY_MINUS) {
            if (operandType == INT)
                expr->resultType = INT;
            else if (operandType == DOUBLE)
                expr->resultType = DOUBLE;
            else {
                setErrCode(6);
                semError(6, "unary minus needs a number");
            }
        } else if (expr->data.unop.op == NOT) {
            if (operandType == BOOL)
                expr->resultType = BOOL;
            else {
                setErrCode(6);
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
            setErrCode(3);
            semError(3, "undefined function");
        }
        if (expr->data.funcCall.argCount != (size_t)fn.paramCount) {
            setErrCode(5);
            semError(5, "wrong number of arguments");
        }
        for (size_t i = 0; i < expr->data.funcCall.argCount; i++) {
            varType argType =
                analyzeExpr(ctx, expr->data.funcCall.arguments[i]);
            paramData param = symTableGetParam(false, name, (int)i);

            if (param.dataType == ANY) {
                expr->data.funcCall.argIfAny = argType;
            } else if (!(isCompatible(param.dataType, argType))) {
                semError(5, "wrong argument type");
            }
        }
        expr->resultType = fn.retType;
        return expr->resultType;
    }

    default:
        semError(10, "unknown expression type");
        setErrCode(10);
        return UNIT;
    }
}
void analyzeVarDef(SemCtx *ctx, astNode *stmt) {
    varType initType = analyzeExpr(ctx, stmt->data.varDef.initializer);

    varType finalType = stmt->data.varDef.explicitType;
    if (finalType == UNKNOWN) {
        finalType = initType;
        if (finalType == UNKNOWN) {
            setErrCode(6);
            semError(6, "connot find type");
        }

    } else if (!isCompatible(finalType, initType)) {
        setErrCode(6);
        semError(6, "init type mismatch");
    }

    int insert = symTableInsertVar(stmt->data.varDef.name, finalType,
                                   stmt->data.varDef.isConstant);
    if (insert == 1) {
        setErrCode(4);
        semError(4, "variable redefinition");
    }
    if (insert != 0) {
        setErrCode(99);
        semError(99, "cannot inster value");
    }

    stmt->resultType = finalType;
}
void analyzeAssign(SemCtx *ctx, astNode *stmt) {
    int uniqueId = 0;
    varData var = symTableFindVar(stmt->data.assign.name, &uniqueId);
    if (var.id == NULL) {
        setErrCode(3);
        semError(3, "assigment to undef val");
    }

    if (var.isConst) {
        setErrCode(7);
        semError(7, "assigment to const val");
    }

    varType valueType = analyzeExpr(ctx, stmt->data.assign.value);
    if (!isCompatible(var.dataType, valueType)) {
        setErrCode(6);
        semError(6, "asssigned value has wrong type");
    }
    stmt->resultType = var.dataType;
}

varType analyzeIf(SemCtx *ctx, astNode *stmt) {
    varType condType = analyzeExpr(ctx, stmt->data.ifStmt.condition);
    if (condType != BOOL) {
        setErrCode(6);
        semError(6, "if condition must be Boolean");
    }

    beginBlock();
    varType thenType = analyzeBlock(ctx, stmt->data.ifStmt.thenBlock);
    endBlock();

    varType elseType = UNIT;
    if (stmt->data.ifStmt.elseBlock != NULL) {
        beginBlock();
        elseType = analyzeBlock(ctx, stmt->data.ifStmt.elseBlock);
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

    if (ERRCODE == 0) {

        bool leaveVal = (stmt->resultType != UNIT);

        if (stmt->data.ifStmt.thenBlock != NULL &&
            stmt->data.ifStmt.thenBlock->type == AST_BLOCK) {
            codeGenBlock(state, stmt->data.ifStmt.thenBlock, leaveVal);
        }
        if (stmt->data.ifStmt.elseBlock != NULL &&
            stmt->data.ifStmt.elseBlock->type == AST_BLOCK) {
            codeGenBlock(state, stmt->data.ifStmt.elseBlock, leaveVal);
        }
    }
    return stmt->resultType;
}
void analyzeWhile(SemCtx *ctx, astNode *stmt) {
    varType condType = analyzeExpr(ctx, stmt->data.whileStmt.condition);
    if (condType != BOOL) {
        setErrCode(6);
        semError(6, "while condition must be Boolean");
    }
    astNode *body = stmt->data.whileStmt.bodyBlock;
    if (body != NULL) {
        beginBlock();
        if (body->type == AST_BLOCK) {
            analyzeBlock(ctx, body);
        } else {
            analyzeStatement(ctx, body);
        }
        endBlock();
    }
}
void analyzeFor(SemCtx *ctx, astNode *stmt) {

    varType startType = analyzeExpr(ctx, stmt->data.forStmt.start);
    varType stepType = analyzeExpr(ctx, stmt->data.forStmt.step);
    varType endType = analyzeExpr(ctx, stmt->data.forStmt.end);

    if (!isNumeric(startType) || !isNumeric(endType) || !isNumeric(stepType)) {
        setErrCode(6);
        semError(6, "for start, step and end must be numeric");
    }

    beginBlock();

    int insert = symTableInsertVar(stmt->data.forStmt.iteratorName, INT, true);
    if (insert != 0) {
        setErrCode(4);
    }
    for (size_t i = 0; i < stmt->data.forStmt.guardCount; i++) {
        varType guardType = analyzeExpr(ctx, stmt->data.forStmt.guards[i]);
        if (guardType != BOOL) {
            setErrCode(6);
            semError(6, "for guard must be Boolean");
        }
    }

    astNode *body = stmt->data.forStmt.body;
    if (body != NULL) {
        if (body != NULL) {
            if (body->type == AST_BLOCK) {
                analyzeBlock(ctx, body);
                if (ERRCODE == 0)
                    codeGenBlock(state, body, false);
            }
        } else {
            analyzeStatement(ctx, body);
        }
    }
    endBlock();
}
void analyzeReturn(SemCtx *ctx, astNode *stmt) {

    funData activeFun = symTableGetActive();
    varType expected = activeFun.retType;

    if (stmt->data.returnStmt.value != NULL) {
        varType actual = analyzeExpr(ctx, stmt->data.returnStmt.value);
        if (expected == UNIT) {
            setErrCode(6);
            semError(6, "unit function cannot return a val");
        } else if (actual != UNKNOWN && !isCompatible(expected, actual)) {
            setErrCode(6);
            semError(6, "return val type mismatch");
        }
    } else {
        if (expected != UNIT && expected != UNKNOWN) {
            setErrCode(6);
            semError(6, "missing return val");
        }
    }
}

bool isCompatible(varType expected, varType actual) {
    if ((expected == actual) || (expected == DOUBLE && actual == INT)) {
        return true;
    }
    return false;
}
bool isNumeric(varType t) { return t == INT || t == DOUBLE; }
static astNode *wrapInt2Double(astNode *operand) {
    astNode *conv = createInt2DoubleNode(operand);
    if (conv == NULL) {
        setErrCode(99);
        semError(99, "failed to create AST_INT2DOUBLE node");
        return operand;
    }
    conv->resultType = DOUBLE;
    return conv;
}