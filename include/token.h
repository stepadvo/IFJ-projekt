/**
 * @file token.h
 * @author IVO ZAHRADNÍČEK
 * @brief Token types shared by the IFJ26 lexer and parser.
 *
 * This header defines the representation of a token and the function used
 * to release its optional text attribute. The supported scope includes the
 * core language and the FUNEXP, BOOLTHEN, and CONTROL extensions.
 */

#ifndef IFJ_TOKEN_H
#define IFJ_TOKEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @enum TokenType
 * @brief Kind of lexical token recognized by the lexer.
 *
 * The token type determines which TokenAttribute member is valid.
 * TOKEN_NEWLINE marks the end of a logical line, and TOKEN_EOF marks
 * the end of the input stream.
 */
typedef enum {
    TOKEN_INVALID = 0,
    TOKEN_EOF,
    TOKEN_NEWLINE,

    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_DOUBLE_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_BOOL_LITERAL,

    TOKEN_KW_DEF,
    TOKEN_KW_DO,
    TOKEN_KW_DOUBLE,
    TOKEN_KW_ELSE,
    TOKEN_KW_IF,
    TOKEN_KW_IMPORT,
    TOKEN_KW_INT,
    TOKEN_KW_RETURN,
    TOKEN_KW_STRING,
    TOKEN_KW_THEN,
    TOKEN_KW_UNIT,
    TOKEN_KW_VAL,
    TOKEN_KW_VAR,
    TOKEN_KW_WHILE,

    TOKEN_KW_BOOLEAN,
    TOKEN_KW_FOR,

    TOKEN_KW_CASE,
    TOKEN_KW_CLASS,
    TOKEN_KW_EXTENSION,
    TOKEN_KW_LIST,
    TOKEN_KW_MATCH,
    TOKEN_KW_TYPE,

    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_NOT,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_LEFT_ARROW,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_DOT,
    TOKEN_AT
} TokenType;

/**
 * @struct TokenText
 * @brief Text attribute owned by a token.
 *
 * Identifier names and string literal contents are stored in a dynamically
 * allocated array. The array is null-terminated; the terminating '\0' is
 * not included in length.
 */
typedef struct {
    char *data;       /**< Pointer to the null-terminated text. */
    size_t length;    /**< Text length, excluding the terminating '\0'. */
} TokenText;

/**
 * @union TokenAttribute
 * @brief Token attribute whose valid member depends on Token.type.
 *
 * Tokens without a value attribute do not use any member of this union.
 */
typedef union {
    int64_t int_value;       /**< TOKEN_INT_LITERAL value; the minus sign is a separate token. */
    double double_value;    /**< TOKEN_DOUBLE_LITERAL value. */
    bool bool_value;        /**< TOKEN_BOOL_LITERAL value. */
    TokenText text;          /**< Text for TOKEN_IDENTIFIER or TOKEN_STRING_LITERAL. */
} TokenAttribute;

/**
 * @struct Token
 * @brief One token returned by the lexer to the parser.
 *
 * The structure stores the token kind, optional attribute, source position,
 * and information needed to process logical lines and indentation.
 */
typedef struct {
    TokenType type;                  /**< Token kind. */
    TokenAttribute attribute;        /**< Token value or text attribute. */

    size_t line;                     /**< Physical line where the token starts, numbered from 1. */
    size_t column;                   /**< Column where the token starts, numbered from 1. */

    bool starts_logical_line;        /**< True if the token starts a new logical line. */
    size_t indent;                   /**< Number of leading spaces on the logical line; otherwise 0. */
    bool newline_before;             /**< True if an LF occurred since the previous token. */
} Token;

/**
 * @brief Releases a token's text attribute and resets the token.
 *
 * The text attribute is released only for TOKEN_IDENTIFIER and
 * TOKEN_STRING_LITERAL. After this call, the token is set to TOKEN_INVALID
 * and all other fields are zeroed. Do not copy TokenText with a plain
 * assignment, as that would create two owners of the same pointer.
 *
 * @param token Pointer to an initialized token; NULL is allowed.
 */
void token_free(Token *token);

#endif

