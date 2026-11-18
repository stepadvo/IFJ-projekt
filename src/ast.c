/**
 * @file ast.c
 * @brief AST node creation and cleanup.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 */

#include "ast.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void checkMalloc(void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "Internal error: Memory allocation failed\n");
        exit(99);
    }
}

static astNode *newNode(astNodeType type) {
    astNode *node = calloc(1, sizeof(*node));
    checkMalloc(node);
    node->type = type;
    node->resultType = UNKNOWN;
    return node;
}

static void appendNode(astNode ***items, size_t *count, size_t *capacity,
                       astNode *node) {
    if (*count == *capacity) {
        size_t newCapacity = *capacity ? *capacity * 2 : 4;
        astNode **newItems = realloc(*items, newCapacity * sizeof(**items));
        checkMalloc(newItems);
        *items = newItems;
        *capacity = newCapacity;
    }
    (*items)[(*count)++] = node;
}

astNode *createProgramNode(void) { return newNode(AST_PROGRAM); }

int addFunctionToProgram(astNode *programNode, astNode *funcDef) {
    if (programNode == NULL || programNode->type != AST_PROGRAM ||
        funcDef == NULL) {
        return -1;
    }

    appendNode(&programNode->data.program.functions,
               &programNode->data.program.count,
               &programNode->data.program.capacity, funcDef);
    return 0;
}

astNode *createFuncDefNode(char *name, bool isMain, astFuncParam *params,
                           size_t paramCount, astNode *body) {
    if (name == NULL || body == NULL || (paramCount > 0 && params == NULL))
        return NULL;

    astNode *node = newNode(AST_FUNC_DEF);
    node->data.funcDef.name = name;
    node->data.funcDef.isMain = isMain;
    node->data.funcDef.explicitReturnType = UNKNOWN;
    node->data.funcDef.params = params;
    node->data.funcDef.paramCount = paramCount;
    node->data.funcDef.body = body;
    node->data.funcDef.alreadyGenerated = false;
    return node;
}

astNode *createBlockNode(void) {
    astNode *node = newNode(AST_BLOCK);
    node->data.block.bufferNum = -1;
    return node;
}

int addStatementToBlock(astNode *blockNode, astNode *statement) {
    if (blockNode == NULL || blockNode->type != AST_BLOCK ||
        statement == NULL) {
        return -1;
    }

    appendNode(&blockNode->data.block.statements, &blockNode->data.block.count,
               &blockNode->data.block.capacity, statement);
    return 0;
}

astNode *createVarDefNode(char *name, bool isConstant, varType explicitType,
                          astNode *initializer) {
    if (name == NULL || initializer == NULL)
        return NULL;

    astNode *node = newNode(AST_VAR_DEF);
    node->data.varDef.name = name;
    node->data.varDef.isConstant = isConstant;
    node->data.varDef.explicitType = explicitType;
    node->data.varDef.initializer = initializer;
    return node;
}

astNode *createAssignNode(char *name, astNode *value) {
    if (name == NULL || value == NULL)
        return NULL;

    astNode *node = newNode(AST_ASSIGN);
    node->data.assign.name = name;
    node->data.assign.value = value;
    return node;
}

astNode *createReturnNode(astNode *value) {
    astNode *node = newNode(AST_RETURN);
    node->data.returnStmt.value = value;
    return node;
}

astNode *createIfNode(astNode *condition, astNode *thenBlock,
                      astNode *elseBlock) {
    if (condition == NULL || thenBlock == NULL)
        return NULL;

    astNode *node = newNode(AST_IF);
    node->data.ifStmt.condition = condition;
    node->data.ifStmt.thenBlock = thenBlock;
    node->data.ifStmt.elseBlock = elseBlock;
    return node;
}

astNode *createWhileNode(astNode *condition, astNode *bodyBlock) {
    if (condition == NULL || bodyBlock == NULL)
        return NULL;

    astNode *node = newNode(AST_WHILE);
    node->data.whileStmt.condition = condition;
    node->data.whileStmt.bodyBlock = bodyBlock;
    return node;
}

astNode *createForNode(char *iteratorName, astNode *start, astNode *end,
                       bool endInclusive, astNode *step, astNode *body) {
    if (iteratorName == NULL || start == NULL || end == NULL || body == NULL)
        return NULL;

    astNode *node = newNode(AST_FOR);
    node->data.forStmt.iteratorName = iteratorName;
    node->data.forStmt.start = start;
    node->data.forStmt.end = end;
    node->data.forStmt.endInclusive = endInclusive;
    node->data.forStmt.step = step;
    node->data.forStmt.body = body;
    return node;
}

int addGuardToFor(astNode *forNode, astNode *guard) {
    if (forNode == NULL || forNode->type != AST_FOR || guard == NULL)
        return -1;

    appendNode(&forNode->data.forStmt.guards, &forNode->data.forStmt.guardCount,
               &forNode->data.forStmt.guardCapacity, guard);
    return 0;
}

astNode *createFuncCallNode(char *name) {
    if (name == NULL)
        return NULL;

    astNode *node = newNode(AST_FUNC_CALL);
    node->data.funcCall.funcName = name;
    node->data.funcCall.argIfAny = UNKNOWN;
    return node;
}

