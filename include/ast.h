/**
 * @file ast.h
 * @brief interface declaration for the Abstract Syntax Tree (AST)
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 *
 * Declarations of the structures and functions working with the AST.
 */

#ifndef AST_H
#define AST_H

#include "symtable.h"
#include "token.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief enum of all possible node types in the AST
 */
typedef enum {
    AST_PROGRAM,    /**< Root node of the entire IFJ26 program */
    AST_FUNC_DEF,   /**< Function definition (def) */
    AST_VAR_DEF,    /**< Variable definition (val / var) */
    AST_ASSIGN,     /**< Assignment to a variable (=) */
    AST_IF,         /**< Conditional statement (if-then-else) */
    AST_WHILE,      /**< Loop statement (while) */
    AST_FOR,        /**< Range-based for loop (CONTROL extension) */
    AST_FUNC_CALL,  /**< Function call */
    AST_RETURN,     /**< Return statement inside a function */
    AST_BINOP,      /**< Binary operations (+, -, *, /, <, ==, etc.) */
    AST_UNOP,       /**< Unary operations (-, !) */
    AST_LITERAL,    /**< Constant literals (42, "text", 3.14) */
    AST_IDENTIFIER, /**< Variable or function names */
    AST_BLOCK /**< Block of statements (body of functions, loops, conditions) */
} astNodeType;

/**
 * @brief the abstract syntax tree node structure
 *
 * Nodes own their child nodes, copied strings, and parameter/child arrays.
 * Each child must have one owner. All symbol pointers (sym_ptr) are borrowed
 * from the symbol table and are never freed by the AST.
 *
 * result_type is set to UNKNOWN until the resulting type is determined.
 */
typedef struct astNode astNode;

/**
 * @brief structure for holding function parameters in the AST
 */
typedef struct {
    char *name;
    varType type;
} astFuncParam;

struct astNode {
    astNodeType type;
    varType result_type;

    union {
        // if the node type is AST_PROGRAM
        struct {
            astNode **functions;
            size_t count;
            size_t capacity;
        } program;

        // if the node type is AST_BINOP
        struct {
            TokenType op;
            astNode *left;
            astNode *right;
        } binop;

        // if the node type is AST_UNOP
        struct {
            TokenType op;
            astNode *operand;
        } unop;

        // if the node type is AST_LITERAL
        struct {
            TokenType lit_type;
            TokenAttribute attribute;
        } literal;

        // if the node type is AST_IDENTIFIER
        struct {
            char *name;
            symbol *sym_ptr;
        } identifier;

        // if the node type is AST_IF (statement or expression under CONTROL)
        struct {
            astNode *condition;
            astNode *then_block;
            astNode *else_block;
        } if_stmt;

        // if the node type is AST_FOR (CONTROL extension)
        struct {
            char *iterator_name; // Name of the new immutable loop variable
            symbol *sym_ptr;     // Borrowed pointer; owned by symbol table
            astNode *start;      // Inclusive starting bound expression
            astNode *end;        // Ending bound expression
            bool end_inclusive;  // true = "to", false = "until"
            astNode *step;       // Optional "by" expression; NULL means +1
            astNode **guards;    // Optional "if" conditions, in source order
            size_t guard_count;
            size_t guard_capacity;
            astNode *body; // AST_BLOCK or a single statement/expression
        } for_stmt;

        // if the node type is AST_WHILE
        struct {
            astNode *condition;
            astNode *body_block;
        } while_stmt;

        // if the node type is AST_BLOCK
        struct {
            astNode **statements;
            size_t count;
            size_t capacity;
        } block;

        // if the node type is AST_FUNC_CALL
        struct {
            char *func_name;
            symbol *sym_ptr;
            astNode **arguments;
            size_t arg_count;
            size_t arg_capacity;
        } func_call;

        // if the node type is AST_VAR_DEF
        struct {
            char *name;
            bool is_constant;
            varType explicit_type;
            astNode *initializer;
        } var_def;

        // if the node type is AST_ASSIGN
        struct {
            char *name;
            symbol *sym_ptr;
            astNode *value;
        } assign;

        // if the node type is AST_FUNC_DEF
        struct {
            char *name;
            symbol *sym_ptr;
            bool is_main;
            varType explicit_return_type; // UNKNOWN if omitted in the source
            astFuncParam *params;
            size_t param_count;
            astNode *body;
        } func_def;

        // if the node type is AST_RETURN
        struct {
            astNode *value;
        } return_stmt;

    } data;
};

