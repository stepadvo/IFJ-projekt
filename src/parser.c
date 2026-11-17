/**
 * @file parser.c
 * @brief Initial skeleton of the IFJ26 parser.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 *
 * Currently parses the mandatory "import ifj26.*" prolog.
 * Function definitions, statements and expressions are TODO.
 */

#include "parser.h"
#include "compiler.h"
#include "lexer.h"
#include "token.h"
#include <stdio.h>
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
        if (currentToken.type == TOKEN_EOF) {
            root = createProgramNode();
            // TODO: Check for @main during semantic analysis.
        } else {
            // TODO: Parse top-level function definitions and @main.
            syntaxError("function definitions are not implemented yet");
        }
    }

    tokenFree(&currentToken);
    lexerDestroy(lexer);
    lexer = NULL;
    return root;
}
