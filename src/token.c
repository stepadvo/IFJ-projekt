/**
 * @file token.c
 * @brief Token cleanup for the IFJ26 lexical analyzer.
 * @author Ivo Zahradníček, xzahrai00@stud.fit.vutbr.cz
 *
 * Implements the release of owned token text and resets tokens
 * to an empty state.
 */

#include "token.h"
#include <stdlib.h>

/**
 * @brief Releases owned text and resets the token to an empty state.
 *
 * @param token Initialized token to reset; NULL is allowed.
 */
void token_free(Token *token) {
    if (token == NULL) {
        return;
    }

    if (token->type == TOKEN_IDENTIFIER ||
        token->type == TOKEN_STRING_LITERAL) {
        free(token->attribute.text.data);
    }

    *token = (Token){0};
}