/**
 * @file ast.h
 * @brief AST structures and functions used by the IFJ26 parser.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 *
 */

#ifndef AST_H
#define AST_H

#include "compiler.h"
#include "token.h"
#include <stdbool.h>
#include <stddef.h>

/** Types of nodes stored in the AST. */
typedef enum {
    AST_PROGRAM,    /**< Program root (all function definitions). */
    AST_FUNC_DEF,   /**< Function definition (def add(...) = ...). */
    AST_VAR_DEF,    /**< New variable (val x = 5, var y = 0). */
    AST_ASSIGN,     /**< Assignment to an existing variable (x = 5). */
    AST_IF,         /**< If statement or expression. */
    AST_WHILE,      /**< While loop. */
    AST_FOR,        /**< Range-based for loop (CONTROL). */
    AST_FUNC_CALL,  /**< Function call (add(2, 3)). */
    AST_RETURN,     /**< Return statement (return x). */
    AST_BINOP,      /**< Binary expression (a + b, x < 5). */
    AST_UNOP,       /**< Unary expression (-x, !flag). */
    AST_LITERAL,    /**< Literal (42, "hello", true). */
    AST_IDENTIFIER, /**< Use of a variable or parameter (x). */
    AST_BLOCK       /**< Statements in a block. */
} astNodeType;

typedef struct astNode astNode;

/** One parameter in a function definition (e.g. x: Int). */
typedef struct {
    char *name;   /**< Parameter name; owned by the AST. */
    varType type; /**< Declared parameter type. */
} astFuncParam;

/**
 * @brief A node in the syntax tree.
 *
 * Only the data member matching type is valid. resultType is initially
 * UNKNOWN and can be filled in during semantic analysis.
 */
struct astNode {
    astNodeType type;   /**< Node kind, e.g. AST_IF or AST_BINOP. */
    varType resultType; /**< Result type after semantic analysis. */

    union {
        /**
         * @brief Root of the file.
         * For two definitions, functions[0] and functions[1] point to
         * two AST_FUNC_DEF nodes.
         */
        struct {
            astNode **functions; /**< AST_FUNC_DEF nodes, in source order. */
            size_t count;        /**< Number of functions. */
            size_t capacity;     /**< Allocated slots in functions. */
        } program;

        /**
         * @brief Binary operation (e.g. 2 + 3 * 4).
         * The AST stores precedence through its shape: + is the parent
         * and * is its right child in this example.
         */
        struct {
            TokenType op;   /**< Operator (+, *, ==, ...). */
            astNode *left;  /**< Left-hand expression. */
            astNode *right; /**< Right-hand expression. */
        } binop;

        /**
         * @brief Unary operation (e.g. -x or !ready).
         * Unlike a binary operation, this has only one operand.
         */
        struct {
            TokenType op;     /**< Unary operator (-, +, !). */
            astNode *operand; /**< Expression to which it applies. */
        } unop;

        /**
         * @brief Literal from a token (e.g. 42, true, "hello").
         * litType tells us which value in attribute is valid.
         * String data is copied from the lexer token.
         */
        struct {
            TokenType litType; /**< Literal token type. */
            TokenAttribute
                attribute; /**< Literal value; string data is copied. */
        } literal;

        /**
         * @brief Use of a named value (e.g. x in return x).
         * This is a reference to an existing variable or parameter,
         * not a new declaration. The name is enough for symbol lookup.
         */
        struct {
            char *name; /**< Variable or parameter name. */
        } identifier;

        /**
         * @brief If statement or expression (CONTROL / BOOLTHEN).
         * An if expression has a value, so it needs an else branch.
         * A plain if statement may omit else.
         */
        struct {
            astNode *condition; /**< Boolean expression after if. */
            astNode
                *thenBlock; /**< True branch: expression, statement or block. */
            astNode *elseBlock; /**< False branch; NULL if omitted. */
        } ifStmt;

        /**
         * @brief Range-based loop (e.g. for i <- 1 until 10 by 2 do ...).
         * Each guard is another Boolean condition (if ...). If a guard
         * is false, that iteration is skipped; the loop continues.
         */
        struct {
            char *iteratorName;   /**< Loop variable (i). */
            astNode *start;       /**< Starting bound expression. */
            astNode *end;         /**< Ending bound expression. */
            bool endInclusive;    /**< true for to, false for until. */
            astNode *step;        /**< Optional by expression; NULL means +1. */
            astNode **guards;     /**< Conditions after if; all must hold. */
            size_t guardCount;    /**< Number of guards. */
            size_t guardCapacity; /**< Allocated slots in guards. */
            astNode *body; /**< Loop body (statement, expression or block). */
        } forStmt;

        /**
         * @brief While loop (e.g. while x < 10 do ...).
         * The body runs while the condition evaluates to true.
         */
        struct {
            astNode *condition; /**< Loop condition. */
            astNode *bodyBlock; /**< Loop body, stored as AST_BLOCK. */
        } whileStmt;

        /**
         * @brief Statements belonging to one block.
         * Example: val x = 5, then return x. The nodes are kept
         * in source order. bufferNum is reserved for the generator.
         */
        struct {
            astNode **statements; /**< Statements in their original order. */
            size_t count;         /**< Number of statements. */
            size_t capacity;      /**< Allocated slots in statements. */
            int bufferNum;        /**< Buffer ID used by code generation. */
        } block;

