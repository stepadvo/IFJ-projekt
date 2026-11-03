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
static Token current_token = {0};

/** Read the next token. get_token() also frees the previous one. */
static bool advance(void) {
    LexerStatus status = get_token(lexer, &current_token);

    if (status == LEXER_OK) {
        return true;
    }

    if (status == LEXER_INTERNAL_ERROR) {
        ERRCODE = 99;
    } else if (ERRCODE == 0) {
        ERRCODE = 1;
    }
    return false;
}

/** Report a syntax error and remember its error code. */
static void syntaxError(const char *message) {
    fprintf(stderr, "Syntax error at line %zu: %s\n", current_token.line,
            message);
    if (ERRCODE == 0) {
        ERRCODE = 2;
    }
}

/** Check the current token and move to the next one. */
static bool expect(TokenType expected) {
    if (current_token.type != expected) {
        syntaxError("unexpected token");
        return false;
    }
    return advance();
}

/** Parse the mandatory first line: import ifj26.* */
static bool parseProlog(void) {
    if (!expect(TOKEN_KW_IMPORT)) {
        return false;
    }

    if (current_token.type != TOKEN_IDENTIFIER ||
        strcmp(current_token.attribute.text.data, "ifj26") != 0) {
        syntaxError("expected 'ifj26' after import");
        return false;
    }

    return advance() && expect(TOKEN_DOT) && expect(TOKEN_MULTIPLY) &&
           expect(TOKEN_NEWLINE);
}

/** Start parsing and release the lexer after use. */
astNode *parse(void) {

    current_token = (Token){0};
    lexer = lexer_create(stdin);
    if (lexer == NULL) {
        fprintf(stderr, "Internal error: could not create lexer\n");
        ERRCODE = 99;
        return NULL;
    }

    astNode *root = NULL;

    if (advance() && parseProlog()) {
        if (current_token.type == TOKEN_EOF) {
            root = createProgramNode();
            // TODO: Check for @main during semantic analysis.
        } else {
            // TODO: Parse top-level function definitions and @main.
            syntaxError("function definitions are not implemented yet");
        }
    }

    token_free(&current_token);
    lexer_destroy(lexer);
    lexer = NULL;
    return root;
}
