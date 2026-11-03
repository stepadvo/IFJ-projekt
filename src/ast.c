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
    node->result_type = UNKNOWN;

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
    node->result_type = UNKNOWN;

    // Store all the data passed from the parser
    node->data.func_def.name = name;
    node->data.func_def.sym_ptr = NULL;
    node->data.func_def.is_main = isMain;
    node->data.func_def.explicit_return_type = UNKNOWN;
    node->data.func_def.params = params;
    node->data.func_def.param_count = paramCount;

    node->data.func_def.body = body;

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
        size_t new_capacity = (programNode->data.program.capacity == 0)
                                  ? 4
                                  : programNode->data.program.capacity * 2;

        // Resize the array of pointers
        astNode **new_array = realloc(programNode->data.program.functions,
                                      new_capacity * sizeof(astNode *));
        checkMalloc(new_array);

        programNode->data.program.functions = new_array;
        programNode->data.program.capacity = new_capacity;
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
    node->result_type = UNKNOWN;
    node->data.for_stmt.iterator_name = iteratorName;
    node->data.for_stmt.sym_ptr = NULL;
    node->data.for_stmt.start = start;
    node->data.for_stmt.end = end;
    node->data.for_stmt.end_inclusive = endInclusive;
    node->data.for_stmt.step = step;
    node->data.for_stmt.guards = NULL;
    node->data.for_stmt.guard_count = 0;
    node->data.for_stmt.guard_capacity = 0;
    node->data.for_stmt.body = body;
    return node;
}

int addGuardToFor(astNode *forNode, astNode *guard) {
    if (forNode == NULL || forNode->type != AST_FOR || guard == NULL) {
        return -1;
    }

    if (forNode->data.for_stmt.guard_count ==
        forNode->data.for_stmt.guard_capacity) {
        size_t new_capacity = forNode->data.for_stmt.guard_capacity == 0
                                  ? 4
                                  : forNode->data.for_stmt.guard_capacity * 2;
        astNode **new_guards = realloc(forNode->data.for_stmt.guards,
                                       new_capacity * sizeof(*new_guards));
        checkMalloc(new_guards);
        forNode->data.for_stmt.guards = new_guards;
        forNode->data.for_stmt.guard_capacity = new_capacity;
    }

    forNode->data.for_stmt.guards[forNode->data.for_stmt.guard_count++] = guard;
    return 0;
}

/**
 * Free an AST and everything owned by its nodes.
 * Symbol table entries (sym_ptr) are only borrowed, never freed here.
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
        free(root->data.func_def.name);
        for (size_t i = 0; i < root->data.func_def.param_count; i++) {
            free(root->data.func_def.params[i].name);
        }
        free(root->data.func_def.params);
        freeAst(root->data.func_def.body);
        break;

    case AST_VAR_DEF:
        free(root->data.var_def.name);
        freeAst(root->data.var_def.initializer);
        break;

    case AST_ASSIGN:
        free(root->data.assign.name);
        freeAst(root->data.assign.value);
        break;

    case AST_IF:
        freeAst(root->data.if_stmt.condition);
        freeAst(root->data.if_stmt.then_block);
        freeAst(root->data.if_stmt.else_block);
        break;

    case AST_WHILE:
        freeAst(root->data.while_stmt.condition);
        freeAst(root->data.while_stmt.body_block);
        break;

    case AST_FOR:
        free(root->data.for_stmt.iterator_name);
        freeAst(root->data.for_stmt.start);
        freeAst(root->data.for_stmt.end);
        freeAst(root->data.for_stmt.step);
        for (size_t i = 0; i < root->data.for_stmt.guard_count; i++) {
            freeAst(root->data.for_stmt.guards[i]);
        }
        free(root->data.for_stmt.guards);
        freeAst(root->data.for_stmt.body);
        break;

    case AST_FUNC_CALL:
        free(root->data.func_call.func_name);
        for (size_t i = 0; i < root->data.func_call.arg_count; i++) {
            freeAst(root->data.func_call.arguments[i]);
        }
        free(root->data.func_call.arguments);
        break;

    case AST_RETURN:
        freeAst(root->data.return_stmt.value);
        break;

    case AST_BINOP:
        freeAst(root->data.binop.left);
        freeAst(root->data.binop.right);
        break;

    case AST_UNOP:
        freeAst(root->data.unop.operand);
        break;

    case AST_LITERAL:
        if (root->data.literal.lit_type == TOKEN_STRING_LITERAL) {
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
    node->result_type = UNKNOWN;
    return node;
}

/** Append a statement to a block, transferring ownership on success. */
int addStatementToBlock(astNode *blockNode, astNode *statement) {
    if (blockNode == NULL || blockNode->type != AST_BLOCK ||
        statement == NULL) {
        return -1;
    }

    if (blockNode->data.block.count == blockNode->data.block.capacity) {
        size_t new_capacity = blockNode->data.block.capacity == 0
                                  ? 4
                                  : blockNode->data.block.capacity * 2;
        astNode **new_statements =
            realloc(blockNode->data.block.statements,
                    new_capacity * sizeof(*new_statements));
        checkMalloc(new_statements);
        blockNode->data.block.statements = new_statements;
        blockNode->data.block.capacity = new_capacity;
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
    node->result_type = UNKNOWN;
    node->data.var_def.name = name;
    node->data.var_def.is_constant = isConstant;
    node->data.var_def.explicit_type = explicitType;
    node->data.var_def.initializer = initializer;
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
    node->result_type = UNKNOWN;
    node->data.literal.lit_type = token->type;

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
    node->result_type = UNKNOWN;
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_block = thenBlock;
    node->data.if_stmt.else_block = elseBlock;
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
    node->result_type = UNKNOWN;
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body_block = bodyBlock;
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
    node->result_type = UNKNOWN;
    node->data.assign.name = name;
    node->data.assign.sym_ptr = NULL;
    node->data.assign.value = value;
    return node;
}

/** Create a return statement; NULL value represents bare return. */
astNode *createReturnNode(astNode *value) {
    astNode *node = calloc(1, sizeof(*node));
    checkMalloc(node);
    node->type = AST_RETURN;
    node->result_type = UNKNOWN;
    node->data.return_stmt.value = value;
    return node;
}

/*
 * end ast.c
 */
