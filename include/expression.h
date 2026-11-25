/**
 * @file expression.h
 * @brief Expression stack and precedence table.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 */

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "ast.h"
#include <stddef.h>

/** @brief Symbols used by the expression parser. */
typedef enum {
    EXPR_END,         /**< End ($) */
    EXPR_OR,          /**< || */
    EXPR_AND,         /**< && */
    EXPR_EQUALITY,    /**< ==, != */
    EXPR_COMPARISON,  /**< <, <=, >, >= */
    EXPR_ADD,         /**< Binary +, - */
    EXPR_MULTIPLY,    /**< *, / */
    EXPR_UNARY,       /**< Unary +, -, ! */
    EXPR_LPAREN,      /**< ( */
    EXPR_RPAREN,      /**< ) */
    EXPR_VALUE,       /**< Literal, name or call */
    EXPR_NONTERMINAL, /**< Parsed expression (E) */
    EXPR_INVALID      /**< Unknown symbol */
} ExprSymbol;

/** @brief Actions from the precedence table. */
typedef enum {
    PREC_ERROR = 0, /**< Invalid pair. */
    PREC_SHIFT,     /**< < : shift the next token. */
    PREC_REDUCE,    /**< > : reduce an expression. */
    PREC_MATCH,     /**< = : match parentheses. */
    PREC_ACCEPT     /**< Expression finished. */
} PrecRelation;

/** @brief One stack item and its AST node. */
typedef struct {
    ExprSymbol symbol;
    TokenType tokenType;
    astNode *node;
} ExprStackItem;

/** @brief Expression stack. */
typedef struct {
    ExprStackItem *items;
    size_t count;
    size_t capacity;
} ExprStack;

/**
 * @brief Initializes the stack.
 * @param stack Stack to initialize.
 */
void exprStackInit(ExprStack *stack);

/**
 * @brief Frees the stack and remaining AST nodes.
 * @param stack Stack to free.
 */
void exprStackDestroy(ExprStack *stack);

/**
 * @brief Pushes an item onto the stack.
 * @param stack Expression stack.
 * @param item Item to push.
 * @return 0 on success, -1 on memory allocation err
 */
int exprStackPush(ExprStack *stack, ExprStackItem item);

/**
 * @brief Removes the top item.
 * @param stack Expression stack.
 * @param item Stores the removed item, including its AST node.
 * @return False if the stack is empty.
 */
bool exprStackPop(ExprStack *stack, ExprStackItem *item);

/**
 * @brief Returns the top item without removing it.
 * @param stack Expression stack.
 * @return Top item, or NULL if empty.
 */
const ExprStackItem *exprStackTop(const ExprStack *stack);

/**
 * @brief Finds the last terminal, skipping E.
 * @param stack Expression stack.
 * @return Terminal item, or NULL if none.
 */
const ExprStackItem *exprStackTopTerminal(const ExprStack *stack);

/**
 * @brief Converts a token to an expression symbol.
 * @param tokenType Lexer token type.
 * @param isUnary True for unary + or -.
 * @return Expression symbol, or EXPR_INVALID.
 */
ExprSymbol exprSymbolFromToken(TokenType tokenType, bool isUnary);

/**
 * @brief Returns the precedence action.
 * @param stackTerminal Last terminal on the stack.
 * @param input Next expression symbol.
 * @return SHIFT, REDUCE, MATCH, ACCEPT, or ERROR.
 */
PrecRelation exprPrecedence(ExprSymbol stackTerminal, ExprSymbol input);

#endif // EXPRESSION_H
