/**
 * @file token.h
 * @brief Token types shared by the IFJ26 lexer and parser.
 * @author Ivo Zahradníček, xzahrai00@stud.fit.vutbr.cz
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
    TOKEN_INVALID = 0, /**< Invalid or empty token. */
    TOKEN_EOF,         /**< End of input. */
    TOKEN_NEWLINE,     /**< End of a non-empty logical line. */

    TOKEN_IDENTIFIER,     /**< Identifier name. */
    TOKEN_INT_LITERAL,    /**< Integer literal. */
    TOKEN_DOUBLE_LITERAL, /**< Floating-point literal. */
    TOKEN_STRING_LITERAL, /**< String literal. */
    TOKEN_BOOL_LITERAL,   /**< Boolean literal: true or false. */

    TOKEN_KW_DEF,    /**< Keyword "def": function definition. */
    TOKEN_KW_DO,     /**< Keyword "do": loop body introduction. */
    TOKEN_KW_DOUBLE, /**< Keyword "Double": floating-point type. */
    TOKEN_KW_ELSE,   /**< Keyword "else": alternative branch. */
    TOKEN_KW_IF,     /**< Keyword "if": conditional statement or expression. */
    TOKEN_KW_IMPORT, /**< Keyword "import": import declaration. */
    TOKEN_KW_INT,    /**< Keyword "Int": integer type. */
    TOKEN_KW_RETURN, /**< Keyword "return": return from a function. */
    TOKEN_KW_STRING, /**< Keyword "String": string type. */
    TOKEN_KW_THEN,   /**< Keyword "then": conditional branch introduction. */
    TOKEN_KW_UNIT,   /**< Keyword "Unit": type with the single value (). */
    TOKEN_KW_VAL,    /**< Keyword "val": immutable variable declaration. */
    TOKEN_KW_VAR,    /**< Keyword "var": mutable variable declaration. */
    TOKEN_KW_WHILE,  /**< Keyword "while": conditional loop. */

    TOKEN_KW_BOOLEAN, /**< Keyword "Boolean": Boolean type (BOOLTHEN). */
    TOKEN_KW_FOR,     /**< Keyword "for": iteration construct (CONTROL). */

    TOKEN_KW_CASE,      /**< Reserved keyword "case". */
    TOKEN_KW_CLASS,     /**< Reserved keyword "class". */
    TOKEN_KW_EXTENSION, /**< Reserved keyword "extension". */
    TOKEN_KW_LIST,      /**< Reserved keyword "List". */
    TOKEN_KW_MATCH,     /**< Reserved keyword "match". */
    TOKEN_KW_TYPE,      /**< Reserved keyword "type". */

    TOKEN_ASSIGN,        /**< Assignment or definition: "=". */
    TOKEN_PLUS,          /**< Plus: "+". */
    TOKEN_MINUS,         /**< Minus: "-". */
    TOKEN_MULTIPLY,      /**< Multiplication: "*". */
    TOKEN_DIVIDE,        /**< Division: "/". */
    TOKEN_EQUAL,         /**< Equality: "==". */
    TOKEN_NOT_EQUAL,     /**< Inequality: "!=". */
    TOKEN_LESS,          /**< Less than: "<". */
    TOKEN_LESS_EQUAL,    /**< Less than or equal to: "<=". */
    TOKEN_GREATER,       /**< Greater than: ">". */
    TOKEN_GREATER_EQUAL, /**< Greater than or equal to: ">=". */
    TOKEN_NOT,           /**< Logical negation: "!". */
    TOKEN_AND,           /**< Logical conjunction: "&&". */
    TOKEN_OR,            /**< Logical disjunction: "||". */
    TOKEN_LEFT_ARROW,    /**< Generator arrow: "<-". */

    TOKEN_LPAREN,    /**< Opening parenthesis: "(". */
    TOKEN_RPAREN,    /**< Closing parenthesis: ")". */
    TOKEN_COLON,     /**< Colon: ":". */
    TOKEN_COMMA,     /**< Comma: ",". */
    TOKEN_SEMICOLON, /**< Semicolon: ";". */
    TOKEN_DOT,       /**< Dot: ".". */
    TOKEN_AT         /**< Annotation marker: "@". */
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
    char *data;    /**< Pointer to the null-terminated text. */
    size_t length; /**< Text length, excluding the terminating '\0'. */
} TokenText;

/**
 * @union TokenAttribute
 * @brief Token attribute whose valid member depends on Token.type.
 *
 * Tokens without a value attribute do not use any member of this union.
 */
typedef union {
    int64_t int_value;   /**< TOKEN_INT_LITERAL value; the minus sign is a
                            separate token. */
    double double_value; /**< TOKEN_DOUBLE_LITERAL value. */
    bool bool_value;     /**< TOKEN_BOOL_LITERAL value. */
    TokenText text; /**< Text for TOKEN_IDENTIFIER or TOKEN_STRING_LITERAL. */
} TokenAttribute;

/**
 * @struct Token
 * @brief One token returned by the lexer to the parser.
 *
 * The structure stores the token kind, optional attribute, source position,
 * and information needed to process logical lines and indentation.
 */
typedef struct {
    TokenType type;           /**< Token kind. */
    TokenAttribute attribute; /**< Token value or text attribute. */

    size_t line; /**< Physical line where the token starts, numbered from 1. */
    size_t column; /**< Column where the token starts, numbered from 1. */

    bool starts_logical_line; /**< True if the token starts a new logical line.
                               */
    size_t indent; /**< Number of leading spaces on the logical line; otherwise
                      0. */

    /**
     * @brief Whether an LF occurred before this actual token.
     *
     * Counts LF characters between the previous actual token and this
     * token, including skipped comments but excluding token contents.
     * For the first actual token, counts from the beginning of input.
     * Always false for TOKEN_NEWLINE, TOKEN_EOF, and TOKEN_INVALID.
     */
    bool newline_before;
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
