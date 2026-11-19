/**
 * @file expression.c
 * @brief Expression stack and precedence table.
 */

#include "expression.h"
#include <stdlib.h>

/**
 * @brief Initializes the stack.
 * @param stack Stack to initialize.
 */
void exprStackInit(ExprStack *stack) {
    stack->items = NULL;
    stack->count = 0;
    stack->capacity = 0;
}

/**
 * @brief Frees the stack and remaining AST nodes.
 * @param stack Stack to free.
 */
void exprStackDestroy(ExprStack *stack) {
    /** free any remaining AST nodes */
    for (size_t i = 0; i < stack->count; i++) {
        freeAst(stack->items[i].node);
    }

    free(stack->items);
    exprStackInit(stack);
}

/**
 * @brief Pushes an item onto the stack.
 * @param stack Expression stack.
 * @param item Item to push.
 */
void exprStackPush(ExprStack *stack, ExprStackItem item) {
    /** grow the stack when full */
    if (stack->count == stack->capacity) {
        size_t newCapacity = stack->capacity == 0 ? 4 : stack->capacity * 2;
        ExprStackItem *newItems =
            realloc(stack->items, newCapacity * sizeof(*newItems));
        checkMalloc(newItems);
        stack->items = newItems;
        stack->capacity = newCapacity;
    }

    stack->items[stack->count++] = item;
}

/**
 * @brief Removes the top item.
 * @param stack Expression stack.
 * @param item Stores the removed item and its AST node.
 * @return False if empty.
 */
bool exprStackPop(ExprStack *stack, ExprStackItem *item) {
    if (stack->count == 0) {
        return false;
    }

    *item = stack->items[--stack->count];
    return true;
}

/**
 * @brief Returns the top item.
 * @param stack Expression stack.
 * @return Top item, or NULL if empty.
 */
const ExprStackItem *exprStackTop(const ExprStack *stack) {
    if (stack->count == 0) {
        return NULL;
    }

    return &stack->items[stack->count - 1];
}

/**
 * @brief Finds the last terminal, skipping E.
 * @param stack Expression stack.
 * @return Terminal item, or NULL if none.
 */
const ExprStackItem *exprStackTopTerminal(const ExprStack *stack) {
    /** skip E */
    for (size_t i = stack->count; i > 0; i--) {
        if (stack->items[i - 1].symbol != EXPR_NONTERMINAL) {
            return &stack->items[i - 1];
        }
    }

    return NULL;
}

/**
 * @brief Converts a token to an expression symbol.
 * @param tokenType Lexer token type.
 * @param isUnary True for unary + or -.
 * @return Expression symbol, or EXPR_INVALID.
 */
ExprSymbol exprSymbolFromToken(TokenType tokenType, bool isUnary) {
    switch (tokenType) {
    case TOKEN_EOF:
    case TOKEN_NEWLINE:
        return EXPR_END;
    case TOKEN_OR:
        return EXPR_OR;
    case TOKEN_AND:
        return EXPR_AND;
    case TOKEN_EQUAL:
    case TOKEN_NOT_EQUAL:
        return EXPR_EQUALITY;
    case TOKEN_LESS:
    case TOKEN_LESS_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
        return EXPR_COMPARISON;
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        /** +/- can be unary or binary */
        return isUnary ? EXPR_UNARY : EXPR_ADD;
    case TOKEN_MULTIPLY:
    case TOKEN_DIVIDE:
        return EXPR_MULTIPLY;
    case TOKEN_NOT:
        return EXPR_UNARY;
    case TOKEN_LPAREN:
        return EXPR_LPAREN;
    case TOKEN_RPAREN:
        return EXPR_RPAREN;
    case TOKEN_IDENTIFIER:
    case TOKEN_INT_LITERAL:
    case TOKEN_DOUBLE_LITERAL:
    case TOKEN_STRING_LITERAL:
    case TOKEN_BOOL_LITERAL:
        return EXPR_VALUE;
    default:
        return EXPR_INVALID;
    }
}

/*
 * row = stack symbol, column = next symbol
 * missing entries = PREC_ERROR
 * priority (low -> high): ||, &&, ==, <, +, *, unary
 */
