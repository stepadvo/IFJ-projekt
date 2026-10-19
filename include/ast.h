/**
 * @file ast.h
 * @brief interface declaration for the Abstract Syntax Tree (AST)
 * @author Jiří Dohnal, xdohnaj00
 *
 * Declarations of the structures and functions working with the AST.
 */

#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stddef.h>
#include "token.h"
#include "symtable.h"

/** @brief enum of all possible node types in the AST */
typedef enum
{
    AST_PROGRAM,        // Root node of the entire IFJ26 program
    AST_FUNC_DEF,       // Function definition (def)
    AST_VAR_DEF,        // Variable definition (val / var)
    AST_ASSIGN,         // Assignment to a variable (=)
    AST_IF,             // Conditional statement (if-then-else)
    AST_WHILE,          // Loop statement (while)
    AST_FUNC_CALL,      // Function call
    AST_RETURN,         // Return statement inside a function
    AST_BINOP,          // Binary operations (+, -, *, /, <, ==, etc.)
    AST_UNOP,           // Unary operations (-, !)
    AST_LITERAL,        // Constant literals (42, "text", 3.14)
    AST_IDENTIFIER,     // Variable or function names
    AST_BLOCK           // Block of statements (body of functions, loops, conditions)
} astNodeType;

/**
 * @brief the abstract syntax tree node structure
 * 
 * the structure will be created and allocated by create node functions
 */
typedef struct astNode astNode;

/**
 * @brief structure for holding function parameters in the AST
 */
typedef struct
{
    char *name;
    varType type;
} astFuncParam;

struct astNode
{
    astNodeType type;
    varType result_type; 
    
    union
    {
        // if the node type is AST_PROGRAM
        struct
        {
            astNode **functions;
            size_t count;
            size_t capacity;
        } program;

        // if the node type is AST_BINOP
        struct
        {
            TokenType op;
            astNode *left;
            astNode *right;
        } binop;

        // if the node type is AST_UNOP
        struct
        {
            TokenType op;
            astNode *operand;
        } unop;

        // if the node type is AST_LITERAL
        struct
        {
            TokenType lit_type;
            TokenAttribute attribute;
        } literal;

        // if the node type is AST_IDENTIFIER
        struct
        {
            char *name;
            symbol *sym_ptr;
        } identifier;

        // if the node type is AST_IF
        struct
        {
            astNode *condition;
            astNode *then_block;
            astNode *else_block;
        } if_stmt;

        // if the node type is AST_WHILE
        struct
        {
            astNode *condition;
            astNode *body_block;
        } while_stmt;

        // if the node type is AST_BLOCK
        struct
        {
            astNode **statements;
            size_t count;
            size_t capacity;
        } block;
        
        // if the node type is AST_FUNC_CALL
        struct
        {
            char *func_name;
            symbol *sym_ptr;
            astNode **arguments;
            size_t arg_count;
            size_t arg_capacity;
        } func_call;

        // if the node type is AST_VAR_DEF
        struct
        {
            char *name;
            bool is_constant;
            varType explicit_type;
            astNode *initializer;
        } var_def;

        // if the node type is AST_ASSIGN
        struct
        {
            char *name;
            symbol *sym_ptr;
            astNode *value;
        } assign;
        
        // if the node type is AST_FUNC_DEF
        struct
        {
            char *name;
            symbol *sym_ptr;
            bool is_main;
            astFuncParam *params;
            size_t param_count;
            astNode *body;
        } func_def;

        // if the node type is AST_RETURN
        struct
        {
            astNode *value;
        } return_stmt;

    } data;
};

/**
 * @brief creates a program root node
 */
astNode *createProgramNode();

/**
 * @brief creates an empty block node
 */
astNode *createBlockNode();

/**
 * @brief appends a statement to the block node
 */
int addStatementToBlock(astNode *blockNode, astNode *statement); 

/**
 * @brief creates a function definition node
 */
astNode *createFuncDefNode(char *name, bool isMain, astFuncParam *params, size_t paramCount, astNode *body);

/**
 * @brief creates a variable definition node (val/var)
 */
astNode *createVarDefNode(char *name, bool isConstant, varType explicitType, astNode *initializer);

/**
 * @brief creates a literal node
 */
astNode *createLiteralNode(Token *token);

/**
 * @brief destructs an ast and frees allocated memory
 */
void freeAst(astNode *root);

#endif // AST_H

/*
 * end ast.h
 */
