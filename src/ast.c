/**
 * @file ast.c
 * @brief implementation of the Abstract Syntax Tree (AST)
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 *
 */

#include "ast.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void checkMalloc(void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "Internal error: Memory allocation failed\n");
        exit(99); // code 99: special errors like memory allocation
    }
}

astNode *createProgramNode(void) {
    astNode *node = (astNode *)malloc(sizeof(astNode));
    checkMalloc(node);

    node->type = AST_PROGRAM;
    node->resultType = UNKNOWN;

    // Initial prep for functions
    node->data.program.functions = NULL;
    node->data.program.count = 0;
    node->data.program.capacity = 0;

    return node;
}

astNode *createFuncDefNode(char *name, bool isMain, astFuncParam *params,
                           size_t paramCount, astNode *body) {
    if (name == NULL || body == NULL || (paramCount > 0 && params == NULL)) {
        return NULL;
    }

    astNode *node = (astNode *)malloc(sizeof(astNode));
    checkMalloc(node);

    node->type = AST_FUNC_DEF;
    node->resultType = UNKNOWN;

    // Store all the data passed from the parser
    node->data.funcDef.name = name;
    node->data.funcDef.isMain = isMain;
    node->data.funcDef.explicitReturnType = UNKNOWN;
    node->data.funcDef.params = params;
    node->data.funcDef.paramCount = paramCount;

    node->data.funcDef.body = body;
    node->data.funcDef.alreadyGenerated = false;

    return node;
}

int addFunctionToProgram(astNode *programNode, astNode *funcDef) {
    if (programNode == NULL || programNode->type != AST_PROGRAM ||
        funcDef == NULL) {
        return -1;
    }

    // If the array is full/unallocated, we shall expand it
    if (programNode->data.program.count >= programNode->data.program.capacity) {
        // Double the capacity (or set to 4 if it is currently 0)
        size_t newCapacity = (programNode->data.program.capacity == 0)
                                 ? 4
                                 : programNode->data.program.capacity * 2;

        // Resize the array of pointers
        astNode **newArray = realloc(programNode->data.program.functions,
                                     newCapacity * sizeof(astNode *));
        checkMalloc(newArray);

        programNode->data.program.functions = newArray;
        programNode->data.program.capacity = newCapacity;
    }

    // Add the new function to the end of the array
    programNode->data.program.functions[programNode->data.program.count] =
        funcDef;

    // Increment the total function count
    programNode->data.program.count++;

    return 0;
}

astNode *createForNode(char *iteratorName, astNode *start, astNode *end,
                       bool endInclusive, astNode *step, astNode *body) {
    if (iteratorName == NULL || start == NULL || end == NULL || body == NULL) {
        return NULL;
    }

    astNode *node = malloc(sizeof(*node));
    checkMalloc(node);
    node->type = AST_FOR;
    node->resultType = UNKNOWN;
    node->data.forStmt.iteratorName = iteratorName;
    node->data.forStmt.start = start;
    node->data.forStmt.end = end;
    node->data.forStmt.endInclusive = endInclusive;
    node->data.forStmt.step = step;
    node->data.forStmt.guards = NULL;
    node->data.forStmt.guardCount = 0;
    node->data.forStmt.guardCapacity = 0;
    node->data.forStmt.body = body;
    return node;
}

int addGuardToFor(astNode *forNode, astNode *guard) {
    if (forNode == NULL || forNode->type != AST_FOR || guard == NULL) {
        return -1;
    }

    if (forNode->data.forStmt.guardCount ==
        forNode->data.forStmt.guardCapacity) {
        size_t newCapacity = forNode->data.forStmt.guardCapacity == 0
                                 ? 4
                                 : forNode->data.forStmt.guardCapacity * 2;
        astNode **newGuards = realloc(forNode->data.forStmt.guards,
                                      newCapacity * sizeof(*newGuards));
        checkMalloc(newGuards);
        forNode->data.forStmt.guards = newGuards;
        forNode->data.forStmt.guardCapacity = newCapacity;
    }

    forNode->data.forStmt.guards[forNode->data.forStmt.guardCount++] = guard;
    return 0;
}

