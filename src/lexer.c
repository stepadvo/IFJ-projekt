/**
 * @file lexer.c
 * @brief Implementation of the IFJ26 lexical analyzer.
 * @author Ivo Zahradníček, xzahrai00@stud.fit.vutbr.cz
 *
 * Implements lexer context management, character reading, source position
 * tracking, and token recognition.
 */

#include "lexer.h"
#include <stdlib.h>

/**
 * @struct Lexer
 * @brief Private state of the lexical analyzer.
 */
struct Lexer {
    FILE *input;         /**< Input stream borrowed from the caller. */
    size_t line;         /**< Line of the next character, numbered from 1. */
    size_t column;       /**< Column of the next character, numbered from 1. */
    bool line_has_token; /**< Whether the current logical line contains a
                            token.*/
};

/**
 * @brief Reads one character and updates the source position.
 *
 * @param lexer Initialized lexer context.
 * @param out_char Output character, or EOF at the end of input.
 * @return LEXER_OK on success, including EOF, or LEXER_INTERNAL_ERROR
 *         if reading fails.
 */
static LexerStatus lexer_read_char(Lexer *lexer, int *out_char) {
    int character = fgetc(lexer->input);
    *out_char = character;

    if (character == EOF) {
        /* Distinguish a read error from normal EOF. */
        if (ferror(lexer->input)) {
            return LEXER_INTERNAL_ERROR;
        }

        return LEXER_OK;
    }

    if (character == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }

    return LEXER_OK;
}

/**
 * @brief Reads the next character and pushes it back without changing
 *        the tracked source position.
 *
 * @param lexer Initialized lexer context.
 * @param out_char Output character, or EOF at the end of input.
 * @return LEXER_OK on success, or LEXER_INTERNAL_ERROR if reading
 *         or pushing the character back fails.
 */
static LexerStatus lexer_peek_char(Lexer *lexer, int *out_char) {
    *out_char = EOF;

    int character = fgetc(lexer->input);

    if (character == EOF) {
        /* Distinguish a read error from normal EOF. */
        if (ferror(lexer->input)) {
            return LEXER_INTERNAL_ERROR;
        }

        return LEXER_OK;
    }

    if (ungetc(character, lexer->input) == EOF) {
        return LEXER_INTERNAL_ERROR;
    }

    *out_char = character;
    return LEXER_OK;
}

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
    lexer->line_has_token = false;

    return lexer;
}

/**
 * @brief Reads the next token from the input stream.
 *
 * @param lexer Initialized lexer context.
 * @param out_token Initialized output token.
 * @return LEXER_OK when a token is returned;
 *         LEXER_ERROR on a lexical error;
 *         LEXER_INTERNAL_ERROR on an internal failure or input
 *         not supported by the current implementation.
 */
LexerStatus get_token(Lexer *lexer, Token *out_token) {
    token_free(out_token);

    out_token->line = lexer->line;
    out_token->column = lexer->column;

    int character;
    LexerStatus status = lexer_peek_char(lexer, &character);

    if (status != LEXER_OK) {
        return status;
    }

    if (character == EOF) {
        if (lexer->line_has_token) {
            out_token->type = TOKEN_NEWLINE;
            lexer->line_has_token = false;
            return LEXER_OK;
        }

        out_token->type = TOKEN_EOF;
        return LEXER_OK;
    }

    status = lexer_read_char(lexer, &character);

    if (status != LEXER_OK) {
        return status;
    }

    TokenType type;

    /*
     * GPT-6.1 Sol assisted with this code.
     * Prompt: Implementuj pouze switch pro rozpoznávání operátorů v
     * get_token(). První znak je již spotřebovaný a uložený v character.
     * Rozpoznávej
     * +, -, *, /, =, ==, !, !=, <, <=, <-, >, >=, && a || a nastav
     * odpovídající hodnotu TokenType do proměnné type.
     * Pro kontrolu druhého znaku si vytvoř pomocnou proměnnou a použij
     * funkci lexer_peek_char(Lexer *lexer, int *out_char) a spotřebuj jej
     * pomocí funkce lexer_read_char(Lexer *lexer, int *out_char) pouze tehdy,
     * pokud tvoří daný operátor. Jiný následující znak ponech ve vstupu. Chyby
     * pomocných funkcí předávej volajícímu. Samostatné & nebo | odmítni pomocí
     * LEXER_ERROR. Pro dosud nepodporované vstupní znaky dočasně vrať
     * LEXER_INTERNAL_ERROR. Rozpoznávání komentářů u / ponech jako TODO.
     * Neměň veřejné rozhraní ani ostatní části funkce.
     */

    switch (character) {
    case '+':
        type = TOKEN_PLUS;
        break;

    case '-':
        type = TOKEN_MINUS;
        break;

    case '*':
        type = TOKEN_MULTIPLY;
        break;

    case '/':
        /* TODO: Distinguish division from comment starts. */
        type = TOKEN_DIVIDE;
        break;

    case '=': {
        type = TOKEN_ASSIGN;

        int next_character;
        status = lexer_peek_char(lexer, &next_character);

        if (status != LEXER_OK) {
            return status;
        }

        if (next_character == '=') {
            status = lexer_read_char(lexer, &next_character);

            if (status != LEXER_OK) {
                return status;
            }

            type = TOKEN_EQUAL;
        }

        break;
    }

    case '!': {
        type = TOKEN_NOT;

        int next_character;
        status = lexer_peek_char(lexer, &next_character);

        if (status != LEXER_OK) {
            return status;
        }

        if (next_character == '=') {
            status = lexer_read_char(lexer, &next_character);

            if (status != LEXER_OK) {
                return status;
            }

            type = TOKEN_NOT_EQUAL;
        }

        break;
    }

    case '<': {
        type = TOKEN_LESS;

        int next_character;
        status = lexer_peek_char(lexer, &next_character);

        if (status != LEXER_OK) {
            return status;
        }

        if (next_character == '=' || next_character == '-') {
            status = lexer_read_char(lexer, &next_character);

            if (status != LEXER_OK) {
                return status;
            }

            if (next_character == '=') {
                type = TOKEN_LESS_EQUAL;
            } else {
                type = TOKEN_LEFT_ARROW;
            }
        }

        break;
    }

    case '>': {
        type = TOKEN_GREATER;

        int next_character;
        status = lexer_peek_char(lexer, &next_character);

        if (status != LEXER_OK) {
            return status;
        }

        if (next_character == '=') {
            status = lexer_read_char(lexer, &next_character);

            if (status != LEXER_OK) {
                return status;
            }

            type = TOKEN_GREATER_EQUAL;
        }

        break;
    }

    case '&':
    case '|': {
        int next_character;
        status = lexer_peek_char(lexer, &next_character);

        if (status != LEXER_OK) {
            return status;
        }

        if (next_character != character) {
            return LEXER_ERROR;
        }

        type = (character == '&') ? TOKEN_AND : TOKEN_OR;

        status = lexer_read_char(lexer, &next_character);

        if (status != LEXER_OK) {
            return status;
        }

        break;
    }

    default:
        /* TODO: Recognize the remaining tokens. */
        return LEXER_INTERNAL_ERROR;
    }

    out_token->type = type;
    out_token->starts_logical_line = !lexer->line_has_token;
    lexer->line_has_token = true;

    return LEXER_OK;
}

/**
 * @brief Releases the lexer context without closing its input stream.
 *
 * @param lexer Context to release; NULL is allowed.
 */
void lexer_destroy(Lexer *lexer) { free(lexer); }