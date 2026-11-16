/**
 * @file parser.c
 * @brief Initial skeleton of the IFJ26 parser.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 *
 * Parses the prolog and basic single-line function definitions.
 * Parameters, blocks and complete expressions will be added later.
 */

#include "parser.h"
#include "compiler.h"
#include "lexer.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Lexer *lexer = NULL;
static Token currentToken = {0};

/** Read the next token. getToken() also frees the previous one. */
static bool advance(void) {
    LexerStatus status = getToken(lexer, &currentToken);

    if (status == LEXER_OK)
        return true;
    if (status == LEXER_INTERNAL_ERROR)
        ERRCODE = 99;
    else if (ERRCODE == 0)
        ERRCODE = 1;
    return false;
}

/** Report a syntax error and remember its error code. */
static void syntaxError(const char *message) {
    fprintf(stderr, "Syntax error at line %zu: %s\n", currentToken.line,
            message);
    if (ERRCODE == 0) {
        ERRCODE = 2;
    }

    return;
}

/** Check the current token and move to the next one. */
static bool match(TokenType expected, const char *errMessage) {
    if (currentToken.type != expected) {
        syntaxError(errMessage);
        return false;
    }

    return advance();
}

/** Parse the mandatory first line: import ifj26.* */
static bool parseProlog(void) {
    const TokenType prolog[] = {TOKEN_KW_IMPORT, TOKEN_IDENTIFIER, TOKEN_DOT,
                                TOKEN_MULTIPLY, TOKEN_NEWLINE};

    const char *errMessages[] = {
        "expected 'import'", "expected 'ifj26' after 'import'",
        "expected '.' after 'ifj26'", "expected '*' after '.'",
        "expected '\\n' after '*'"};

    for (size_t i = 0; i < 5; i++) {
        if (i == 1 && currentToken.type == TOKEN_IDENTIFIER) {
            if (strcmp(currentToken.attribute.text.data, "ifj26")) {
                syntaxError(errMessages[i]);
                return false;
            }
        }

        if (!match(prolog[i], errMessages[i]))
            return false;
    }
    return true;
}

/** Parse an optional @main annotation before a function definition. */
static bool parseMainAnnotation(bool *isMain) {
    *isMain = false;

    if (currentToken.type != TOKEN_AT) {
        return true;
    }

    size_t atLine = currentToken.line;
    size_t atColumn = currentToken.column;

    if (!advance()) {
        return false;
    }

    // The annotation must be written as @main, without a space after @.
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

    // @main may appear on its own line or before def on the same line.
    if (currentToken.type == TOKEN_NEWLINE) {
        return advance();
    }

    return true;
}

/** Parse a function with no parameters and a simple one-line body. */
static astNode *parseFunctionDef(bool isMain) {
    if (!match(TOKEN_KW_DEF, "expected 'def'")) {
        return NULL;
    }

    if (currentToken.type != TOKEN_IDENTIFIER) {
        syntaxError("expected function name");
        return NULL;
    }

    size_t nameLength = currentToken.attribute.text.length;
    char *name = malloc(nameLength + 1);
    checkMalloc(name);
    memcpy(name, currentToken.attribute.text.data, nameLength + 1);

    if (!advance() || !match(TOKEN_LPAREN, "expected '('") ||
        !match(TOKEN_RPAREN, "expected ')' (parameters not supported yet)")) {
        free(name);
        return NULL;
    }

    varType returnType = UNKNOWN;
    if (currentToken.type == TOKEN_COLON) {
        if (!advance()) {
            free(name);
            return NULL;
        }

        switch (currentToken.type) {
        case TOKEN_KW_INT:
            returnType = INT;
            break;
        case TOKEN_KW_DOUBLE:
            returnType = DOUBLE;
            break;
        case TOKEN_KW_STRING:
            returnType = STRING;
            break;
        case TOKEN_KW_BOOLEAN:
            returnType = BOOL;
            break;
        case TOKEN_KW_UNIT:
            returnType = UNIT;
            break;
        default:
            syntaxError("expected return type");
            free(name);
            return NULL;
        }

        if (!advance()) {
            free(name);
            return NULL;
        }
    }

    if (!match(TOKEN_ASSIGN, "expected '=' after function header")) {
        free(name);
        return NULL;
    }

    astNode *body = NULL;
    if (currentToken.type == TOKEN_INT_LITERAL ||
        currentToken.type == TOKEN_DOUBLE_LITERAL ||
        currentToken.type == TOKEN_STRING_LITERAL ||
        currentToken.type == TOKEN_BOOL_LITERAL) {
        body = createLiteralNode(&currentToken);
        if (!advance()) {
            freeAst(body);
            free(name);
            return NULL;
        }
    } else if (currentToken.type == TOKEN_LPAREN) {
        if (!advance() || !match(TOKEN_RPAREN, "expected ')' for Unit value")) {
            free(name);
            return NULL;
        }
        body = createUnitNode();
    } else {
        syntaxError("expected simple expression (blocks not supported yet)");
        free(name);
        return NULL;
    }

    if (currentToken.type != TOKEN_NEWLINE && currentToken.type != TOKEN_EOF) {
        syntaxError("expected end of function body");
        freeAst(body);
        free(name);
        return NULL;
    }

    if (currentToken.type == TOKEN_NEWLINE && !advance()) {
        freeAst(body);
        free(name);
        return NULL;
    }

    astNode *function = createFuncDefNode(name, isMain, NULL, 0, body);
    if (function == NULL) {
        freeAst(body);
        free(name);
        ERRCODE = 99;
        return NULL;
    }

    function->data.funcDef.explicitReturnType = returnType;
    return function;
}

/** Start parsing and release the lexer after use. */
astNode *parse(void) {
    currentToken = (Token){0};
    lexer = lexerCreate(stdin);
    if (lexer == NULL) {
        fprintf(stderr, "Internal error: could not create lexer\n");
        ERRCODE = 99;
        return NULL;
    }
    astNode *root = NULL;

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

    if (ERRCODE != 0) {
        freeAst(root);
        root = NULL;
    }

    tokenFree(&currentToken);
    lexerDestroy(lexer);
    lexer = NULL;
    return root;
}