/**
 * Free an AST and everything owned by its nodes.
 */
void freeAst(astNode *root) {
    if (root == NULL) {
        return;
    }

    switch (root->type) {
    case AST_PROGRAM:
        for (size_t i = 0; i < root->data.program.count; i++) {
            freeAst(root->data.program.functions[i]);
        }
        free(root->data.program.functions);
        break;

    case AST_FUNC_DEF:
        free(root->data.funcDef.name);
        for (size_t i = 0; i < root->data.funcDef.paramCount; i++) {
            free(root->data.funcDef.params[i].name);
        }
        free(root->data.funcDef.params);
        freeAst(root->data.funcDef.body);
        break;

    case AST_VAR_DEF:
        free(root->data.varDef.name);
        freeAst(root->data.varDef.initializer);
        break;

    case AST_ASSIGN:
        free(root->data.assign.name);
        freeAst(root->data.assign.value);
        break;

    case AST_IF:
        freeAst(root->data.ifStmt.condition);
        freeAst(root->data.ifStmt.thenBlock);
        freeAst(root->data.ifStmt.elseBlock);
        break;

    case AST_WHILE:
        freeAst(root->data.whileStmt.condition);
        freeAst(root->data.whileStmt.bodyBlock);
        break;

    case AST_FOR:
        free(root->data.forStmt.iteratorName);
        freeAst(root->data.forStmt.start);
        freeAst(root->data.forStmt.end);
        freeAst(root->data.forStmt.step);
        for (size_t i = 0; i < root->data.forStmt.guardCount; i++) {
            freeAst(root->data.forStmt.guards[i]);
        }
        free(root->data.forStmt.guards);
        freeAst(root->data.forStmt.body);
        break;

    case AST_FUNC_CALL:
        free(root->data.funcCall.funcName);
        for (size_t i = 0; i < root->data.funcCall.argCount; i++) {
            freeAst(root->data.funcCall.arguments[i]);
        }
        free(root->data.funcCall.arguments);
        break;

    case AST_RETURN:
        freeAst(root->data.returnStmt.value);
        break;

    case AST_BINOP:
        freeAst(root->data.binop.left);
        freeAst(root->data.binop.right);
        break;

    case AST_UNOP:
        freeAst(root->data.unop.operand);
        break;

    case AST_LITERAL:
        if (root->data.literal.litType == TOKEN_STRING_LITERAL) {
            free(root->data.literal.attribute.text.data);
        }
        break;

    case AST_IDENTIFIER:
        free(root->data.identifier.name);
        break;

    case AST_BLOCK:
        for (size_t i = 0; i < root->data.block.count; i++) {
            freeAst(root->data.block.statements[i]);
        }
        free(root->data.block.statements);
        break;
    }

    free(root);
}

/** Create an empty block whose statements can be appended later. */
astNode *createBlockNode(void) {
    astNode *node = calloc(1, sizeof(*node));
    checkMalloc(node);

    node->type = AST_BLOCK;
    node->resultType = UNKNOWN;
    node->data.block.bufferNum = -1;
    return node;
}

/** Append a statement to a block, transferring ownership on success. */
int addStatementToBlock(astNode *blockNode, astNode *statement) {
    if (blockNode == NULL || blockNode->type != AST_BLOCK ||
        statement == NULL) {
        return -1;
    }

    if (blockNode->data.block.count == blockNode->data.block.capacity) {
        size_t newCapacity = blockNode->data.block.capacity == 0
                                 ? 4
                                 : blockNode->data.block.capacity * 2;
        astNode **newStatements = realloc(blockNode->data.block.statements,
                                          newCapacity * sizeof(*newStatements));
        checkMalloc(newStatements);
        blockNode->data.block.statements = newStatements;
        blockNode->data.block.capacity = newCapacity;
    }

    blockNode->data.block.statements[blockNode->data.block.count++] = statement;
    return 0;
}