        /**
         * @brief Function call (e.g. add(2, x + 1)).
         * Each argument is an expression; arguments[1] would be the
         * AST_BINOP for x + 1. No arguments means argCount == 0.
         */
        struct {
            char *funcName;      /**< Name of the called function. */
            astNode **arguments; /**< Argument expressions in call order. */
            size_t argCount;     /**< Number of arguments. */
            size_t argCapacity;  /**< Allocated slots in arguments. */
        } funcCall;

        /**
         * @brief New variable (e.g. val x: Int = 5).
         * Unlike AST_ASSIGN, this introduces the variable. If there
         * is no written type, the semantic pass infers it.
         */
        struct {
            char *name;           /**< Variable name. */
            bool isConstant;      /**< true for val, false for var. */
            varType explicitType; /**< Written type; UNKNOWN if omitted. */
            astNode *initializer; /**< Expression used to initialize it. */
        } varDef;

        /**
         * @brief Assignment to an existing variable (e.g. x = x + 1).
         * The target is stored by name; value is the expression to assign.
         */
        struct {
            char *name;     /**< Variable being assigned to. */
            astNode *value; /**< New value as an expression. */
        } assign;

        /**
         * @brief Function definition (e.g. def add(a: Int) = a + 1).
         * A single-expression function points straight to the expression.
         * A multiline function points to AST_BLOCK instead.
         * alreadyGenerated is used to avoid generating a function twice.
         */
        struct {
            char *name;                 /**< Function name. */
            bool isMain;                /**< true if marked with @main. */
            varType explicitReturnType; /**< Written return type; UNKNOWN if
                                           omitted. */
            astFuncParam *params;       /**< Parameters in declaration order. */
            size_t paramCount;          /**< Number of parameters. */
            astNode *body;         /**< AST_BLOCK or a single expression. */
            bool alreadyGenerated; /**< Set once this function has been
                                      generated. */
        } funcDef;

        /**
         * @brief Return from a function (e.g. return x + 1).
         * A return without a value has value == NULL.
         */
        struct {
            astNode *value; /**< Returned expression; NULL for bare return. */
        } returnStmt;

    } data;
};

/**
 * @brief Checks an allocation and exits with code 99 if it failed.
 * @param ptr Pointer returned by an allocation function.
 */
void checkMalloc(void *ptr);

/** Creates an empty AST_PROGRAM node. */
astNode *createProgramNode(void);

/** Creates an empty AST_BLOCK node (bufferNum starts at -1). */
astNode *createBlockNode(void);

/**
 * @brief Adds a statement to a block.
 * @return 0 on success, -1 for invalid arguments.
 * The block takes ownership of the statement on success.
 */
int addStatementToBlock(astNode *blockNode, astNode *statement);

/**
 * @brief Creates a function definition.
 * @param name Allocated function name.
 * @param isMain Whether the function has @main.
 * @param params Array of parameters, including their allocated names.
 * @param paramCount Number of entries in params.
 * @param body Function body (AST_BLOCK or a single expression).
 *
 * The new node owns name, params and body. Its explicitReturnType starts
 * as UNKNOWN and alreadyGenerated starts as false.
 */
astNode *createFuncDefNode(char *name, bool isMain, astFuncParam *params,
                           size_t paramCount, astNode *body);

/**
 * @brief Creates a val/var declaration.
 * @param explicitType Declared type, or UNKNOWN if omitted.
 * The node takes ownership of name and initializer.
 */
astNode *createVarDefNode(char *name, bool isConstant, varType explicitType,
                          astNode *initializer);

/**
 * @brief Creates a literal from a lexer token.
 * Copies string contents so the token can be freed independently.
 * Returns NULL for unsupported token types.
 */
astNode *createLiteralNode(Token *token);

/**
 * @brief Creates an if node.
 * elseBlock may be NULL for an if statement without else. An if expression
 * needs both branches.
 */
astNode *createIfNode(astNode *condition, astNode *thenBlock,
                      astNode *elseBlock);

/** Creates a while loop with a condition and body block. */
astNode *createWhileNode(astNode *condition, astNode *bodyBlock);

/**
 * @brief Creates a range-based for loop.
 * @param endInclusive true for to, false for until.
 * @param step Optional by expression; NULL uses the default step.
 * Takes ownership of iteratorName and all child nodes passed to it.
 */
astNode *createForNode(char *iteratorName, astNode *start, astNode *end,
                       bool endInclusive, astNode *step, astNode *body);

/**
 * @brief Appends one guard condition to a for loop.
 * @return 0 on success, -1 for invalid arguments.
 * On success the for node owns guard.
 */
int addGuardToFor(astNode *forNode, astNode *guard);

/** Expression nodes. The returned node owns the supplied operands/name. */
astNode *createBinopNode(TokenType op, astNode *left, astNode *right);
astNode *createUnopNode(TokenType op, astNode *operand);
astNode *createIdentifierNode(char *name);
astNode *createFuncCallNode(char *name);
int addArgumentToCall(astNode *callNode, astNode *argument);
/** Unit literal, written (). */
astNode *createUnitNode(void);

/** Creates an assignment; the node owns name and value. */
astNode *createAssignNode(char *name, astNode *value);

/** Creates a return node (value may be NULL). */
astNode *createReturnNode(astNode *value);

/** Recursively frees the AST, including names and child arrays. */
void freeAst(astNode *root);

/**
 * @brief Appends a function to an AST_PROGRAM node.
 * @return 0 on success, -1 for invalid arguments.
 * The program takes ownership of funcDef on success.
 */
int addFunctionToProgram(astNode *programNode, astNode *funcDef);

#endif // AST_H
