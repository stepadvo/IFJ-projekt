/**
 * @file ast.h
 * @brief Abstract Syntax Tree structures and functions for the IFJ26 parser.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 */

#ifndef AST_H
#define AST_H

#include "compiler.h"
#include "token.h"
#include <stdbool.h>
#include <stddef.h>

/** @brief if defined, the node creation functions won't take ownership of the
 * pointers to strings */
// #define IFJ_AST_NOOWN

/**
 * @brief Identifies the specific type of an AST node.
 */
typedef enum {
    /**< The root node that contains all function definitions in the file. */
    AST_PROGRAM,
    /**< A function definition, including its parameters and body. */
    AST_FUNC_DEF,
    /**< A new variable declaration (val or var). */
    AST_VAR_DEF,
    /**< An assignment updating an existing variable. */
    AST_ASSIGN,
    /**< An if-statement or an if-expression (with an else branch). */
    AST_IF,
    /**< A standard while loop. */
    AST_WHILE,
    /**< A range-based for loop, optionally with guard conditions. */
    AST_FOR,
    /**< A function invocation with its arguments. */
    AST_FUNC_CALL,
    /**< A return statement, which may or may not return a value. */
    AST_RETURN,
    /**< A binary operation with left and right operands (e.g., addition). */
    AST_BINOP,
    /**< A unary operation applied to a single operand (e.g., logical NOT). */
    AST_UNOP,
    /**< A literal value like a number, string, or boolean. */
    AST_LITERAL,
    /**< A reference to a variable or parameter by its name. */
    AST_IDENTIFIER,
    /**< A block of statements grouped together, usually in braces. */
    AST_BLOCK,
    /**< A convertion from INT to DOUBLE (for the generator)*/
    AST_INT2DOUBLE
} astNodeType;

/** @brief Type of unary oparators. */
typedef enum {
    UNARY_MINUS, /**< Unary minus: "-[val]" */
    NOT,         /**< Not: "![val]" */
    UNARY_PLUS   /**< Unary plus: "+[val]" */
} astUnopType;

/** @brief Type of binary oparators */
typedef enum {
    PLUS,          /**< Plus: "+" */
    CONCATENATION, /**< Concatenation: "[str] + [str]" */
    MINUS,         /**< Minus: "-" */
    MULTIPLY,      /**< Multiplication: "*" */
    DIVIDE,        /**< Division: "[float] / [float]" */
    I_DIVIDE,      /**< Integer divison: "[int] / [int]" */
    EQUAL,         /**< Equality: "==" */
    NOT_EQUAL,     /**< Inequality: "!=" */
    LESS,          /**< Less than: "<" */
    LESS_EQUAL,    /**< Less than or equal to: "<=" */
    GREATER,       /**< Greater than: ">" */
    GREATER_EQUAL, /**< Greater than or equal to: ">=" */
    AND,           /**< Logical conjunction: "&&" */
    OR             /**< Logical disjunction: "||" */
} astBinopType;

/** @brief Forward declaration of the AST node structure. */
typedef struct astNode astNode;

/**
 * @brief Represents a single parameter in a function signature.
 */
typedef struct {
    /**< The name of the parameter. The AST takes ownership of this string. */
    char *name;
    /**< The declared data type of the parameter. */
    varType type;
} astFuncParam;

/**
 * @brief A generic node in the Abstract Syntax Tree.
 *
 * This struct uses a union to store data specific to the node's type.
 * You should only access the union member that corresponds to `type`.
 */
struct astNode {
    /**< The kind of node this is (e.g., AST_IF, AST_BINOP). */
    astNodeType type;
    /**< The data type this node evaluates to. Filled during semantic analysis.
     */
    varType resultType;

    union {
        /**
         * @brief Represents the entire parsed program.
         * Contains a sequential list of all function definitions.
         */
        struct {
            /**< Array pointing to AST_FUNC_DEF nodes. */
            astNode **functions;
            /**< The number of functions currently stored. */
            size_t count;
            /**< The allocated size of the functions array. */
            size_t capacity;
        } program;

