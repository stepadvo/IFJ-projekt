/**
 * @file lexer.h
 * @author IVO ZAHRADNÍČEK
 * @brief Interface of the IFJ26 lexical analyzer.
 *
 * The lexer reads a source program from the supplied input stream and
 * provides its tokens to the parser one at a time.
 */

#ifndef IFJ_LEXER_H
#define IFJ_LEXER_H

#include <stdio.h>

#include "token.h"

/**
 * @struct Lexer
 * @brief Opaque lexical analyzer context.
 *
 * The structure definition is private to the lexer implementation. Callers
 * use it only through a pointer and the public functions in this interface.
 */
typedef struct Lexer Lexer;

/**
 * @enum LexerStatus
 * @brief Result of a lexical analyzer operation.
 */
typedef enum {
    LEXER_OK = 0,
    LEXER_ERROR = 1,
    LEXER_INTERNAL_ERROR = 99
} LexerStatus;

/**
 * @brief Creates a lexer for the supplied input stream.
 *
 * The input stream must remain open while the lexer is in use. Destroying
 * the lexer does not close the stream.
 *
 * @param input Open readable stream, typically stdin.
 * @return Pointer to the new lexer context, or NULL if allocation fails
 *         or input is NULL.
 */
Lexer *lexer_create(FILE *input);

/**
 * @brief Reads and returns the next token from the input stream.
 *
 * Before the first call, initialize the output structure, for example with
 * Token token = {0}. On subsequent calls using the same structure, its
 * previous contents are released first. The output structure owns any text
 * attribute and keeps it valid until token_free() is called or this
 * function is called again with the same pointer.
 *
 * Identifiers and string literals carry text attributes. Integer, double,
 * and Boolean literals carry converted values. Other token kinds have no
 * attribute.
 *
 * Each non-empty logical line ends with TOKEN_NEWLINE. TOKEN_NEWLINE is
 * not emitted inside parentheses; a physical line break there can be
 * reported through newline_before. Indentation is returned as the number
 * of spaces in indent on the first actual token of a logical line.
 *
 * This function does not print diagnostics or terminate the program. After
 * a lexical or internal error, the caller must clean up and stop reading.
 *
 * @param lexer Context created by lexer_create().
 * @param out_token Output token, initialized to zero or containing a token
 *                  previously returned by this function. Any previous text
 *                  attribute is released before the new token is stored.
 * @return LEXER_OK when a token is returned, including TOKEN_EOF;
 *         LEXER_ERROR for a lexical error; or LEXER_INTERNAL_ERROR for an
 *         internal error or input read error. On error, out_token is set
 *         to TOKEN_INVALID.
 */
LexerStatus get_token(Lexer *lexer, Token *out_token);

/**
 * @brief Releases the lexer context.
 *
 * This function does not release tokens already returned by the lexer and
 * does not close the input stream. Passing NULL has no effect.
 *
 * @param lexer Lexer context; NULL is allowed.
 */
void lexer_destroy(Lexer *lexer);

#endif

