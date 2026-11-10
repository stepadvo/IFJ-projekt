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
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @struct Lexer
 * @brief Private state of the lexical analyzer.
 */
struct Lexer {
    FILE *input;       /**< Input stream borrowed from the caller. */
    size_t line;       /**< Line of the next character, numbered from 1. */
    size_t column;     /**< Column of the next character, numbered from 1. */
    bool lineHasToken; /**< Whether the current logical line contains a
                            token.*/
    size_t parenDepth; /**< Number of currently open parentheses. */
    bool newlineSinceToken; /**< Whether an LF occurred since the last actual
                                 token. */
    size_t physicalIndent;  /**< Leading spaces on the current physical line. */
    bool atPhysicalLineStart; /**< Whether only leading spaces have been
                                    read. */
    size_t leadingTabColumn;  /**< First leading tab column; zero if absent. */
    size_t logicalIndent; /**< Indentation of the logical line's first physical
                              line. */
    int pendingCharacter; /**< Buffered character, or EOF when empty. */
    bool physicalLineHasToken; /**< Whether this physical line has an actual
                                  token. */
};

/**
 * @brief Reads one character and updates the source position.
 *
 * @param lexer Initialized lexer context.
 * @param outChar Output character, or EOF at the end of input.
 * @return LEXER_OK on success, including EOF, or LEXER_INTERNAL_ERROR
 *         if reading fails.
 */