/**
 * @brief checks if dynamic allocation succeded
 * @param ptr Pointer to the allocated memory to check
 */
void checkMalloc(void *ptr);

/**
 * @brief creates a program root node
 * @return Pointer to the newly allocated program node
 */
astNode *createProgramNode();

/**
 * @brief creates an empty block node
 * @return Pointer to the newly allocated block node
 */
astNode *createBlockNode();

/**
 * @brief appends a statement to the block node
 * @param blockNode The block node to append to
 * @param statement The statement node to append
 * @return 0 on success, non-zero on failure
 */
int addStatementToBlock(astNode *blockNode, astNode *statement);

/**
 * @brief creates a function definition node
 * @param name Name of the function
 * @param isMain True if the function is @main
 * @param params Array of function parameters
 * @param paramCount Number of parameters
 * @param body Block node or single-expression function body
 * @details On success, the AST owns name, params (including parameter names),
 *          and body. The declared return type can be set in
 *          data.func_def.explicit_return_type after parsing the header.
 * @return Pointer to the newly allocated function node
 */
astNode *createFuncDefNode(char *name, bool isMain, astFuncParam *params,
                           size_t paramCount, astNode *body);

/**
 * @brief creates a variable definition node (val/var)
 * @param name Name of the variable
 * @param isConstant True if defined with 'val', false for 'var'
 * @param explicitType The explicitly stated type (or UNKNOWN if inferred)
 * @param initializer The expression node assigned to the variable
 * @return Pointer to the newly allocated variable definition node
 */
astNode *createVarDefNode(char *name, bool isConstant, varType explicitType,
                          astNode *initializer);

/**
 * @brief creates a literal node
 * @param token The token containing an Int, Double, String, or Boolean literal
 * @details String contents are copied; the lexer still owns the source token.
 *          Identifier tokens are not literals and return NULL.
 * @return Pointer to the newly allocated literal node
 */
astNode *createLiteralNode(Token *token);

/**
 * @brief creates a conditional if-then-else node
 * @param condition The expression node representing the if condition
 * @param thenBlock A block or single expression/statement for the true branch
 * @param elseBlock A block or single expression/statement for the false branch
 *                  (can be NULL for an if statement; required for an if
 * expression)
 * @return Pointer to the newly allocated if node
 */
astNode *createIfNode(astNode *condition, astNode *thenBlock,
                      astNode *elseBlock);

/**
 * @brief creates a while loop node
 * @param condition The expression node representing the loop condition
 * @param bodyBlock The block node containing the loop body
 * @return Pointer to the newly allocated while node
 */
astNode *createWhileNode(astNode *condition, astNode *bodyBlock);

/**
 * @brief creates a range-based for loop (CONTROL extension)
 * @param iteratorName Heap-allocated name of the immutable loop variable
 * @param start Inclusive starting bound expression
 * @param end Ending bound expression
 * @param endInclusive True for "to", false for "until"
 * @param step Optional "by" expression; NULL means the default step of +1
 * @param body Block node or a single statement/expression
 * @return Pointer to the newly allocated for node
 *
 * On success, the new node owns iteratorName, start, end, step and body.
 */
astNode *createForNode(char *iteratorName, astNode *start, astNode *end,
                       bool endInclusive, astNode *step, astNode *body);

/**
 * @brief appends a Boolean guard to a for loop, in source order
 * @param forNode The AST_FOR node
 * @param guard Condition expression to append
 * @return 0 on success, -1 for invalid arguments
 *
 * On success, forNode owns the guard node.
 */
int addGuardToFor(astNode *forNode, astNode *guard);

/**
 * @brief creates an assignment node
 * @param name The name of the target variable being assigned to
 * @param value The expression node representing the new value
 * @return Pointer to the newly allocated assignment node
 */
astNode *createAssignNode(char *name, astNode *value);

/**
 * @brief creates a return statement node
 * @param value The expression node to return (can be NULL)
 * @return Pointer to the newly allocated return node
 */
astNode *createReturnNode(astNode *value);

/**
 * @brief recursively frees the tree and all data it owns (not symbols)
 * @param root Pointer to the root node of the AST to free
 */
void freeAst(astNode *root);

/**
 * @brief appends a function definition to the program root node
 * @param programNode The root program node
 * @param funcDef The function definition node to append
 * @return 0 on success, non-zero on failure
 */
int addFunctionToProgram(astNode *programNode, astNode *funcDef);

#endif // AST_H
