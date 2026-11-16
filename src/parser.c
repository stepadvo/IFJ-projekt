/**
 * @file parser.c
 * @brief Recursive-descent parser for the currently supported IFJ26 syntax.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 *
 * Parses the mandatory prolog, function headers, and single-line expressions.
 * Indented blocks and function calls will be added later.
 */

#include "parser.h"
#include "compiler.h"
#include "expression.h"
#include "lexer.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Lexer *lexer = NULL;
static Token currentToken = {0};

/**
 * @brief Reads the next token.
 * @return True on success, false on error.
 */
static bool advance(void) {
    LexerStatus status = getToken(lexer, &currentToken);

    /** set lexer/internal error */
    if (status == LEXER_OK)
        return true;
    if (status == LEXER_INTERNAL_ERROR)
        ERRCODE = 99;
    else if (ERRCODE == 0)
        ERRCODE = 1;
    return false;
}

/**
 * @brief Reports a syntax error.
 * @param message Error message to print.
 */
static void syntaxError(const char *message) {
    fprintf(stderr, "Syntax error at line %zu: %s\n", currentToken.line,
            message);
    if (ERRCODE == 0) {
        ERRCODE = 2;
    }

    return;
}

/**
 * @brief Checks a token and moves to the next one.
 * @param expected Expected token type.
 * @param errMessage Message if the token does not match.
 * @return True on success, false on error.
 */
static bool match(TokenType expected, const char *errMessage) {
    if (currentToken.type != expected) {
        syntaxError(errMessage);
        return false;
    }

    return advance();
}

/**
 * @brief Copies the current identifier.
 * @return New name, owned by the caller or AST.
 */
static char *copyName(void) {
    /** make space for the name and '\0' */
    size_t length = currentToken.attribute.text.length;
    char *name = malloc(length + 1);
    checkMalloc(name);

    memcpy(name, currentToken.attribute.text.data, length + 1);
    return name;
}

/**
 * @brief Parses "import ifj26.*".
 * @return True if valid, false on error.
 */
static bool parseProlog(void) {
    /** expected tokens */
    const TokenType prolog[] = {TOKEN_KW_IMPORT, TOKEN_IDENTIFIER, TOKEN_DOT,
                                TOKEN_MULTIPLY, TOKEN_NEWLINE};
    /** corresponding errors */
    const char *errMsg[] = {
        "expected 'import'", "expected 'ifj26' after 'import'",
        "expected '.' after 'ifj26'", "expected '*' after '.'",
        "expected '\\n' after '*'"};

    for (size_t i = 0; i < 5; i++) {
        if (i == 1 && currentToken.type == TOKEN_IDENTIFIER) {
            if (strcmp(currentToken.attribute.text.data, "ifj26")) {
                syntaxError(errMsg[i]);
                return false;
            }
        }

        if (!match(prolog[i], errMsg[i]))
            return false;
    }
    return true;
}

/**
 * @brief Parses an optional @main annotation.
 * @param isMain Set to true if @main is present.
 * @return True if valid or missing, false on error.
 */
static bool parseMainAnnotation(bool *isMain) {
    *isMain = false;

    /** no annotation */
    if (currentToken.type != TOKEN_AT) {
        return true;
    }

    size_t atLine = currentToken.line;
    size_t atColumn = currentToken.column;

    if (!advance()) {
        return false;
    }

    /** check @main with no space */
    if (currentToken.type != TOKEN_IDENTIFIER ||
        strcmp(currentToken.attribute.text.data, "main") != 0 ||
        currentToken.line != atLine || currentToken.column != atColumn + 1) {
        syntaxError("expected '@main' without whitespace");
        return false;
    }

    *isMain = true;

    if (!advance()) {
        return false;
    }

    /** @main may be on its own line */
    if (currentToken.type == TOKEN_NEWLINE) {
        return advance();
    }

    return true;
}

/**
 * @brief Reads a type keyword.
 * @param type Stores the parsed type.
 * @return True on success, false on error.
 */
static bool parseType(varType *type) {
    /** token type -> varType */
    switch (currentToken.type) {
    case TOKEN_KW_INT:
        *type = INT;
        break;
    case TOKEN_KW_DOUBLE:
        *type = DOUBLE;
        break;
    case TOKEN_KW_STRING:
        *type = STRING;
        break;
    case TOKEN_KW_BOOLEAN:
        *type = BOOL;
        break;
    case TOKEN_KW_UNIT:
        *type = UNIT;
        break;
    default:
        syntaxError("expected type");
        return false;
    }

    return advance();
}

/**
 * @brief Frees a parameter array and its names.
 * @param params Parameter array.
 * @param count Number of stored parameters.
 */