static LexerStatus lexerReadChar(Lexer *lexer, int *outChar) {
    int character;

    if (lexer->pendingCharacter != EOF) {
        character = lexer->pendingCharacter;
        lexer->pendingCharacter = EOF;
    } else {
        character = fgetc(lexer->input);
    }

    *outChar = character;

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
 * @param outChar Output character, or EOF at the end of input.
 * @return LEXER_OK on success, or LEXER_INTERNAL_ERROR if reading
 *         or pushing the character back fails.
 */
static LexerStatus lexerPeekChar(Lexer *lexer, int *outChar) {
    *outChar = EOF;

    if (lexer->pendingCharacter != EOF) {
        *outChar = lexer->pendingCharacter;
        return LEXER_OK;
    }

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

    *outChar = character;
    return LEXER_OK;
}

/**
 * @brief Classifies a complete identifier-like word.
 *
 * @param text Null-terminated word.
 * @return Keyword type, TOKEN_BOOL_LITERAL, or TOKEN_IDENTIFIER.
 */
static TokenType lexerIdentifierType(const char *text) {
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
 * @param firstCharacter Consumed letter or underscore.
 * @param outText Receives owned text on success; unchanged on error.
 * @return LEXER_OK, LEXER_ERROR for a standalone underscore,
 *         or LEXER_INTERNAL_ERROR on allocation or input failure.
 */
static LexerStatus lexerReadIdentifier(Lexer *lexer, int firstCharacter,
                                       TokenText *outText) {
    size_t length = 0;
    size_t capacity = 32;
    char *buffer = malloc(capacity);

    if (buffer == NULL) {
        goto allocationError;
    }

    int character = firstCharacter;

    while (true) {
        /* Keep one free byte for the terminating '\0'. */
        if (length == capacity - 1) {
            if (capacity > SIZE_MAX / 2) {
                goto allocationError;
            }

            size_t newCapacity = capacity * 2;
            char *newBuffer = realloc(buffer, newCapacity);

            if (newBuffer == NULL) {
                goto allocationError;
            }

            buffer = newBuffer;
            capacity = newCapacity;
        }

        buffer[length++] = (char)character;

        int nextCharacter;
        LexerStatus status = lexerPeekChar(lexer, &nextCharacter);

        if (status != LEXER_OK) {
            free(buffer);
            return status;
        }

        bool continuesIdentifier =
            (nextCharacter >= 'a' && nextCharacter <= 'z') ||
            (nextCharacter >= 'A' && nextCharacter <= 'Z') ||
            (nextCharacter >= '0' && nextCharacter <= '9') ||
            nextCharacter == '_';

        if (!continuesIdentifier) {
            break;
        }

        status = lexerReadChar(lexer, &character);

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

    outText->data = buffer;
    outText->length = length;

    return LEXER_OK;

allocationError:
    free(buffer);

    setErrCode(LEXER_INTERNAL_ERROR);

    fprintf(stderr,
            "Internal lexer error at %zu:%zu: "
            "identifier buffer allocation failed.\n",
            lexer->line, lexer->column);

    return LEXER_INTERNAL_ERROR;
}

/** @brief Checks whether a character is an ASCII decimal digit. */
static bool lexerIsDigit(int character) {
    return character >= '0' && character <= '9';
}

/** @brief Appends a character and keeps the number buffer null-terminated. */
static LexerStatus lexerAppendNumberChar(Lexer *lexer, char **buffer,
                                         size_t *length, size_t *capacity,
                                         int character) {
    if (*capacity == 0 || *length == *capacity - 1) {
        if (*capacity > SIZE_MAX / 2) {
            goto allocationError;
        }

        size_t newCapacity = *capacity == 0 ? 32 : *capacity * 2;
        char *newBuffer = realloc(*buffer, newCapacity);

        if (newBuffer == NULL) {
            goto allocationError;
        }

        *buffer = newBuffer;
        *capacity = newCapacity;
    }

    (*buffer)[(*length)++] = (char)character;
    (*buffer)[*length] = '\0';
    return LEXER_OK;

allocationError:
    setErrCode(LEXER_INTERNAL_ERROR);
    fprintf(stderr,
            "Internal lexer error at %zu:%zu: "
            "number buffer allocation failed.\n",
            lexer->line, lexer->column);
    return LEXER_INTERNAL_ERROR;
}

/**
 * @brief Reads digits, omitting separators from the resulting buffer.
 * @param previousIsDigit Whether a digit was already appended for this part.
 * @return LEXER_ERROR if a required digit is missing or a separator is
 * misplaced. Lexical diagnostics are left to lexerReadNumber().
 */
static LexerStatus lexerReadNumberDigits(Lexer *lexer, char **buffer,
                                         size_t *length, size_t *capacity,
                                         bool previousIsDigit) {
    while (true) {
        int character;
        LexerStatus status = lexerPeekChar(lexer, &character);

        if (status != LEXER_OK) {
            return status;
        }

        if (lexerIsDigit(character)) {
            status = lexerReadChar(lexer, &character);
            if (status != LEXER_OK) {
                return status;
            }

            status = lexerAppendNumberChar(lexer, buffer, length, capacity,
                                           character);
            if (status != LEXER_OK) {
                return status;
            }

            previousIsDigit = true;
        } else if (character == '_' && previousIsDigit) {
            status = lexerReadChar(lexer, &character);
            if (status != LEXER_OK) {
                return status;
            }

            previousIsDigit = false;
        } else {
            break;
        }
    }

    return previousIsDigit ? LEXER_OK : LEXER_ERROR;
}

/**
 * @brief Reads a decimal integer or floating-point literal.
 * @param firstCharacter Already consumed digit or dot followed by a digit.
 * @param outToken Receives the type and value only on success.
 */
static LexerStatus lexerReadNumber(Lexer *lexer, int firstCharacter,
                                   Token *outToken) {
    char *buffer = NULL;
    size_t length = 0;
    size_t capacity = 0;
    bool isDouble = firstCharacter == '.';
    int character;

    LexerStatus status = lexerAppendNumberChar(lexer, &buffer, &length,
                                               &capacity, firstCharacter);
    if (status != LEXER_OK) {
        goto failure;
    }

    status =
        lexerReadNumberDigits(lexer, &buffer, &length, &capacity, !isDouble);
    if (status != LEXER_OK) {
        goto failure;
    }

    status = lexerPeekChar(lexer, &character);
    if (status != LEXER_OK) {
        goto failure;
    }

    if (!isDouble && character == '.') {
        status = lexerReadChar(lexer, &character);
        if (status != LEXER_OK) {
            goto failure;
        }

        status = lexerPeekChar(lexer, &character);
        if (status != LEXER_OK) {
            goto failure;
        }

        if (lexerIsDigit(character)) {
            isDouble = true;
            status =
                lexerAppendNumberChar(lexer, &buffer, &length, &capacity, '.');
            if (status != LEXER_OK) {
                goto failure;
            }

            status = lexerReadNumberDigits(lexer, &buffer, &length, &capacity,
                                           false);
            if (status != LEXER_OK) {
                goto failure;
            }
        } else {
            /* The dot belongs to the next token, e.g. in 42.toDouble. */
            lexer->pendingCharacter = '.';
            lexer->column--;
        }
    }

    status = lexerPeekChar(lexer, &character);
    if (status != LEXER_OK) {
        goto failure;
    }

    if (character == 'e' || character == 'E') {
        isDouble = true;
        status = lexerReadChar(lexer, &character);
        if (status != LEXER_OK) {
            goto failure;
        }

        status = lexerAppendNumberChar(lexer, &buffer, &length, &capacity,
                                       character);
        if (status != LEXER_OK) {
            goto failure;
        }

        status = lexerPeekChar(lexer, &character);
        if (status != LEXER_OK) {
            goto failure;
        }

        if (character == '+' || character == '-') {
            status = lexerReadChar(lexer, &character);
            if (status != LEXER_OK) {
                goto failure;
            }

            status = lexerAppendNumberChar(lexer, &buffer, &length, &capacity,
                                           character);
            if (status != LEXER_OK) {
                goto failure;
            }
        }

        status =
            lexerReadNumberDigits(lexer, &buffer, &length, &capacity, false);
        if (status != LEXER_OK) {
            goto failure;
        }
    }

    status = lexerPeekChar(lexer, &character);
    if (status != LEXER_OK) {
        goto failure;
    }

    if (isDouble && (character == 'd' || character == 'D')) {
        status = lexerReadChar(lexer, &character);
        if (status != LEXER_OK) {
            goto failure;
        }
    }

    status = lexerPeekChar(lexer, &character);
    if (status != LEXER_OK) {
        goto failure;
    }

    if (lexerIsDigit(character) || (character >= 'a' && character <= 'z') ||
        (character >= 'A' && character <= 'Z') || character == '_') {
        goto lexicalError;
    }

    char *end;
    TokenAttribute attribute = {0};
    errno = 0;

    if (isDouble) {
        double value = strtod(buffer, &end);
        if (*end != '\0' || !isfinite(value) ||
            (errno == ERANGE && value == 0.0)) {
            goto lexicalError;
        }

        attribute.doubleValue = value;
    } else {
        long long value = strtoll(buffer, &end, 10);
        if (*end != '\0' || errno == ERANGE || value > INT64_MAX ||
            (buffer[0] == '0' && value != 0)) {
            goto lexicalError;
        }

        attribute.intValue = (int64_t)value;
    }

    free(buffer);
    outToken->attribute = attribute;
    outToken->type = isDouble ? TOKEN_DOUBLE_LITERAL : TOKEN_INT_LITERAL;
    return LEXER_OK;

lexicalError:
    status = LEXER_ERROR;

failure:
    free(buffer);
    if (status == LEXER_ERROR) {
        setErrCode(LEXER_ERROR);
        fprintf(stderr, "Lexical error at %zu:%zu: invalid numeric literal.\n",
                outToken->line, outToken->column);
    }
    return status;
}

/** @brief Skips a line comment, leaving LF or EOF for getToken(). */
static LexerStatus lexerSkipLineComment(Lexer *lexer) {
    while (true) {
        int character;
        LexerStatus status = lexerPeekChar(lexer, &character);
        if (status != LEXER_OK) {
            return status;
        }

        if (character == '\n' || character == EOF) {
            return LEXER_OK;
        }

        status = lexerReadChar(lexer, &character);
        if (status != LEXER_OK) {
            return status;
        }
    }
}

/** @brief Skips a possibly nested block comment after its opening delimiter. */
static LexerStatus lexerSkipBlockComment(Lexer *lexer,
                                         const Token *commentStart,
                                         bool *containsNewline) {
    size_t depth = 1;
    *containsNewline = false;

    while (depth > 0) {
        int character;
        LexerStatus status = lexerReadChar(lexer, &character);
        if (status != LEXER_OK) {
            return status;
        }

        if (character == EOF) {
            setErrCode(LEXER_ERROR);
            fprintf(stderr,
                    "Lexical error at %zu:%zu: unterminated block comment.\n",
                    commentStart->line, commentStart->column);
            return LEXER_ERROR;
        }

        if (character == '\n') {
            *containsNewline = true;
            continue;
        }

        if (character != '/' && character != '*') {
            continue;
        }

        int nextCharacter;
        status = lexerPeekChar(lexer, &nextCharacter);
        if (status != LEXER_OK) {
            return status;
        }

        if (character == '/' && nextCharacter == '*') {
            status = lexerReadChar(lexer, &nextCharacter);
            if (status != LEXER_OK) {
                return status;
            }

            if (depth == SIZE_MAX) {
                setErrCode(LEXER_INTERNAL_ERROR);
                fprintf(stderr,
                        "Internal lexer error at %zu:%zu: "
                        "block comment nesting is too deep.\n",
                        lexer->line, lexer->column);
                return LEXER_INTERNAL_ERROR;
            }

            depth++;
        } else if (character == '*' && nextCharacter == '/') {
            status = lexerReadChar(lexer, &nextCharacter);
            if (status != LEXER_OK) {
                return status;
            }

            depth--;
        }
    }

    return LEXER_OK;
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
Lexer *lexerCreate(FILE *input) {
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
    lexer->pendingCharacter = EOF;
    lexer->line = 1;
    lexer->column = 1;
    lexer->lineHasToken = false;
    lexer->parenDepth = 0;
    lexer->newlineSinceToken = false;
    lexer->physicalIndent = 0;
    lexer->atPhysicalLineStart = true;
    lexer->physicalLineHasToken = false;
    lexer->leadingTabColumn = 0;
    lexer->logicalIndent = 0;

    return lexer;
}

/**
 * @brief Reads the next token from the input stream.
 *
 * @param lexer Initialized lexer context.
 * @param outToken Initialized output token.
 * @return LEXER_OK when a token is returned;
 *         LEXER_ERROR on a lexical error;
 *         LEXER_INTERNAL_ERROR on an internal failure or input
 *         not supported by the current implementation.
 */
LexerStatus getToken(Lexer *lexer, Token *outToken) {
    tokenFree(outToken);

    int character;
    LexerStatus status;
    bool requireLineEnd = false;

    while (true) {
        outToken->line = lexer->line;
        outToken->column = lexer->column;

        status = lexerPeekChar(lexer, &character);
        if (status != LEXER_OK) {
            return status;
        }

        if (character == EOF) {
            if (lexer->lineHasToken && lexer->parenDepth == 0) {
                outToken->type = TOKEN_NEWLINE;
                lexer->lineHasToken = false;
                return LEXER_OK;
            }

            outToken->type = TOKEN_EOF;
            return LEXER_OK;
        }

        status = lexerReadChar(lexer, &character);
        if (status != LEXER_OK) {
            return status;
        }

        if (character == ' ') {
            if (lexer->atPhysicalLineStart) {
                lexer->physicalIndent++;
            }
            continue;
        }

        if (character == '\t') {
            if (lexer->atPhysicalLineStart && lexer->leadingTabColumn == 0) {
                lexer->leadingTabColumn = outToken->column;
            }
            continue;
        }

        if (character == '\n') {
            lexer->physicalIndent = 0;
            lexer->atPhysicalLineStart = true;
            lexer->physicalLineHasToken = false;
            lexer->leadingTabColumn = 0;
            lexer->newlineSinceToken = true;
            requireLineEnd = false;

            if (lexer->parenDepth == 0 && lexer->lineHasToken) {
                outToken->type = TOKEN_NEWLINE;
                lexer->lineHasToken = false;
                return LEXER_OK;
            }
            continue;
        }

        if (requireLineEnd) {
            setErrCode(LEXER_ERROR);
            fprintf(stderr,
                    "Lexical error at %zu:%zu: "
                    "only whitespace may follow a multiline block comment "
                    "on its closing line.\n",
                    outToken->line, outToken->column);
            return LEXER_ERROR;
        }

        if (character == '/') {
            int nextCharacter;
            status = lexerPeekChar(lexer, &nextCharacter);
            if (status != LEXER_OK) {
                return status;
            }

            if (nextCharacter == '/' || nextCharacter == '*') {
                status = lexerReadChar(lexer, &nextCharacter);
                if (status != LEXER_OK) {
                    return status;
                }

                lexer->atPhysicalLineStart = false;

                if (nextCharacter == '/') {
                    status = lexerSkipLineComment(lexer);
                    if (status != LEXER_OK) {
                        return status;
                    }
                } else {
                    bool containsNewline;
                    status = lexerSkipBlockComment(lexer, outToken,
                                                   &containsNewline);
                    if (status != LEXER_OK) {
                        return status;
                    }

                    if (containsNewline) {
                        lexer->physicalIndent = 0;
                        lexer->physicalLineHasToken = false;
                        lexer->leadingTabColumn = 0;
                        lexer->newlineSinceToken = true;
                        requireLineEnd = true;
                    }
                }
                continue;
            }
        }

        bool startsPhysicalLine = !lexer->physicalLineHasToken;
        lexer->atPhysicalLineStart = false;
        lexer->physicalLineHasToken = true;

        if (lexer->leadingTabColumn != 0) {
            size_t tabColumn = lexer->leadingTabColumn;
            lexer->leadingTabColumn = 0;
            setErrCode(LEXER_ERROR);
            fprintf(stderr,
                    "Lexical error at %zu:%zu: "
                    "tab is not allowed in indentation.\n",
                    outToken->line, tabColumn);
            return LEXER_ERROR;
        }

        if (startsPhysicalLine && lexer->parenDepth > 0 &&
            lexer->physicalIndent <= lexer->logicalIndent) {
            setErrCode(LEXER_ERROR);
            fprintf(stderr,
                    "Lexical error at %zu:%zu: "
                    "continuation line must be indented more than "
                    "the first line of the logical line.\n",
                    outToken->line, outToken->column);
            return LEXER_ERROR;
        }

        break;
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
        type = TOKEN_DIVIDE;
        break;

    case '=': {
        type = TOKEN_ASSIGN;

        int nextCharacter;
        status = lexerPeekChar(lexer, &nextCharacter);

        if (status != LEXER_OK) {
            return status;
        }

        if (nextCharacter == '=') {
            status = lexerReadChar(lexer, &nextCharacter);

            if (status != LEXER_OK) {
                return status;
            }

            type = TOKEN_EQUAL;
        }

        break;
    }

    case '!': {
        type = TOKEN_NOT;

        int nextCharacter;
        status = lexerPeekChar(lexer, &nextCharacter);

        if (status != LEXER_OK) {
            return status;
        }

        if (nextCharacter == '=') {
            status = lexerReadChar(lexer, &nextCharacter);

            if (status != LEXER_OK) {
                return status;
            }

            type = TOKEN_NOT_EQUAL;
        }

        break;
    }

    case '<': {
        type = TOKEN_LESS;

        int nextCharacter;
        status = lexerPeekChar(lexer, &nextCharacter);

        if (status != LEXER_OK) {
            return status;
        }

        if (nextCharacter == '=' || nextCharacter == '-') {
            status = lexerReadChar(lexer, &nextCharacter);

            if (status != LEXER_OK) {
                return status;
            }

            if (nextCharacter == '=') {
                type = TOKEN_LESS_EQUAL;
            } else {
                type = TOKEN_LEFT_ARROW;
            }
        }

        break;
    }

    case '>': {
        type = TOKEN_GREATER;

        int nextCharacter;
        status = lexerPeekChar(lexer, &nextCharacter);

        if (status != LEXER_OK) {
            return status;
        }

        if (nextCharacter == '=') {
            status = lexerReadChar(lexer, &nextCharacter);

            if (status != LEXER_OK) {
                return status;
            }

            type = TOKEN_GREATER_EQUAL;
        }

        break;
    }

    case '&':
    case '|': {
        int nextCharacter;
        status = lexerPeekChar(lexer, &nextCharacter);

        if (status != LEXER_OK) {
            return status;
        }

        if (nextCharacter != character) {
            setErrCode(LEXER_ERROR);

            fprintf(stderr,
                    "Lexical error at %zu:%zu: expected '%c' after '%c'.\n",
                    outToken->line, outToken->column, character, character);

            return LEXER_ERROR;
        }

        type = (character == '&') ? TOKEN_AND : TOKEN_OR;

        status = lexerReadChar(lexer, &nextCharacter);

        if (status != LEXER_OK) {
            return status;
        }

        break;
    }

    case '(': {
        type = TOKEN_LPAREN;

        lexer->parenDepth += 1;

        break;
    }

    case ')': {
        type = TOKEN_RPAREN;

        if (lexer->parenDepth > 0) {
            lexer->parenDepth -= 1;
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
        int nextCharacter;
        status = lexerPeekChar(lexer, &nextCharacter);
        if (status != LEXER_OK) {
            return status;
        }

        if (lexerIsDigit(nextCharacter)) {
            status = lexerReadNumber(lexer, character, outToken);
            if (status != LEXER_OK) {
                return status;
            }
            type = outToken->type;
        } else {
            type = TOKEN_DOT;
        }
        break;
    }

    case '@': {
        type = TOKEN_AT;
        break;
    }

    default:
        if (lexerIsDigit(character)) {
            status = lexerReadNumber(lexer, character, outToken);
            if (status != LEXER_OK) {
                return status;
            }
            type = outToken->type;
            break;
        }

        if ((character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') || character == '_') {
            status = lexerReadIdentifier(lexer, character,
                                         &outToken->attribute.text);

            if (status != LEXER_OK) {
                return status;
            }

            const char *text = outToken->attribute.text.data;
            type = lexerIdentifierType(text);

            if (type != TOKEN_IDENTIFIER) {
                bool booleanValue =
                    type == TOKEN_BOOL_LITERAL && strcmp(text, "true") == 0;

                free(outToken->attribute.text.data);
                outToken->attribute = (TokenAttribute){0};

                if (type == TOKEN_BOOL_LITERAL) {
                    outToken->attribute.boolValue = booleanValue;
                }
            }

            break;
        }
        /* TODO: Implement recognition of the remaining tokens. */
        setErrCode(LEXER_INTERNAL_ERROR);

        fprintf(stderr,
                "Internal lexer error at %zu:%zu: "
                "token recognition is not implemented for byte 0x%02X.\n",
                outToken->line, outToken->column, (unsigned int)character);

        return LEXER_INTERNAL_ERROR;
    }

    outToken->type = type;
    outToken->startsLogicalLine = !lexer->lineHasToken;

    if (outToken->startsLogicalLine) {
        outToken->indent = lexer->physicalIndent;
        lexer->logicalIndent = lexer->physicalIndent;
    }

    outToken->newlineBefore = lexer->newlineSinceToken;

    lexer->lineHasToken = true;
    lexer->newlineSinceToken = false;

    return LEXER_OK;
}

/**
 * @brief Releases the lexer context without closing its input stream.
 *
 * @param lexer Context to release; NULL is allowed.
 */
void lexerDestroy(Lexer *lexer) { free(lexer); }