static const PrecRelation precedenceTable[EXPR_VALUE + 1][EXPR_VALUE + 1] =
    {
        [EXPR_END] =
            {
                /** $ = start/end */
                [EXPR_END] = PREC_ACCEPT,
                [EXPR_OR] = PREC_SHIFT,
                [EXPR_AND] = PREC_SHIFT,
                [EXPR_EQUALITY] = PREC_SHIFT,
                [EXPR_COMPARISON] = PREC_SHIFT,
                [EXPR_ADD] = PREC_SHIFT,
                [EXPR_MULTIPLY] = PREC_SHIFT,
                [EXPR_UNARY] = PREC_SHIFT,
                [EXPR_LPAREN] = PREC_SHIFT,
                [EXPR_VALUE] = PREC_SHIFT,
            },
        [EXPR_OR] =
            {
                /** lowest priority */
                [EXPR_END] = PREC_REDUCE,
                [EXPR_OR] = PREC_REDUCE,
                [EXPR_AND] = PREC_SHIFT,
                [EXPR_EQUALITY] = PREC_SHIFT,
                [EXPR_COMPARISON] = PREC_SHIFT,
                [EXPR_ADD] = PREC_SHIFT,
                [EXPR_MULTIPLY] = PREC_SHIFT,
                [EXPR_UNARY] = PREC_SHIFT,
                [EXPR_LPAREN] = PREC_SHIFT,
                [EXPR_RPAREN] = PREC_REDUCE,
                [EXPR_VALUE] = PREC_SHIFT,
            },
        [EXPR_AND] =
            {
                [EXPR_END] = PREC_REDUCE,
                [EXPR_OR] = PREC_REDUCE,
                [EXPR_AND] = PREC_REDUCE,
                [EXPR_EQUALITY] = PREC_SHIFT,
                [EXPR_COMPARISON] = PREC_SHIFT,
                [EXPR_ADD] = PREC_SHIFT,
                [EXPR_MULTIPLY] = PREC_SHIFT,
                [EXPR_UNARY] = PREC_SHIFT,
                [EXPR_LPAREN] = PREC_SHIFT,
                [EXPR_RPAREN] = PREC_REDUCE,
                [EXPR_VALUE] = PREC_SHIFT,
            },
        [EXPR_EQUALITY] =
            {
                /** no chained comparisons */
                [EXPR_END] = PREC_REDUCE,
                [EXPR_OR] = PREC_REDUCE,
                [EXPR_AND] = PREC_REDUCE,
                [EXPR_ADD] = PREC_SHIFT,
                [EXPR_MULTIPLY] = PREC_SHIFT,
                [EXPR_UNARY] = PREC_SHIFT,
                [EXPR_LPAREN] = PREC_SHIFT,
                [EXPR_RPAREN] = PREC_REDUCE,
                [EXPR_VALUE] = PREC_SHIFT,
            },
        [EXPR_COMPARISON] =
            {
                /** no chained comparisons */
                [EXPR_END] = PREC_REDUCE,
                [EXPR_OR] = PREC_REDUCE,
                [EXPR_AND] = PREC_REDUCE,
                [EXPR_ADD] = PREC_SHIFT,
                [EXPR_MULTIPLY] = PREC_SHIFT,
                [EXPR_UNARY] = PREC_SHIFT,
                [EXPR_LPAREN] = PREC_SHIFT,
                [EXPR_RPAREN] = PREC_REDUCE,
                [EXPR_VALUE] = PREC_SHIFT,
            },
        [EXPR_ADD] =
            {
                /** + then * -> shift */
                [EXPR_END] = PREC_REDUCE,
                [EXPR_OR] = PREC_REDUCE,
                [EXPR_AND] = PREC_REDUCE,
                [EXPR_EQUALITY] = PREC_REDUCE,
                [EXPR_COMPARISON] = PREC_REDUCE,
                [EXPR_ADD] = PREC_REDUCE,
                [EXPR_MULTIPLY] = PREC_SHIFT,
                [EXPR_UNARY] = PREC_SHIFT,
                [EXPR_LPAREN] = PREC_SHIFT,
                [EXPR_RPAREN] = PREC_REDUCE,
                [EXPR_VALUE] = PREC_SHIFT,
            },
        [EXPR_MULTIPLY] =
            {
                /** * then + -> reduce */
                [EXPR_END] = PREC_REDUCE,
                [EXPR_OR] = PREC_REDUCE,
                [EXPR_AND] = PREC_REDUCE,
                [EXPR_EQUALITY] = PREC_REDUCE,
                [EXPR_COMPARISON] = PREC_REDUCE,
                [EXPR_ADD] = PREC_REDUCE,
                [EXPR_MULTIPLY] = PREC_REDUCE,
                [EXPR_UNARY] = PREC_SHIFT,
                [EXPR_LPAREN] = PREC_SHIFT,
                [EXPR_RPAREN] = PREC_REDUCE,
                [EXPR_VALUE] = PREC_SHIFT,
            },
        [EXPR_UNARY] =
            {
                /** - -x: start from right */
                [EXPR_END] = PREC_REDUCE,
                [EXPR_OR] = PREC_REDUCE,
                [EXPR_AND] = PREC_REDUCE,
                [EXPR_EQUALITY] = PREC_REDUCE,
                [EXPR_COMPARISON] = PREC_REDUCE,
                [EXPR_ADD] = PREC_REDUCE,
                [EXPR_MULTIPLY] = PREC_REDUCE,
                [EXPR_UNARY] = PREC_SHIFT,
                [EXPR_LPAREN] = PREC_SHIFT,
                [EXPR_RPAREN] = PREC_REDUCE,
                [EXPR_VALUE] = PREC_SHIFT,
            },
        [EXPR_LPAREN] =
            {
                /** ( and ) -> match */
                [EXPR_OR] = PREC_SHIFT,
                [EXPR_AND] = PREC_SHIFT,
                [EXPR_EQUALITY] = PREC_SHIFT,
                [EXPR_COMPARISON] = PREC_SHIFT,
                [EXPR_ADD] = PREC_SHIFT,
                [EXPR_MULTIPLY] = PREC_SHIFT,
                [EXPR_UNARY] = PREC_SHIFT,
                [EXPR_LPAREN] = PREC_SHIFT,
                [EXPR_RPAREN] = PREC_MATCH,
                [EXPR_VALUE] = PREC_SHIFT,
            },
        [EXPR_RPAREN] =
            {
                /** finish () first */
                [EXPR_END] = PREC_REDUCE,
                [EXPR_OR] = PREC_REDUCE,
                [EXPR_AND] = PREC_REDUCE,
                [EXPR_EQUALITY] = PREC_REDUCE,
                [EXPR_COMPARISON] = PREC_REDUCE,
                [EXPR_ADD] = PREC_REDUCE,
                [EXPR_MULTIPLY] = PREC_REDUCE,
                [EXPR_RPAREN] = PREC_REDUCE,
            },
        [EXPR_VALUE] =
            {
                /** value -> E */
                [EXPR_END] = PREC_REDUCE,
                [EXPR_OR] = PREC_REDUCE,
                [EXPR_AND] = PREC_REDUCE,
                [EXPR_EQUALITY] = PREC_REDUCE,
                [EXPR_COMPARISON] = PREC_REDUCE,
                [EXPR_ADD] = PREC_REDUCE,
                [EXPR_MULTIPLY] = PREC_REDUCE,
                [EXPR_RPAREN] = PREC_REDUCE,
            },
};

/**
 * @brief Returns the precedence action.
 * @param stackTerminal Last terminal on the stack.
 * @param input Next expression symbol.
 * @return SHIFT, REDUCE, MATCH, ACCEPT, or ERROR.
 */
PrecRelation exprPrecedence(ExprSymbol stackTerminal, ExprSymbol input) {
    if (stackTerminal < EXPR_END || stackTerminal > EXPR_VALUE ||
        input < EXPR_END || input > EXPR_VALUE) {
        return PREC_ERROR;
    }

    return precedenceTable[stackTerminal][input];
}