static void freeParameters(astFuncParam *params, size_t count) {
    /** free copied names */
    for (size_t i = 0; i < count; i++) {
        free(params[i].name);
    }

    free(params);
}

/**
 * @brief Parses parameters (name: Type, ...).
 * @param params Stores the parameter array.
 * @param count Stores the number of parameters.
 * @return True on success, false on error.
 */
static bool parseParameters(astFuncParam **params, size_t *count) {
    size_t capacity = 0;

    /** empty list */
    if (currentToken.type == TOKEN_RPAREN) {
        return true;
    }

    /** read name : Type */
    while (true) {
        if (currentToken.type != TOKEN_IDENTIFIER) {
            syntaxError("expected parameter name");
            return false;
        }

        /** save name before advance */
        char *name = copyName();

        if (!advance() ||
            !match(TOKEN_COLON, "expected ':' after parameter name")) {
            free(name);
            return false;
        }

        varType type;
        if (!parseType(&type)) {
            free(name);
            return false;
        }

        /** grow parameter array if needed */
        if (*count == capacity) {
            size_t newCapacity = capacity == 0 ? 4 : capacity * 2;
            astFuncParam *newParams =
                realloc(*params, newCapacity * sizeof(**params));
            checkMalloc(newParams);
            *params = newParams;
            capacity = newCapacity;
        }

        /** save parameter */
        (*params)[*count] = (astFuncParam){.name = name, .type = type};
        (*count)++;

        /** comma -> next parameter */
        if (currentToken.type != TOKEN_COMMA) {
            return true;
        }
        if (!advance()) {
            return false;
        }
        /** trailing comma needs a line break */
        if (currentToken.type == TOKEN_RPAREN) {
            if (currentToken.newlineBefore) {
                return true;
            }
            syntaxError("expected parameter after ','");
            return false;
        }
    }
}

/** @brief Parses an expression. @return Expression AST, or NULL on error. */
static astNode *parseExpression(void);

/**
 * @brief Parses a function call after its name.
 * @param name Function name, owned by the new node.
 * @return Call AST, or NULL on error.
 */
static astNode *parseFunctionCall(char *name) {
    astNode *call = createFuncCallNode(name);
    if (call == NULL) {
        free(name);
        return NULL;
    }

    if (!match(TOKEN_LPAREN, "expected '(' after function name")) {
        freeAst(call);
        return NULL;
    }

    /** read each argument as an expression */
    while (currentToken.type != TOKEN_RPAREN) {
        astNode *argument = parseExpression();
        if (argument == NULL) {
            freeAst(call);
            return NULL;
        }
        if (addArgumentToCall(call, argument) != 0) {
            freeAst(argument);
            freeAst(call);
            return NULL;
        }

        if (currentToken.type != TOKEN_COMMA) {
            break;
        }
        if (!advance()) {
            freeAst(call);
            return NULL;
        }
        /** a trailing comma needs a line break */
        if (currentToken.type == TOKEN_RPAREN && !currentToken.newlineBefore) {
            syntaxError("expected argument after ','");
            freeAst(call);
            return NULL;
        }
    }

    if (!match(TOKEN_RPAREN, "expected ')' after arguments")) {
        freeAst(call);
        return NULL;
    }
    return call;
}

/**
 * @brief Creates a variable or call node from a saved name.
 * @param name Name to give to the AST.
 * @return Value AST, or NULL on error.
 */
static astNode *parseIdentifierValue(char *name) {
    if (currentToken.type == TOKEN_LPAREN) {
        return parseFunctionCall(name);
    }
    astNode *node = createIdentifierNode(name);
    if (node == NULL) {
        free(name);
    }
    return node;
}

/**
 * @brief Checks if a symbol is a binary operator.
 * @param symbol Expression symbol.
 * @return True for binary operators.
 */
static bool isBinarySymbol(ExprSymbol symbol) {
    return symbol == EXPR_OR || symbol == EXPR_AND ||
           symbol == EXPR_EQUALITY || symbol == EXPR_COMPARISON ||
           symbol == EXPR_ADD || symbol == EXPR_MULTIPLY;
}

/**
 * @brief Finds the table symbol for the current token.
 * @param expectsOperand True when the next token should start a value.
 * @param openParens Number of unmatched '(' on the stack.
 * @return Symbol for the precedence table.
 */
static ExprSymbol expressionInputSymbol(bool expectsOperand, size_t openParens) {
    /** these tokens end the expression only outside parentheses */
    if (openParens == 0) {
        switch (currentToken.type) {
        case TOKEN_COMMA:
        case TOKEN_RPAREN:
        case TOKEN_SEMICOLON:
        case TOKEN_KW_THEN:
        case TOKEN_KW_DO:
            return EXPR_END;
        default:
            break;
        }
    }

    return exprSymbolFromToken(currentToken.type, expectsOperand);
}