int addArgumentToCall(astNode *callNode, astNode *argument) {
    if (callNode == NULL || callNode->type != AST_FUNC_CALL ||
        argument == NULL) {
        return -1;
    }

    appendNode(&callNode->data.funcCall.arguments,
               &callNode->data.funcCall.argCount,
               &callNode->data.funcCall.argCapacity, argument);
    return 0;
}

astNode *createBinopNode(astBinopType op, astNode *left, astNode *right) {
    if (left == NULL || right == NULL)
        return NULL;

    astNode *node = newNode(AST_BINOP);
    node->data.binop.op = op;
    node->data.binop.left = left;
    node->data.binop.right = right;
    return node;
}

astNode *createUnopNode(astUnopType op, astNode *operand) {
    if (operand == NULL)
        return NULL;

    astNode *node = newNode(AST_UNOP);
    node->data.unop.op = op;
    node->data.unop.operand = operand;
    return node;
}

astNode *createIdentifierNode(char *name) {
    if (name == NULL)
        return NULL;

    astNode *node = newNode(AST_IDENTIFIER);
    node->data.identifier.name = name;
    return node;
}

astNode *createLiteralNode(Token *token) {
    if (token == NULL || (token->type != TOKEN_INT_LITERAL &&
                          token->type != TOKEN_DOUBLE_LITERAL &&
                          token->type != TOKEN_STRING_LITERAL &&
                          token->type != TOKEN_BOOL_LITERAL)) {
        return NULL;
    }

    astNode *node = newNode(AST_LITERAL);
    node->data.literal.litType = token->type;

    if (token->type == TOKEN_STRING_LITERAL) {
        // The lexer owns the token, so the AST keeps its own string copy.
        size_t length = token->attribute.text.length;
        if (token->attribute.text.data == NULL || length == SIZE_MAX) {
            free(node);
            return NULL;
        }

        char *text = malloc(length + 1);
        checkMalloc(text);
        memcpy(text, token->attribute.text.data, length);
        text[length] = '\0';
        node->data.literal.attribute.text.data = text;
        node->data.literal.attribute.text.length = length;
    } else {
        node->data.literal.attribute = token->attribute;
    }
    return node;
}

astNode *createUnitNode(void) {
    astNode *node = newNode(AST_LITERAL);
    node->resultType = UNIT;
    node->data.literal.litType = TOKEN_LPAREN;
    return node;
}

void freeAst(astNode *node) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
    case AST_PROGRAM:
        for (size_t i = 0; i < node->data.program.count; i++) {
            freeAst(node->data.program.functions[i]);
        }
        free(node->data.program.functions);
        break;
    case AST_FUNC_DEF:
        free(node->data.funcDef.name);
        for (size_t i = 0; i < node->data.funcDef.paramCount; i++) {
            free(node->data.funcDef.params[i].name);
        }
        free(node->data.funcDef.params);
        freeAst(node->data.funcDef.body);
        break;
    case AST_BLOCK:
        for (size_t i = 0; i < node->data.block.count; i++) {
            freeAst(node->data.block.statements[i]);
        }
        free(node->data.block.statements);
        break;
    case AST_VAR_DEF:
        free(node->data.varDef.name);
        freeAst(node->data.varDef.initializer);
        break;
    case AST_ASSIGN:
        free(node->data.assign.name);
        freeAst(node->data.assign.value);
        break;
    case AST_RETURN:
        freeAst(node->data.returnStmt.value);
        break;
    case AST_IF:
        freeAst(node->data.ifStmt.condition);
        freeAst(node->data.ifStmt.thenBlock);
        freeAst(node->data.ifStmt.elseBlock);
        break;
    case AST_WHILE:
        freeAst(node->data.whileStmt.condition);
        freeAst(node->data.whileStmt.bodyBlock);
        break;
    case AST_FOR:
        free(node->data.forStmt.iteratorName);
        freeAst(node->data.forStmt.start);
        freeAst(node->data.forStmt.end);
        freeAst(node->data.forStmt.step);
        for (size_t i = 0; i < node->data.forStmt.guardCount; i++) {
            freeAst(node->data.forStmt.guards[i]);
        }
        free(node->data.forStmt.guards);
        freeAst(node->data.forStmt.body);
        break;
    case AST_FUNC_CALL:
        free(node->data.funcCall.funcName);
        for (size_t i = 0; i < node->data.funcCall.argCount; i++) {
            freeAst(node->data.funcCall.arguments[i]);
        }
        free(node->data.funcCall.arguments);
        break;
    case AST_BINOP:
        freeAst(node->data.binop.left);
        freeAst(node->data.binop.right);
        break;
    case AST_UNOP:
        freeAst(node->data.unop.operand);
        break;
    case AST_LITERAL:
        if (node->data.literal.litType == TOKEN_STRING_LITERAL) {
            free(node->data.literal.attribute.text.data);
        }
        break;
    case AST_IDENTIFIER:
        free(node->data.identifier.name);
        break;
    }

    free(node);
}
