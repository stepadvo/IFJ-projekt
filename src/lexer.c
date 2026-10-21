/**
 * @file lexer.c
 * @author IVO ZAHRADNÍČEK
 * @brief Implementation of the IFJ26 lexical analyzer.
 */

#include "lexer.h"
#include <stdlib.h>

/**
 * @struct Lexer
 * @brief Private state of the lexical analyzer.
 */
struct Lexer {
    FILE *input;   /**< Input stream borrowed from the caller. */
    size_t line;   /**< Line of the next character, numbered from 1. */
    size_t column; /**< Column of the next character, numbered from 1. */
};

/**
 * @brief Reads one character and updates the source position.
 *
 * @param lexer Initialized lexer context.
 * @param out_char Output character, or EOF at the end of input.
 * @return LEXER_OK on success, including EOF, or LEXER_INTERNAL_ERROR
 *         if reading fails.
 */
// static LexerStatus lexer_read_char(Lexer *lexer, int *out_char) {
//     int character = fgetc(lexer->input);
//     *out_char = character;
//
//     if (character == EOF) {
//         if (ferror(lexer->input)) { /* Distinguish a read error from normal
//         EOF. */
//             return LEXER_INTERNAL_ERROR;
//         }
//
//         return LEXER_OK;
//     }
//
//     if (character == '\n') {
//         lexer->line++;
//         lexer->column = 1;
//     } else {
//         lexer->column++;
//     }
//
//     return LEXER_OK;
// }

/**
 * @brief Reads the next character and pushes it back without changing
 *        the tracked source position.
 *
 * @param lexer Initialized lexer context.
 * @param out_char Output character, or EOF at the end of input.
 * @return LEXER_OK on success, or LEXER_INTERNAL_ERROR if reading
 *         or pushing the character back fails.
 */
// static LexerStatus lexer_peek_char(Lexer *lexer, int *out_char) {
//     *out_char = EOF;
//
//     int character = fgetc(lexer->input);
//
//     if (character == EOF) {
//         if (ferror(lexer->input)) {
//             return LEXER_INTERNAL_ERROR;
//         }
//
//         return LEXER_OK;
//     }
//
//     if (ungetc(character, lexer->input) == EOF) {
//         return LEXER_INTERNAL_ERROR;
//     }
//
//     *out_char = character;
//     return LEXER_OK;
// }

/**
 * @brief Allocates and initializes a lexer context.
 *
 * @param input Open readable input stream.
 * @return Initialized context, or NULL if input is NULL or allocation fails.
 */
Lexer *lexer_create(FILE *input) {
    if (input == NULL) {
        return NULL;
    }

    Lexer *lexer = malloc(sizeof *lexer);
    if (lexer == NULL) {
        return NULL;
    }

    lexer->input = input;
    lexer->line = 1;
    lexer->column = 1;

    return lexer;
}

/**
 * @brief Releases the lexer context without closing its input stream.
 *
 * @param lexer Context to release; NULL is allowed.
 */
void lexer_destroy(Lexer *lexer) { free(lexer); }