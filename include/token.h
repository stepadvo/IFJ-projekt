/**
 * @file token.h
 * @brief interface declaration for the lexical analyzer tokens
 * @author Ivo Zahradníček, xzahrai00
 * 
 * Declarations of the structures and enums for tokens.
 */

#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief enum of all possible token types in IFJ26 */
typedef enum {
    TOKEN_INVALID = 0,
    TOKEN_EOF,
    TOKEN_NEWLINE,
    
    // Virtual tokens for block management (added for the parser)
    TOKEN_INDENT,
    TOKEN_DEDENT,

    // Literals & Identifiers
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_DOUBLE_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_BOOL_LITERAL,

    // Keywords
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

    // Operators
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

    // Symbols
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_DOT,
    TOKEN_AT
} TokenType;

/** @brief structure for holding string token data */
typedef struct {
    char *data;     
    size_t length;  
} TokenText;

/** @brief union for token attributes (values) */
typedef union {
    int64_t int_value;   
    double double_value; 
    bool bool_value;     
    TokenText text;      
} TokenAttribute;

/** @brief the main token structure */
typedef struct {
    TokenType type;
    TokenAttribute attribute;

    size_t line;
    size_t column;

    bool starts_logical_line;
    size_t indent; 
    bool newline_before;
} Token;

/**
 * @brief frees the allocated text inside a token and resets it
 */
void token_free(Token *token);

#endif // TOKEN_H

/*
 * end token.h
 */
