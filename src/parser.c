/**
 * @file parser.c
 * @brief Recursive-descent parser for the currently supported IFJ26 syntax.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 *
 * Parses function definitions, expressions, calls, blocks and control
 * statements.
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
static size_t currentLineIndent = 0;

/**
 * @brief Reads the next token.
 * @return True on success, false on error.
 */
static bool advance(void) {
    LexerStatus status = getToken(lexer, &currentToken);

    /** the lexer reports its own errors through logErr() */
    if (status != LEXER_OK)
        return false;

    if (currentToken.startsLogicalLine)
        currentLineIndent = currentToken.indent;
    return true;
}

/**
 * @brief Reports a syntax error.
 * @param message Error message to print.
 */
static void syntaxError(const char *message) {
    logErr(ERR_SYNTAX, currentToken.line, currentToken.column, "%s\n", message);
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

/** @brief Indicates which tokens can end an expression in CONTROL syntax. */
typedef struct {
    bool stopAtElse;
    bool stopAtRange;
    bool stopAtBy;
    bool stopAtGuard;
} ExpressionStops;

/**
 * @brief Parses an expression.
 * @return Expression AST, or NULL on error.
 */
static astNode *parseExpression(void);
static astNode *parseExpressionWithStops(ExpressionStops stops);
static astNode *parseIf(size_t indent, bool isExpression,
                        ExpressionStops stops);
static astNode *parseStatement(size_t indent, ExpressionStops stops);

/**
 * @brief Checks whether the current identifier matches a contextual word.
 * @param word Word used by the for syntax.
 * @return True if the current token matches the word.
 */
static bool isIdentifierWord(const char *word) {
    return currentToken.type == TOKEN_IDENTIFIER &&
           strcmp(currentToken.attribute.text.data, word) == 0;
}

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
    return symbol == EXPR_OR || symbol == EXPR_AND || symbol == EXPR_EQUALITY ||
           symbol == EXPR_COMPARISON || symbol == EXPR_ADD ||
           symbol == EXPR_MULTIPLY;
}

/**
 * @brief Checks whether a token is an operator.
 * @param type Token to classify.
 * @return True for operator tokens.
 */
static bool isOperatorToken(TokenType type) {
    switch (type) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_MULTIPLY:
    case TOKEN_DIVIDE:
    case TOKEN_EQUAL:
    case TOKEN_NOT_EQUAL:
    case TOKEN_LESS:
    case TOKEN_LESS_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
    case TOKEN_NOT:
    case TOKEN_AND:
    case TOKEN_OR:
    case TOKEN_ASSIGN:
    case TOKEN_LEFT_ARROW:
    case TOKEN_DOT:
        return true;
    default:
        return false;
    }
}

/**
 * @brief Finds the table symbol for the current token.
 * @param expectsOperand True when the next token should start a value.
 * @param openParens Number of unmatched '(' on the stack.
 * @param stops Tokens that may end the expression.
 * @return Symbol for the precedence table.
 */
static ExprSymbol expressionInputSymbol(bool expectsOperand, size_t openParens,
                                        ExpressionStops stops) {
    /** these tokens end the expression only outside parentheses */
    if (openParens == 0) {
        switch (currentToken.type) {
        case TOKEN_COMMA:
        case TOKEN_RPAREN:
        case TOKEN_SEMICOLON:
        case TOKEN_KW_THEN:
        case TOKEN_KW_DO:
            return EXPR_END;
        case TOKEN_KW_ELSE:
            if (stops.stopAtElse)
                return EXPR_END;
            break;
        case TOKEN_KW_IF:
            if (!expectsOperand && stops.stopAtGuard)
                return EXPR_END;
            break;
        case TOKEN_IDENTIFIER:
            if (!expectsOperand) {
                if (stops.stopAtRange &&
                    (isIdentifierWord("to") || isIdentifierWord("until")))
                    return EXPR_END;
                if (stops.stopAtBy && isIdentifierWord("by"))
                    return EXPR_END;
            }
            break;
        default:
            break;
        }
    }

    /** if-expression is a value when an operand is expected */
    if (currentToken.type == TOKEN_KW_IF && expectsOperand)
        return EXPR_VALUE;

    return exprSymbolFromToken(currentToken.type, expectsOperand);
}