        /**
         * @brief A standard binary operation (like arithmetic or comparisons).
         * Operator precedence is determined by how the tree is structured.
         */
        struct {
            /**< The operator (+, *, ==, etc.). */
            astBinopType op;
            /**< The left-hand side of the operation. */
            astNode *left;
            /**< The right-hand side of the operation. */
            astNode *right;
        } binop;

        /**
         * @brief A unary operation (like negation or logical NOT).
         */
        struct {
            /**< The unary operator (-, +, !). */
            astUnopType op;
            /**< The expression being modified. */
            astNode *operand;
        } unop;

        /**
         * @brief A raw literal value found in the source code.
         * For string literals, the actual text is copied from the lexer.
         */
        struct {
            /**< The token type identifying the kind of literal. */
            TokenType litType;
            /**< The parsed value. Text strings are duplicated into AST memory.
             */
            TokenAttribute attribute;
        } literal;

        /**
         * @brief A reference to a variable or parameter.
         * We store the name here so the semantic analyzer can look it up later.
         */
        struct {
            /**< The name of the identifier as written in the code. */
            char *name;
        } identifier;

        /**
         * @brief An if-statement or if-expression.
         * If it's used as an expression, it must have an else block.
         */
        struct {
            /**< The boolean condition to evaluate. */
            astNode *condition;
            /**< The block to execute if the condition is true. */
            astNode *thenBlock;
            /**< The block to execute if the condition is false. Can be NULL. */
            astNode *elseBlock;
        } ifStmt;

        /**
         * @brief A range-based for loop.
         * Iterates over a range, optionally filtering with guard conditions.
         */
        struct {
            /**< The name of the loop iterator variable. */
            char *iteratorName;
            /**< The unique id of the loop iterator variable (for generation).
             */
            int iteratorUqId;
            /**< The starting value of the range. */
            astNode *start;
            /**< The ending value of the range. */
            astNode *end;
            /**< True if the end value is included ('to'), false if not
             * ('until'). */
            bool endInclusive;
            /**< The step size expression. If NULL, defaults to 1. */
            astNode *step;
            /**< Array of additional boolean conditions to filter iterations. */
            astNode **guards;
            /**< The number of guards currently applied. */
            size_t guardCount;
            /**< The allocated size of the guards array. */
            size_t guardCapacity;
            /**< The body of the loop to execute. */
            astNode *body;
        } forStmt;

        /**
         * @brief A standard while loop.
         */
        struct {
            /**< The condition that keeps the loop running. */
            astNode *condition;
            /**< The body of the loop, always stored as an AST_BLOCK. */
            astNode *bodyBlock;
        } whileStmt;

        /**
         * @brief A sequential block of statements.
         * Preserves the order in which statements were written.
         */
        struct {
            /**< Array of statement nodes. */
            astNode **statements;
            /**< The number of statements in this block. */
            size_t count;
            /**< The allocated size of the statements array. */
            size_t capacity;
            /**< Used internally by the code generator. Safe to ignore for now.
             */
            void *bufferPtr;
            /**< Used internally by the code generator. Set to not NULL only
             * when the block is a body of for loop */
            astNode *parentIfForBody;
        } block;

        /**
         * @brief A function call.
         */
        struct {
            /**< The name of the function being called. */
            char *funcName;
            /**< Array of argument expressions passed to the function. */
            astNode **arguments;
            /**< With built-in functions one argument can be Any. */
            varType argIfAny;
            /**< The number of arguments provided. */
            size_t argCount;
            /**< The allocated size of the arguments array. */
            size_t argCapacity;
        } funcCall;