/** Create a val/var declaration; the node owns name and initializer. */
astNode *createVarDefNode(char *name, bool isConstant, varType explicitType,
                          astNode *initializer) {
    if (name == NULL || initializer == NULL) {
        return NULL;
    }

    astNode *node = calloc(1, sizeof(*node));
    checkMalloc(node);
    node->type = AST_VAR_DEF;
    node->resultType = UNKNOWN;
    node->data.varDef.name = name;
    node->data.varDef.isConstant = isConstant;
    node->data.varDef.explicitType = explicitType;
    node->data.varDef.initializer = initializer;
    return node;
}

/** Create a literal with an independent copy of any token-owned text. */
astNode *createLiteralNode(Token *token) {
    if (token == NULL || (token->type != TOKEN_INT_LITERAL &&
                          token->type != TOKEN_DOUBLE_LITERAL &&
                          token->type != TOKEN_STRING_LITERAL &&
                          token->type != TOKEN_BOOL_LITERAL)) {
        return NULL;
    }

    astNode *node = calloc(1, sizeof(*node));
    checkMalloc(node);
    node->type = AST_LITERAL;
    node->resultType = UNKNOWN;
    node->data.literal.litType = token->type;

    if (token->type == TOKEN_STRING_LITERAL) {
        // The lexer will free token->attribute.text.data when advancing.
        size_t length = token->attribute.text.length;
        if (token->attribute.text.data == NULL || length == SIZE_MAX) {
            free(node);
            return NULL;
        }
        char *copy = malloc(length + 1);
        checkMalloc(copy);
        memcpy(copy, token->attribute.text.data, length);
        copy[length] = '\0';
        node->data.literal.attribute.text.data = copy;
        node->data.literal.attribute.text.length = length;
    } else {
        node->data.literal.attribute = token->attribute;
    }

    return node;
}

/** Create an if node; elseBlock may be NULL for statements. */
astNode *createIfNode(astNode *condition, astNode *thenBlock,
                      astNode *elseBlock) {
    if (condition == NULL || thenBlock == NULL) {
        return NULL;
    }
    astNode *node = calloc(1, sizeof(*node));
    checkMalloc(node);
    node->type = AST_IF;
    node->resultType = UNKNOWN;
    node->data.ifStmt.condition = condition;
    node->data.ifStmt.thenBlock = thenBlock;
    node->data.ifStmt.elseBlock = elseBlock;
    return node;
}

/** Create a while loop node. */
astNode *createWhileNode(astNode *condition, astNode *bodyBlock) {
    if (condition == NULL || bodyBlock == NULL) {
        return NULL;
    }
    astNode *node = calloc(1, sizeof(*node));
    checkMalloc(node);
    node->type = AST_WHILE;
    node->resultType = UNKNOWN;
    node->data.whileStmt.condition = condition;
    node->data.whileStmt.bodyBlock = bodyBlock;
    return node;
}

/** Create an assignment; the node owns name and value. */
astNode *createAssignNode(char *name, astNode *value) {
    if (name == NULL || value == NULL) {
        return NULL;
    }
    astNode *node = calloc(1, sizeof(*node));
    checkMalloc(node);
    node->type = AST_ASSIGN;
    node->resultType = UNKNOWN;
    node->data.assign.name = name;
    node->data.assign.value = value;
    return node;
}

/** Create a return statement; NULL value represents bare return. */
astNode *createReturnNode(astNode *value) {
    astNode *node = calloc(1, sizeof(*node));
    checkMalloc(node);
    node->type = AST_RETURN;
    node->resultType = UNKNOWN;
    node->data.returnStmt.value = value;
    return node;
}

/*
 * end ast.c
 */