/**
 * @brief Creates an AST node for the current value.
 * @param stops Tokens that may end the expression.
 * @return Value node, or NULL if the token is not a value.
 */
static astNode *expressionValueNode(ExpressionStops stops) {
    if (currentToken.type == TOKEN_KW_IF)
        return parseIf(currentLineIndent, true, stops);
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
 * @brief Converts a lexer token to a binary AST operator.
 * @param type Lexer token type.
 * @param result Receives the corresponding AST operator.
 * @return True if the token represents a binary operator.
 */
static bool convertTokenToAstBinop(TokenType type, astBinopType *result) {
    switch (type) {
    case TOKEN_PLUS:
        *result = PLUS;
        break;
    case TOKEN_MINUS:
        *result = MINUS;
        break;
    case TOKEN_MULTIPLY:
        *result = MULTIPLY;
        break;
    case TOKEN_DIVIDE:
        *result = DIVIDE;
        break;
    case TOKEN_EQUAL:
        *result = EQUAL;
        break;
    case TOKEN_NOT_EQUAL:
        *result = NOT_EQUAL;
        break;
    case TOKEN_LESS:
        *result = LESS;
        break;
    case TOKEN_LESS_EQUAL:
        *result = LESS_EQUAL;
        break;
    case TOKEN_GREATER:
        *result = GREATER;
        break;
    case TOKEN_GREATER_EQUAL:
        *result = GREATER_EQUAL;
        break;
    case TOKEN_AND:
        *result = AND;
        break;
    case TOKEN_OR:
        *result = OR;
        break;
    default:
        return false;
    }

    return true;
}

/**
 * @brief Converts a lexer token to a unary AST operator.
 * @param type Lexer token type.
 * @param result Receives the corresponding AST operator.
 * @return True if the token represents a unary operator.
 */
static bool convertTokenToAstUnop(TokenType type, astUnopType *result) {
    switch (type) {
    case TOKEN_MINUS:
        *result = UNARY_MINUS;
        return true;
    case TOKEN_NOT:
        *result = NOT;
        return true;
    case TOKEN_PLUS:
        *result = UNARY_PLUS;
        return true;
    default:
        return false;
    }
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
        astUnopType unOp;
        if (!convertTokenToAstUnop(op->tokenType, &unOp)) {
            return false;
        }
        astNode *node = createUnopNode(unOp, last->node);
        if (node == NULL) {
            return false;
        }
        last->node = NULL;
        stack->count -= 2;
        exprStackPush(
            stack, (ExprStackItem){.symbol = EXPR_NONTERMINAL, .node = node});
        return true;
    }

    /** E operator E -> E */
    if (count < 3 || !isBinarySymbol(op->symbol) ||
        stack->items[count - 3].symbol != EXPR_NONTERMINAL) {
        return false;
    }

    ExprStackItem *left = &stack->items[count - 3];
    astBinopType binOp;
    if (!convertTokenToAstBinop(op->tokenType, &binOp)) {
        return false;
    }
    astNode *node = createBinopNode(binOp, left->node, last->node);
    if (node == NULL) {
        return false;
    }
    left->node = NULL;
    last->node = NULL;
    stack->count -= 3;
    exprStackPush(stack,
                  (ExprStackItem){.symbol = EXPR_NONTERMINAL, .node = node});
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
    exprStackPush(stack,
                  (ExprStackItem){.symbol = EXPR_NONTERMINAL, .node = node});
    return true;
}

/**
 * @brief Builds an expression AST using the precedence table.
 * @param firstValue Already-read first value, or NULL.
 * @param stops Tokens that may end the expression.
 * @return Expression AST, or NULL on error.
 */
