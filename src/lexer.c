/**
 * @file lexer.c
 * @brief Implementation of the IFJ26 lexical analyzer.
 * @author Ivo Zahradníček, xzahrai00@stud.fit.vutbr.cz
 *
 * Implements lexer context management, character reading, source position
 * tracking, and token recognition.
 */
#include "lexer.h"
#include "compiler.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
    size_t paren_depth;  /**< Number of currently open parentheses. */
    bool newline_since_token; /**< Whether an LF occurred since the last actual
                                 token. */
    size_t physical_indent; /**< Leading spaces on the current physical line. */
    bool at_physical_line_start; /**< Whether only leading spaces have been
                                    read. */
    size_t leading_tab_column; /**< First leading tab column; zero if absent. */
    size_t logical_indent; /**< Indentation of the logical line's first physical
                              line. */
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
            setErrCode(LEXER_INTERNAL_ERROR);

            fprintf(stderr,
                    "Internal lexer error at %zu:%zu: input read failed.\n",
                    lexer->line, lexer->column);

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
            setErrCode(LEXER_INTERNAL_ERROR);

            fprintf(stderr,
                    "Internal lexer error at %zu:%zu: input read failed.\n",
                    lexer->line, lexer->column);

            return LEXER_INTERNAL_ERROR;
        }

        return LEXER_OK;
    }

    if (ungetc(character, lexer->input) == EOF) {
        setErrCode(LEXER_INTERNAL_ERROR);

        fprintf(stderr,
                "Internal lexer error at %zu:%zu: character pushback failed.\n",
                lexer->line, lexer->column);

        return LEXER_INTERNAL_ERROR;
    }

    *out_char = character;
    return LEXER_OK;
}

/**
 * @brief Classifies a complete identifier-like word.
 *
 * @param text Null-terminated word.
 * @return Keyword type, TOKEN_BOOL_LITERAL, or TOKEN_IDENTIFIER.
 */
static TokenType lexer_identifier_type(const char *text) {
    static const struct {
        const char *text;
        TokenType type;
    } keywords[] = {{"def", TOKEN_KW_DEF},
                    {"do", TOKEN_KW_DO},
                    {"Double", TOKEN_KW_DOUBLE},
                    {"else", TOKEN_KW_ELSE},
                    {"if", TOKEN_KW_IF},
                    {"import", TOKEN_KW_IMPORT},
                    {"Int", TOKEN_KW_INT},
                    {"return", TOKEN_KW_RETURN},
                    {"String", TOKEN_KW_STRING},
                    {"then", TOKEN_KW_THEN},
                    {"Unit", TOKEN_KW_UNIT},
                    {"val", TOKEN_KW_VAL},
                    {"var", TOKEN_KW_VAR},
                    {"while", TOKEN_KW_WHILE},
                    {"Boolean", TOKEN_KW_BOOLEAN},
                    {"for", TOKEN_KW_FOR},
                    {"case", TOKEN_KW_CASE},
                    {"class", TOKEN_KW_CLASS},
                    {"extension", TOKEN_KW_EXTENSION},
                    {"List", TOKEN_KW_LIST},
                    {"match", TOKEN_KW_MATCH},
                    {"type", TOKEN_KW_TYPE},
                    {"true", TOKEN_BOOL_LITERAL},
                    {"false", TOKEN_BOOL_LITERAL}};

    for (size_t i = 0; i < sizeof keywords / sizeof keywords[0]; i++) {
        if (strcmp(text, keywords[i].text) == 0) {
            return keywords[i].type;
        }
    }

    return TOKEN_IDENTIFIER;
}

/**
 * @brief Reads an identifier whose first character is already consumed.
 *
 * @param lexer Initialized lexer context.
 * @param first_character Consumed letter or underscore.
 * @param out_text Receives owned text on success; unchanged on error.
 * @return LEXER_OK, LEXER_ERROR for a standalone underscore,
 *         or LEXER_INTERNAL_ERROR on allocation or input failure.
 */