/**
 * @brief Creates an AST node for the current value.
 * @return Value node, or NULL if the token is not a value.
 */
static astNode *expressionValueNode(void) {
    if (currentToken.type == TOKEN_IDENTIFIER) {
        char *name = copyName();
        if (!advance()) {
            free(name);
            return NULL;
        }
        return parseIdentifierValue(name);
    }

    astNode *node = createLiteralNode(&currentToken);
    if (node != NULL && !advance()) {
        freeAst(node);
        return NULL;
    }
    return node;
}

/**
 * @brief Reduces the last value or operation to E.
 * @param stack Expression stack.
 * @return True if a valid part was reduced.
 */
static bool reduceExpression(ExprStack *stack) {
    size_t count = stack->count;
    if (count == 0) {
        return false;
    }

    ExprStackItem *last = &stack->items[count - 1];

    /** value -> E; the AST node is already there */
    if (last->symbol == EXPR_VALUE) {
        last->symbol = EXPR_NONTERMINAL;
        return true;
    }

    if (last->symbol != EXPR_NONTERMINAL || count < 2) {
        return false;
    }

    ExprStackItem *op = &stack->items[count - 2];

    /** unary E -> E */
    if (op->symbol == EXPR_UNARY) {
        astNode *node = createUnopNode(op->tokenType, last->node);
        if (node == NULL) {
            return false;
        }
        last->node = NULL;
        stack->count -= 2;
        exprStackPush(stack, (ExprStackItem){.symbol = EXPR_NONTERMINAL,
                                            .node = node});
        return true;
    }

    /** E operator E -> E */
    if (count < 3 || !isBinarySymbol(op->symbol) ||
        stack->items[count - 3].symbol != EXPR_NONTERMINAL) {
        return false;
    }

    ExprStackItem *left = &stack->items[count - 3];
    astNode *node = createBinopNode(op->tokenType, left->node, last->node);
    if (node == NULL) {
        return false;
    }
    left->node = NULL;
    last->node = NULL;
    stack->count -= 3;
    exprStackPush(stack, (ExprStackItem){.symbol = EXPR_NONTERMINAL,
                                        .node = node});
    return true;
}

/**
 * @brief Removes matching parentheses from the stack.
 * @param stack Expression stack.
 * @return True if the parentheses are valid.
 */
static bool reduceParentheses(ExprStack *stack) {
    size_t count = stack->count;
    if (count == 0) {
        return false;
    }

    /** () is the Unit value */
    if (stack->items[count - 1].symbol == EXPR_LPAREN) {
        stack->count--;
        exprStackPush(stack, (ExprStackItem){.symbol = EXPR_NONTERMINAL,
                                            .node = createUnitNode()});
        return true;
    }

    /** ( E ) -> E */
    if (count < 2 || stack->items[count - 1].symbol != EXPR_NONTERMINAL ||
        stack->items[count - 2].symbol != EXPR_LPAREN) {
        return false;
    }

    astNode *node = stack->items[count - 1].node;
    stack->items[count - 1].node = NULL;
    stack->count -= 2;
    exprStackPush(stack, (ExprStackItem){.symbol = EXPR_NONTERMINAL,
                                        .node = node});
    return true;
}

/**
 * @brief Builds an expression AST using the precedence table.
 * @param firstValue Already-read first value, or NULL.
 * @return Expression AST, or NULL on error.
 */
static astNode *parseExpressionFromValue(astNode *firstValue) {
    ExprStack stack;
    exprStackInit(&stack);
    exprStackPush(&stack, (ExprStackItem){.symbol = EXPR_END});

    /** used when a statement already read its first name */
    if (firstValue != NULL) {
        exprStackPush(&stack, (ExprStackItem){.symbol = EXPR_VALUE,
                                              .node = firstValue});
    }

    size_t openParens = 0;
    bool expectsOperand = firstValue == NULL;

    while (true) {
        ExprSymbol input = expressionInputSymbol(expectsOperand, openParens);
        const ExprStackItem *terminal = exprStackTopTerminal(&stack);
        if (terminal == NULL || input == EXPR_INVALID) {
            syntaxError("invalid expression");
            break;
        }

        PrecRelation action = exprPrecedence(terminal->symbol, input);

        if (action == PREC_SHIFT) {
            ExprStackItem item = {.symbol = input,
                                  .tokenType = currentToken.type};
            if (input == EXPR_VALUE) {
                item.node = expressionValueNode();
                if (item.node == NULL) {
                    syntaxError("expected value");
                    break;
                }
            }
            /** values advance inside expressionValueNode() */
            if (input != EXPR_VALUE && !advance()) {
                break;
            }
            exprStackPush(&stack, item);

            if (input == EXPR_LPAREN) {
                openParens++;
            }
            expectsOperand = input != EXPR_VALUE;
        } else if (action == PREC_REDUCE) {
            if (!reduceExpression(&stack)) {
                syntaxError("invalid expression");
                break;
            }
        } else if (action == PREC_MATCH) {
            if (!reduceParentheses(&stack)) {
                syntaxError("invalid parentheses");
                break;
            }
            openParens--;
            expectsOperand = false;
            if (!advance()) {
                break;
            }
        } else if (action == PREC_ACCEPT) {
            /** only $ E is a complete expression */
            if (stack.count != 2 ||
                stack.items[1].symbol != EXPR_NONTERMINAL) {
                syntaxError("expected expression");
                break;
            }

            astNode *result = stack.items[1].node;
            stack.items[1].node = NULL;
            exprStackDestroy(&stack);
            return result;
        } else {
            syntaxError("invalid expression");
            break;
        }
    }

    exprStackDestroy(&stack);
    return NULL;
}