static astNode *parseExpressionFromValue(astNode *firstValue,
                                         ExpressionStops stops) {
    ExprStack stack;
    exprStackInit(&stack);
    exprStackPush(&stack, (ExprStackItem){.symbol = EXPR_END});

    /** used when a statement already read its first name */
    if (firstValue != NULL) {
        exprStackPush(
            &stack, (ExprStackItem){.symbol = EXPR_VALUE, .node = firstValue});
    }

    size_t openParens = 0;
    bool expectsOperand = firstValue == NULL;
    bool endAfterBlockIf = false;

    while (true) {
        /** parseIf() may already have consumed the newline after a block */
        ExprSymbol input =
            endAfterBlockIf && openParens == 0
                ? EXPR_END
                : expressionInputSymbol(expectsOperand, openParens, stops);
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
                bool isIf = currentToken.type == TOKEN_KW_IF;
                size_t ifLine = currentToken.line;
                size_t ifIndent = currentLineIndent;
                item.node = expressionValueNode(stops);
                if (item.node == NULL) {
                    syntaxError("expected value");
                    break;
                }
                if (isIf && openParens == 0 && currentToken.startsLogicalLine &&
                    currentToken.line > ifLine &&
                    currentToken.indent <= ifIndent) {
                    endAfterBlockIf = true;
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
            if (stack.count != 2 || stack.items[1].symbol != EXPR_NONTERMINAL) {
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
    return parseExpressionWithStops((ExpressionStops){0});
}

/** @brief Parses an expression with contextual terminators. */
static astNode *parseExpressionWithStops(ExpressionStops stops) {
    return parseExpressionFromValue(NULL, stops);
}

/** @brief Parses an indented block. @param parentIndent Indent of its header.
 */
static astNode *parseBlock(size_t parentIndent);

/**
 * @brief Parses val or var.
 * @param isConstant True for val, false for var.
 * @param stops Tokens that may end the initializer.
 * @return Variable AST, or NULL on error.
 */
static astNode *parseVarDef(bool isConstant, ExpressionStops stops) {
    if (!advance()) {
        return NULL;
    }
    if (currentToken.type != TOKEN_IDENTIFIER) {
        syntaxError("expected variable name");
        return NULL;
    }

    char *name = copyName();
    if (!advance()) {
        free(name);
        return NULL;
    }

    /** optional : Type */
    varType type = UNKNOWN;
    if (currentToken.type == TOKEN_COLON) {
        if (!advance() || !parseType(&type)) {
            free(name);
            return NULL;
        }
    }
    if (!match(TOKEN_ASSIGN, "expected '=' after variable name")) {
        free(name);
        return NULL;
    }

    astNode *value = parseExpressionWithStops(stops);
    if (value == NULL) {
        free(name);
        return NULL;
    }
    astNode *node = createVarDefNode(name, isConstant, type, value);
    if (node == NULL) {
        free(name);
        freeAst(value);
    }
    return node;
}

/** @brief Parses a return statement. @return Return AST, or NULL on error. */
static astNode *parseReturn(ExpressionStops stops) {
    if (!advance()) {
        return NULL;
    }
    /** return without a value is allowed for Unit functions */
    if (currentToken.type == TOKEN_NEWLINE ||
        currentToken.type == TOKEN_SEMICOLON ||
        currentToken.type == TOKEN_EOF ||
        (stops.stopAtElse && currentToken.type == TOKEN_KW_ELSE)) {
        return createReturnNode(NULL);
    }
    astNode *value = parseExpressionWithStops(stops);
    if (value == NULL) {
        return NULL;
    }
    return createReturnNode(value);
}

/**
 * @brief Parses an indented block or a single-line body.
 * @param indent Indent of the enclosing header.
 * @param isExpression True when the body must be a value expression.
 * @param stops Additional expression terminators for inline bodies.
 * @return Body as AST_BLOCK, or NULL on error.
 */
static astNode *parseBody(size_t indent, bool isExpression,
                          ExpressionStops stops) {
    if (currentToken.type == TOKEN_NEWLINE) {
        if (!advance())
            return NULL;
        return parseBlock(indent);
    }

    /** single-line body is represented by a block of one statement */
    astNode *statement = isExpression ? parseExpressionWithStops(stops)
                                      : parseStatement(indent, stops);
    if (statement == NULL)
        return NULL;

    astNode *body = createBlockNode();
    if (addStatementToBlock(body, statement) != 0) {
        freeAst(statement);
        freeAst(body);
        return NULL;
    }
    return body;
}

/**
 * @brief Parses an if statement or an if-expression.
 * @param indent Indent of the containing logical line.
 * @param isExpression True when the if must have an else branch.
 * @param stops Terminators inherited from an outer expression.
 * @return If AST, or NULL on error.
 */
static astNode *parseIf(size_t indent, bool isExpression,
                        ExpressionStops stops) {
    if (!match(TOKEN_KW_IF, "expected 'if'"))
        return NULL;

    astNode *condition = parseExpression();
    if (condition == NULL)
        return NULL;

    if (!match(TOKEN_KW_THEN, "expected 'then'")) {
        freeAst(condition);
        return NULL;
    }

    /** the first branch ends at its matching else */
    ExpressionStops thenStops = stops;
    thenStops.stopAtElse = true;
    astNode *thenBlock = parseBody(indent, isExpression, thenStops);
    if (thenBlock == NULL) {
        freeAst(condition);
        return NULL;
    }

    /** after an inline body, else may start the next logical line */
    if (currentToken.type == TOKEN_NEWLINE && !advance()) {
        freeAst(condition);
        freeAst(thenBlock);
        return NULL;
    }

    astNode *elseBlock = NULL;
    if (currentToken.type == TOKEN_KW_ELSE &&
        (!currentToken.startsLogicalLine || currentToken.indent == indent)) {
        if (!advance()) {
            freeAst(condition);
            freeAst(thenBlock);
            return NULL;
        }

        if (currentToken.type == TOKEN_KW_IF) {
            /** else if is stored as an if inside the else block */
            astNode *nested = parseIf(indent, isExpression, stops);
            if (nested != NULL) {
                elseBlock = createBlockNode();
                addStatementToBlock(elseBlock, nested);
            }
        } else {
            elseBlock = parseBody(indent, isExpression, stops);
        }
        if (elseBlock == NULL) {
            freeAst(condition);
            freeAst(thenBlock);
            return NULL;
        }
    } else if (isExpression) {
        syntaxError("if-expression requires 'else'");
        freeAst(condition);
        freeAst(thenBlock);
        return NULL;
    }

    astNode *node = createIfNode(condition, thenBlock, elseBlock);
    if (node == NULL) {
        freeAst(condition);
        freeAst(thenBlock);
        freeAst(elseBlock);
    }
    return node;
}

/**
 * @brief Parses a while loop with an indented or single-line body.
 * @param indent Indent of the while line.
 * @param stops Terminators inherited from an enclosing inline body.
 * @return While AST, or NULL on error.
 */
static astNode *parseWhile(size_t indent, ExpressionStops stops) {
    if (!match(TOKEN_KW_WHILE, "expected 'while'"))
        return NULL;

    astNode *condition = parseExpression();
    if (condition == NULL)
        return NULL;

    if (!match(TOKEN_KW_DO, "expected 'do'")) {
        freeAst(condition);
        return NULL;
    }

    astNode *body = parseBody(indent, false, stops);
    if (body == NULL) {
        freeAst(condition);
        return NULL;
    }

    astNode *node = createWhileNode(condition, body);
    if (node == NULL) {
        freeAst(condition);
        freeAst(body);
    }
    return node;
}

/**
 * @brief Parses a range-based for loop with optional guards.
 * @param indent Indent of the for line.
 * @param stops Terminators inherited from an enclosing inline body.
 * @return For AST, or NULL on error.
 */
static astNode *parseFor(size_t indent, ExpressionStops stops) {
    if (!match(TOKEN_KW_FOR, "expected 'for'"))
        return NULL;

    /** generator and guards may be on their own logical lines */
    bool multiline = currentToken.type == TOKEN_NEWLINE;
    size_t generatorIndent = indent;
    if (multiline) {
        if (!advance())
            return NULL;
        if (!currentToken.startsLogicalLine || currentToken.indent <= indent) {
            syntaxError("expected indented for generator");
            return NULL;
        }
        generatorIndent = currentToken.indent;
    }

    if (currentToken.type != TOKEN_IDENTIFIER) {
        syntaxError("expected for iterator");
        return NULL;
    }
    char *name = copyName();
    if (!advance() ||
        !match(TOKEN_LEFT_ARROW, "expected '<-' after iterator")) {
        free(name);
        return NULL;
    }

    ExpressionStops startStops = {.stopAtRange = true};
    astNode *start = parseExpressionWithStops(startStops);
    if (start == NULL) {
        free(name);
        return NULL;
    }

    /** to includes the end, until excludes it */
    bool inclusive = isIdentifierWord("to");
    if (!inclusive && !isIdentifierWord("until")) {
        syntaxError("expected 'to' or 'until' in for range");
        free(name);
        freeAst(start);
        return NULL;
    }
    if (!advance()) {
        free(name);
        freeAst(start);
        return NULL;
    }

    ExpressionStops endStops = {.stopAtBy = true, .stopAtGuard = true};
    astNode *end = parseExpressionWithStops(endStops);
    if (end == NULL) {
        free(name);
        freeAst(start);
        return NULL;
    }

    astNode *step = NULL;
    if (isIdentifierWord("by")) {
        if (!inclusive) {
            syntaxError("'by' requires an inclusive 'to' range");
            free(name);
            freeAst(start);
            freeAst(end);
            return NULL;
        }
        if (advance()) {
            ExpressionStops stepStops = {.stopAtGuard = true};
            step = parseExpressionWithStops(stepStops);
        }
        if (step == NULL) {
            free(name);
            freeAst(start);
            freeAst(end);
            return NULL;
        }
    }

    /** create the node before guards so it owns every parsed value */
    astNode *emptyBody = createBlockNode();
    astNode *forNode =
        createForNode(name, start, end, inclusive, step, emptyBody);
    if (forNode == NULL) {
        freeAst(emptyBody);
        free(name);
        freeAst(start);
        freeAst(end);
        freeAst(step);
        return NULL;
    }

    if (multiline) {
        if (currentToken.type != TOKEN_NEWLINE || !advance()) {
            syntaxError("expected newline after for generator");
            freeAst(forNode);
            return NULL;
        }
    }

    while (currentToken.type == TOKEN_KW_IF &&
           (!multiline || (currentToken.startsLogicalLine &&
                           currentToken.indent == generatorIndent))) {
        if (!advance()) {
            freeAst(forNode);
            return NULL;
        }
        ExpressionStops guardStops = {.stopAtGuard = true};
        astNode *guard = parseExpressionWithStops(guardStops);
        if (guard == NULL) {
            freeAst(forNode);
            return NULL;
        }
        addGuardToFor(forNode, guard);

        if (multiline) {
            if (currentToken.type != TOKEN_NEWLINE || !advance()) {
                syntaxError("expected newline after for guard");
                freeAst(forNode);
                return NULL;
            }
        }
    }

    if (currentToken.type != TOKEN_KW_DO ||
        (multiline &&
         (!currentToken.startsLogicalLine || currentToken.indent != indent))) {
        syntaxError("expected 'do' aligned with 'for'");
        freeAst(forNode);
        return NULL;
    }
    if (!advance()) {
        freeAst(forNode);
        return NULL;
    }

    astNode *body = parseBody(indent, false, stops);
    if (body == NULL) {
        freeAst(forNode);
        return NULL;
    }
    freeAst(forNode->data.forStmt.body);
    forNode->data.forStmt.body = body;
    body->data.block.parentIfForBody = forNode;
    return forNode;
}

/**
 * @brief Parses one statement.
 * @param indent Indent of its line.
 * @param stops Expression terminators for an inline statement.
 * @return Statement AST, or NULL on error.
 */
static astNode *parseStatement(size_t indent, ExpressionStops stops) {
    switch (currentToken.type) {
    case TOKEN_KW_VAL:
        return parseVarDef(true, stops);
    case TOKEN_KW_VAR:
        return parseVarDef(false, stops);
    case TOKEN_KW_RETURN:
        return parseReturn(stops);
    case TOKEN_KW_IF:
        return parseIf(indent, false, stops);
    case TOKEN_KW_WHILE:
        return parseWhile(indent, stops);
    case TOKEN_KW_FOR:
        return parseFor(indent, stops);
    case TOKEN_IDENTIFIER: {
        /** check if name starts an assignment or expression */
        char *name = copyName();
        if (!advance()) {
            free(name);
            return NULL;
        }
        if (currentToken.type == TOKEN_ASSIGN) {
            if (!advance()) {
                free(name);
                return NULL;
            }
            astNode *value = parseExpressionWithStops(stops);
            if (value == NULL) {
                free(name);
                return NULL;
            }
            astNode *node = createAssignNode(name, value);
            if (node == NULL) {
                free(name);
                freeAst(value);
            }
            return node;
        }
        astNode *first = parseIdentifierValue(name);
        if (first == NULL)
            return NULL;
        return parseExpressionFromValue(first, stops);
    }
    default:
        /** a plain expression can also be a statement */
        return parseExpressionWithStops(stops);
    }
}

/**
 * @brief Parses statements at the same indentation.
 * @param parentIndent Indent of the function/if header.
 * @return Block AST, or NULL on error.
 */
static astNode *parseBlock(size_t parentIndent) {
    if (!currentToken.startsLogicalLine ||
        currentToken.indent <= parentIndent) {
        syntaxError("expected indented block");
        return NULL;
    }

    size_t indent = currentToken.indent;
    astNode *block = createBlockNode();
    bool afterSemicolon = false;

    while (currentToken.type != TOKEN_EOF) {
        if (currentToken.startsLogicalLine) {
            if (currentToken.indent < indent) {
                break;
            }
            if (currentToken.indent != indent) {
                syntaxError("unexpected indentation");
                break;
            }
        } else if (!afterSemicolon) {
            syntaxError("expected new statement line");
            break;
        }

        /** a logical line cannot begin with an operator */
        if (currentToken.startsLogicalLine &&
            isOperatorToken(currentToken.type)) {
            syntaxError("logical line cannot start with operator");
            break;
        }

        astNode *statement = parseStatement(indent, (ExpressionStops){0});
        if (statement == NULL) {
            break;
        }
        addStatementToBlock(block, statement);

        /** a nested block may have consumed the separating newline */
        if (currentToken.type != TOKEN_NEWLINE &&
            currentToken.type != TOKEN_SEMICOLON &&
            currentToken.startsLogicalLine) {
            afterSemicolon = false;
            continue;
        }

        if (currentToken.type == TOKEN_NEWLINE) {
            if (!advance()) {
                break;
            }
            afterSemicolon = false;
        } else if (currentToken.type == TOKEN_SEMICOLON) {
            if (!advance()) {
                break;
            }
            if (currentToken.type == TOKEN_NEWLINE ||
                currentToken.type == TOKEN_EOF) {
                syntaxError("expected statement after ';'");
                break;
            }
            afterSemicolon = true;
        } else if (currentToken.type != TOKEN_EOF) {
            syntaxError("expected end of statement");
            break;
        }
    }

    if (ERRCODE == 0 && block->data.block.count == 0) {
        syntaxError("empty block");
    }
    if (ERRCODE != 0) {
        freeAst(block);
        return NULL;
    }
    return block;
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

    /** parse a block or a single expression as the function body */
    bool isBlock = currentToken.type == TOKEN_NEWLINE;
    astNode *body = NULL;
    if (isBlock) {
        if (advance()) {
            body = parseBlock(0);
        }
    } else {
        body = parseExpression();
    }
    if (body == NULL) {
        freeParameters(params, paramCount);
        free(name);
        return NULL;
    }

    if (!isBlock) {
        if (currentToken.type != TOKEN_NEWLINE &&
            currentToken.type != TOKEN_EOF &&
            !(currentToken.startsLogicalLine && currentToken.indent == 0)) {
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
    }

    /** wrap single-line expressions in a block */
    if (!isBlock) {
        astNode *block = createBlockNode();
        if (addStatementToBlock(block, body) != 0) {
            freeAst(block);
            freeAst(body);
            freeParameters(params, paramCount);
            free(name);
            return NULL;
        }
        body = block;
    }

    /** build function AST */
    astNode *function =
        createFuncDefNode(name, isMain, params, paramCount, body);
    if (function == NULL) {
        freeAst(body);
        freeParameters(params, paramCount);
        free(name);
        logErr(ERR_INTERNAL, LOGERR_NO_POS, LOGERR_NO_POS, LOGERR_NO_DETMSG);
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
    currentLineIndent = 0;
    lexer = lexerCreate(stdin);
    /** lexerCreate() already reports initialization errors */
    if (lexer == NULL)
        return NULL;
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