static LexerStatus lexer_read_identifier(Lexer *lexer, int first_character,
                                         TokenText *out_text) {
    size_t length = 0;
    size_t capacity = 32;
    char *buffer = malloc(capacity);

    if (buffer == NULL) {
        goto allocation_error;
    }

    int character = first_character;

    while (true) {
        /* Keep one free byte for the terminating '\0'. */
        if (length == capacity - 1) {
            if (capacity > SIZE_MAX / 2) {
                goto allocation_error;
            }

            size_t new_capacity = capacity * 2;
            char *new_buffer = realloc(buffer, new_capacity);

            if (new_buffer == NULL) {
                goto allocation_error;
            }

            buffer = new_buffer;
            capacity = new_capacity;
        }

        buffer[length++] = (char)character;

        int next_character;
        LexerStatus status = lexer_peek_char(lexer, &next_character);

        if (status != LEXER_OK) {
            free(buffer);
            return status;
        }

        bool continues_identifier =
            (next_character >= 'a' && next_character <= 'z') ||
            (next_character >= 'A' && next_character <= 'Z') ||
            (next_character >= '0' && next_character <= '9') ||
            next_character == '_';

        if (!continues_identifier) {
            break;
        }

        status = lexer_read_char(lexer, &character);

        if (status != LEXER_OK) {
            free(buffer);
            return status;
        }
    }

    buffer[length] = '\0';

    if (length == 1 && buffer[0] == '_') {
        free(buffer);

        setErrCode(LEXER_ERROR);

        fprintf(stderr,
                "Lexical error at %zu:%zu: "
                "standalone underscore is not an identifier.\n",
                lexer->line, lexer->column - 1);

        return LEXER_ERROR;
    }

    out_text->data = buffer;
    out_text->length = length;

    return LEXER_OK;

allocation_error:
    free(buffer);

    setErrCode(LEXER_INTERNAL_ERROR);

    fprintf(stderr,
            "Internal lexer error at %zu:%zu: "
            "identifier buffer allocation failed.\n",
            lexer->line, lexer->column);

    return LEXER_INTERNAL_ERROR;
}

/**
 * @brief Allocates and initializes a lexer context.
 *
 * Reports initialization failures to stderr and sets ERRCODE to 99
 * if no previous compilation error has been recorded.
 *
 * @param input Open readable input stream.
 *
 * @return Initialized context, or NULL if input is NULL or allocation fails.
 */