        /**
         * @brief A declaration of a new variable.
         */
        struct {
            /**< The name of the new variable. */
            char *name;
            /**< the unique id of the variable (for generation) */
            int uniqueId;
            /**< True if declared with 'val' (immutable), false for 'var'. */
            bool isConstant;
            /**< The explicitly written type, or UNKNOWN if left for inference.
             */
            varType explicitType;
            /**< The expression assigning its initial value. */
            astNode *initializer;
        } varDef;

        /**
         * @brief An assignment to an existing variable.
         */
        struct {
            /**< The name of the variable being updated. */
            char *name;
            /**< The expression providing the new value. */
            astNode *value;
        } assign;

        /**
         * @brief A function definition.
         */
        struct {
            /**< The name of the function. */
            char *name;
            /**< True if the function was tagged as the main entry point. */
            bool isMain;
            /**< The explicitly stated return type, or UNKNOWN if omitted. */
            varType explicitReturnType;
            /**< Array of expected parameters. */
            astFuncParam *params;
            /**< The number of parameters. */
            size_t paramCount;
            /**< The code body of the function. */
            astNode *body;
            /**< Flag used by the code generator to prevent duplicate work. */
            bool alreadyGenerated;
        } funcDef;

        /**
         * @brief A return statement.
         */
        struct {
            /**< The expression to return. NULL if returning nothing. */
            astNode *value;
        } returnStmt;

        /**
         * @brief A int to double convertion
         */
        struct {
            /**< The expression to be converted */
            astNode *value;
        } int2doubleConv;
    } data;
};

/**
 * @brief Ensures a memory allocation succeeded.
 * @param ptr The pointer returned by malloc, calloc, or realloc.
 * Exits the program with code 99 if the pointer is NULL.
 */
void checkMalloc(void *ptr);

/**
 * @brief Initializes a new program root node.
 * @return A fresh AST_PROGRAM node.
 */
astNode *createProgramNode(void);

/**
 * @brief Initializes a new block node.
 * @return A fresh AST_BLOCK node. (parentIfForBody will be NULL)
 */
astNode *createBlockNode(void);

/**
 * @brief Appends a statement to an existing block.
 * @param blockNode The block to add the statement to.
 * @param statement The statement to add. The block takes ownership of this
 * node.
 * @return 0 on success, or -1 if arguments are invalid.
 */
int addStatementToBlock(astNode *blockNode, astNode *statement);

/**
 * @brief Creates a node for a function definition.
 * @param name The function's name. (The node takes ownership unless
 * IFJ_AST_NOOWN is defined).
 * @param isMain True if it's the main entry point.
 * @param params Array of parameters.
 * @param paramCount The number of items in the params array.
 * @param body The body of the function. (The node takes ownership).
 * @return A new AST_FUNC_DEF node.
 */
astNode *createFuncDefNode(char *name, bool isMain, astFuncParam *params,
                           size_t paramCount, astNode *body);

/**
 * @brief Creates a node for a variable declaration.
 * @param name The variable's name. (The node takes ownership unless
 * IFJ_AST_NOOWN is defined).
 * @param isConstant True for 'val', false for 'var'.
 * @param explicitType The declared type, or UNKNOWN if not provided.
 * @param initializer The initial value expression. (The node takes ownership).
 * @return A new AST_VAR_DEF node.
 */
astNode *createVarDefNode(char *name, bool isConstant, varType explicitType,
                          astNode *initializer);

/**
 * @brief Creates an AST literal from a lexer token.
 * @param token The lexer token to convert.
 * @return A new AST_LITERAL node, or NULL if the token type is unsupported.
 * Note: String values are copied, so the original token can be safely freed.
 */
astNode *createLiteralNode(Token *token);

/**
 * @brief Creates an if-statement or if-expression node.
 * @param condition The boolean expression to evaluate.
 * @param thenBlock The code to run if true.
 * @param elseBlock The code to run if false. Can be NULL for simple
 * if-statements.
 * @return A new AST_IF node.
 */
astNode *createIfNode(astNode *condition, astNode *thenBlock,
                      astNode *elseBlock);