/** @brief Parses an expression from the current token. */
static astNode *parseExpression(void) {
    return parseExpressionFromValue(NULL);
}

/**
 * @brief Parses a function and creates its AST node.
 * @param isMain True if the function has @main.
 * @return Function node, or NULL on error.
 */
static astNode *parseFunctionDef(bool isMain) {
    if (!match(TOKEN_KW_DEF, "expected 'def'")) {
        return NULL;
    }

    if (currentToken.type != TOKEN_IDENTIFIER) {
        syntaxError("expected function name");
        return NULL;
    }

    /** save name and parameters */
    char *name = copyName();
    astFuncParam *params = NULL;
    size_t paramCount = 0;

    if (!advance() || !match(TOKEN_LPAREN, "expected '('") ||
        !parseParameters(&params, &paramCount) ||
        !match(TOKEN_RPAREN, "expected ')' after parameters")) {
        freeParameters(params, paramCount);
        free(name);
        return NULL;
    }

    /** return type is optional */
    varType returnType = UNKNOWN;
    if (currentToken.type == TOKEN_COLON) {
        if (!advance() || !parseType(&returnType)) {
            freeParameters(params, paramCount);
            free(name);
            return NULL;
        }
    }

    if (!match(TOKEN_ASSIGN, "expected '=' after function header")) {
        freeParameters(params, paramCount);
        free(name);
        return NULL;
    }

    /** function body = expression */
    astNode *body = parseExpression();
    if (body == NULL) {
        freeParameters(params, paramCount);
        free(name);
        return NULL;
    }

    /** body must end here */
    if (currentToken.type != TOKEN_NEWLINE && currentToken.type != TOKEN_EOF) {
        syntaxError("expected end of function body");
        freeAst(body);
        freeParameters(params, paramCount);
        free(name);
        return NULL;
    }

    if (currentToken.type == TOKEN_NEWLINE && !advance()) {
        freeAst(body);
        freeParameters(params, paramCount);
        free(name);
        return NULL;
    }

    /** build function AST */
    astNode *function =
        createFuncDefNode(name, isMain, params, paramCount, body);
    if (function == NULL) {
        freeAst(body);
        freeParameters(params, paramCount);
        free(name);
        ERRCODE = 99;
        return NULL;
    }

    function->data.funcDef.explicitReturnType = returnType;
    return function;
}

/**
 * @brief Parses the program and builds its AST.
 * @return Program AST, or NULL on error. Caller frees it.
 */
astNode *parse(void) {
    /** initialize lexer */
    currentToken = (Token){0};
    lexer = lexerCreate(stdin);
    if (lexer == NULL) {
        fprintf(stderr, "Internal error: could not create lexer\n");
        ERRCODE = 99;
        return NULL;
    }
    astNode *root = NULL;

    /** parse all functions after import */
    if (advance() && parseProlog()) {
        root = createProgramNode();

        while (currentToken.type != TOKEN_EOF) {
            bool isMain = false;

            if (!parseMainAnnotation(&isMain)) {
                break;
            }
            if (currentToken.type != TOKEN_KW_DEF) {
                syntaxError("expected function definition");
                break;
            }

            astNode *function = parseFunctionDef(isMain);
            if (function == NULL) {
                break;
            }
            addFunctionToProgram(root, function);
        }
    }

    /** discard AST on error */
    if (ERRCODE != 0) {
        freeAst(root);
        root = NULL;
    }

    tokenFree(&currentToken);
    lexerDestroy(lexer);
    lexer = NULL;
    return root;
}