Lexer *lexer_create(FILE *input) {
    if (input == NULL) {
        setErrCode(LEXER_INTERNAL_ERROR);

        fprintf(stderr, "Internal lexer error: input stream is NULL.\n");
        return NULL;
    }

    Lexer *lexer = malloc(sizeof *lexer);

    if (lexer == NULL) {
        setErrCode(LEXER_INTERNAL_ERROR);

        fprintf(stderr, "Internal lexer error: lexer allocation failed.\n");
        return NULL;
    }

    lexer->input = input;
    lexer->line = 1;
    lexer->column = 1;
    lexer->line_has_token = false;
    lexer->paren_depth = 0;
    lexer->newline_since_token = false;
    lexer->physical_indent = 0;
    lexer->at_physical_line_start = true;
    lexer->leading_tab_column = 0;
    lexer->logical_indent = 0;

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

    int character;
    LexerStatus status;

    while (true) {
        out_token->line = lexer->line;
        out_token->column = lexer->column;

        status = lexer_peek_char(lexer, &character);

        if (status != LEXER_OK) {
            return status;
        }

        if (character == EOF) {
            if (lexer->line_has_token && lexer->paren_depth == 0) {
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

        if (character == ' ') {
            if (lexer->at_physical_line_start) {
                lexer->physical_indent++;
            }

            continue;
        }

        if (character == '\t') {
            if (lexer->at_physical_line_start &&
                lexer->leading_tab_column == 0) {
                lexer->leading_tab_column = out_token->column;
            }

            continue;
        }

        if (character != '\n') {
            bool starts_physical_line = lexer->at_physical_line_start;
            lexer->at_physical_line_start = false;

            if (lexer->leading_tab_column != 0) {
                size_t tab_column = lexer->leading_tab_column;
                lexer->leading_tab_column = 0;

                setErrCode(LEXER_ERROR);

                fprintf(stderr,
                        "Lexical error at %zu:%zu: "
                        "tab is not allowed in indentation.\n",
                        out_token->line, tab_column);

                return LEXER_ERROR;
            }

            if (starts_physical_line && lexer->paren_depth > 0 &&
                lexer->physical_indent <= lexer->logical_indent) {
                setErrCode(LEXER_ERROR);

                fprintf(stderr,
                        "Lexical error at %zu:%zu: "
                        "continuation line must be indented more than "
                        "the first line of the logical line.\n",
                        out_token->line, out_token->column);

                return LEXER_ERROR;
            }

            break;
        }

        lexer->physical_indent = 0;
        lexer->at_physical_line_start = true;
        lexer->leading_tab_column = 0;
        lexer->newline_since_token = true;

        if (lexer->paren_depth == 0 && lexer->line_has_token) {
            out_token->type = TOKEN_NEWLINE;
            lexer->line_has_token = false;
            return LEXER_OK;
        }
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
            setErrCode(LEXER_ERROR);

            fprintf(stderr,
                    "Lexical error at %zu:%zu: expected '%c' after '%c'.\n",
                    out_token->line, out_token->column, character, character);

            return LEXER_ERROR;
        }

        type = (character == '&') ? TOKEN_AND : TOKEN_OR;

        status = lexer_read_char(lexer, &next_character);

        if (status != LEXER_OK) {
            return status;
        }

        break;
    }

    case '(': {
        type = TOKEN_LPAREN;

        lexer->paren_depth += 1;

        break;
    }

    case ')': {
        type = TOKEN_RPAREN;

        if (lexer->paren_depth > 0) {
            lexer->paren_depth -= 1;
        }

        break;
    }

    case ':': {
        type = TOKEN_COLON;
        break;
    }

    case ',': {
        type = TOKEN_COMMA;
        break;
    }

    case ';': {
        type = TOKEN_SEMICOLON;
        break;
    }

    case '.': {
        type = TOKEN_DOT;
        /* TODO: Recognize floating-point literals starting with a dot. */
        break;
    }

    case '@': {
        type = TOKEN_AT;
        break;
    }

    default:
        if ((character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') || character == '_') {
            status = lexer_read_identifier(lexer, character,
                                           &out_token->attribute.text);

            if (status != LEXER_OK) {
                return status;
            }

            const char *text = out_token->attribute.text.data;
            type = lexer_identifier_type(text);

            if (type != TOKEN_IDENTIFIER) {
                bool boolean_value =
                    type == TOKEN_BOOL_LITERAL && strcmp(text, "true") == 0;

                free(out_token->attribute.text.data);
                out_token->attribute = (TokenAttribute){0};

                if (type == TOKEN_BOOL_LITERAL) {
                    out_token->attribute.bool_value = boolean_value;
                }
            }

            break;
        }
        /* TODO: Implement recognition of the remaining tokens. */
        setErrCode(LEXER_INTERNAL_ERROR);

        fprintf(stderr,
                "Internal lexer error at %zu:%zu: "
                "token recognition is not implemented for byte 0x%02X.\n",
                out_token->line, out_token->column, (unsigned int)character);

        return LEXER_INTERNAL_ERROR;
    }

    out_token->type = type;
    out_token->starts_logical_line = !lexer->line_has_token;

    if (out_token->starts_logical_line) {
        out_token->indent = lexer->physical_indent;
        lexer->logical_indent = lexer->physical_indent;
    }

    out_token->newline_before = lexer->newline_since_token;

    lexer->line_has_token = true;
    lexer->newline_since_token = false;

    return LEXER_OK;
}

/**
 * @brief Releases the lexer context without closing its input stream.
 *
 * @param lexer Context to release; NULL is allowed.
 */
void lexer_destroy(Lexer *lexer) { free(lexer); }