/**
 * @brief Creates a while loop node.
 * @param condition The boolean expression to evaluate.
 * @param bodyBlock The code block to repeat.
 * @return A new AST_WHILE node.
 */
astNode *createWhileNode(astNode *condition, astNode *bodyBlock);

/**
 * @brief Creates a range-based for loop node.
 * @param iteratorName The name of the loop variable. (The node takes
 * ownership unless IFJ_AST_NOOWN is defined).
 * @param start The starting bound.
 * @param end The ending bound.
 * @param endInclusive True if 'to' is used, false for 'until'.
 * @param step The step expression. NULL defaults to a step of 1.
 * @param body The code body of the loop (must be type AST_BLOCK).
 * @return A new AST_FOR node.
 *
 * will set the parentIfForBody of body to point to the created node
 */
astNode *createForNode(char *iteratorName, astNode *start, astNode *end,
                       bool endInclusive, astNode *step, astNode *body);

/**
 * @brief Appends an additional guard condition to a for loop.
 * @param forNode The loop to modify.
 * @param guard The condition to add. The loop takes ownership.
 * @return 0 on success, or -1 if arguments are invalid.
 */
int addGuardToFor(astNode *forNode, astNode *guard);

/**
 * @brief Creates a binary operation node.
 * @param op The operator token.
 * @param left The left operand. (The node takes ownership).
 * @param right The right operand. (The node takes ownership).
 * @return A new AST_BINOP node.
 */
astNode *createBinopNode(astBinopType op, astNode *left, astNode *right);

/**
 * @brief Creates a unary operation node.
 * @param op The operator token.
 * @param operand The expression being modified. (The node takes ownership).
 * @return A new AST_UNOP node.
 */
astNode *createUnopNode(astUnopType op, astNode *operand);

/**
 * @brief Creates an identifier reference node.
 * @param name The variable or parameter name. (The node takes ownership unless
 * IFJ_AST_NOOWN is defined).
 * @return A new AST_IDENTIFIER node.
 */
astNode *createIdentifierNode(char *name);

/**
 * @brief Creates a function call node.
 * @param name The name of the function to call. (The node takes ownership
 * unless IFJ_AST_NOOWN is defined).
 * @return A new AST_FUNC_CALL node.
 */
astNode *createFuncCallNode(char *name);

/**
 * @brief Appends an argument to a function call.
 * @param callNode The function call node to modify.
 * @param argument The argument expression to add. The node takes ownership.
 * @return 0 on success, or -1 if arguments are invalid.
 */
int addArgumentToCall(astNode *callNode, astNode *argument);

/**
 * @brief Creates an empty literal representing Unit (written as `()`).
 * @return A new AST_LITERAL node configured as Unit.
 */
astNode *createUnitNode(void);

/**
 * @brief Creates an assignment node to update an existing variable.
 * @param name The variable being updated. (The node takes ownership unless
 * IFJ_AST_NOOWN is defined).
 * @param value The new value expression. (The node takes ownership).
 * @return A new AST_ASSIGN node.
 */
astNode *createAssignNode(char *name, astNode *value);

/**
 * @brief Creates a return statement node.
 * @param value The expression to return. Can be NULL for a bare return.
 * @return A new AST_RETURN node.
 */
astNode *createReturnNode(astNode *value);

/**
 * @brief Creates a int to double convertion node.
 * @param
 * @return A new AST_INT2DOUBLE node.
 */
astNode *createInt2DoubleNode(astNode *convertee);

/**
 * @brief Recursively frees an AST node and all its children.
 * @param root The starting node to free. Also handles freeing associated
 * strings unless IFJ_AST_NOOWN is defined.
 */
void freeAst(astNode *root);

/**
 * @brief Appends a parsed function definition to the program root.
 * @param programNode The program root node.
 * @param funcDef The function to add. The program takes ownership.
 * @return 0 on success, or -1 if arguments are invalid.
 */
int addFunctionToProgram(astNode *programNode, astNode *funcDef);

#endif // AST_H
