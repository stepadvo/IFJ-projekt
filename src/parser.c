/**
 * @file parser.c
 * @brief Recursive-descent parser for the currently supported IFJ26 syntax.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 *
 * Parses the mandatory prolog and basic single-line function definitions.
 * Indented blocks and complete expressions will be added later.
 */

#include "parser.h"
#include "compiler.h"
#include "lexer.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Lexer *lexer = NULL;
static Token currentToken = {0};

/**
 * @brief Reads the next token from the lexer.
 * @return True if a token was read successfully, false on a lexer error.
 *
 * getToken() releases the previous token before storing the next one.
 */
static bool advance(void) {
    /** getToken and save LexerStatus */
    LexerStatus status = getToken(lexer, &currentToken);

    /** set lex/internal (1/99) error if LexerStatus not OK */
    if (status == LEXER_OK)
        return true;
    if (status == LEXER_INTERNAL_ERROR)
        ERRCODE = 99;
    else if (ERRCODE == 0)
        ERRCODE = 1;
    return false;
}

/**
 * @brief Reports a syntax error at the current token.
 * @param message Description of the syntax error.
 *
 * Sets ERRCODE to 2 if no earlier error has been recorded.
 */
static void syntaxError(const char *message) {
    fprintf(stderr, "Syntax error at line %zu: %s\n", currentToken.line,
            message);
    if (ERRCODE == 0) {
        ERRCODE = 2;
    }

    return;
}

/**
 * @brief Checks the expected token type and advances the lexer.
 * @param expected Required token type.
 * @param errMessage Error message used if the token does not match.
 * @return True if matching and advancing succeed, false otherwise.
 */
static bool match(TokenType expected, const char *errMessage) {
    if (currentToken.type != expected) {
        syntaxError(errMessage);
        return false;
    }

    return advance();
}

/**
 * @brief Creates a heap-allocated copy of the current identifier.
 * @return Copied name owned by the caller; it must be freed or passed to the
 * AST.
 *
 * Must be called before advancing, since the lexer owns the token's text.
 */
static char *copyName(void) {
    /** save the name length and reserve space for the terminating '\0' */
    size_t length = currentToken.attribute.text.length;
    char *name = malloc(length + 1);
    checkMalloc(name);

    /** copy the name including its terminating '\0' */
    memcpy(name, currentToken.attribute.text.data, length + 1);
    return name;
}

/**
 * @brief Parses the required "import ifj26.*" prolog.
 * @return True if the prolog is valid, false otherwise.
 */
static bool parseProlog(void) {
    /** sequence of prolog tokenTypes
     * prolog has to be "import ifj26.*"
     */
    const TokenType prolog[] = {TOKEN_KW_IMPORT, TOKEN_IDENTIFIER, TOKEN_DOT,
                                TOKEN_MULTIPLY, TOKEN_NEWLINE};
    /** sequence of error messages corresponding to tokenType synErrors */
    const char *errMsg[] = {
        "expected 'import'", "expected 'ifj26' after 'import'",
        "expected '.' after 'ifj26'", "expected '*' after '.'",
        "expected '\\n' after '*'"};

    /** check if matches, else print corresponding errMsg */
    for (size_t i = 0; i < 5; i++) {
        if (i == 1 && currentToken.type == TOKEN_IDENTIFIER) {
            if (strcmp(currentToken.attribute.text.data, "ifj26")) {
                syntaxError(errMsg[i]);
                return false;
            }
        }

        if (!match(prolog[i], errMsg[i]))
            return false;
    }
    return true;
}

/**
 * @brief Parses the optional @main annotation before a function definition.
 * @param isMain Output flag indicating whether @main was present.
 * @return True if the annotation is valid or absent, false on error.
 *
 * Accepts @main on the same line as def or on the preceding line.
 */
static bool parseMainAnnotation(bool *isMain) {
    *isMain = false;

    /** S -> @ */
    if (currentToken.type != TOKEN_AT) {
        return true;
    }

    size_t atLine = currentToken.line;
    size_t atColumn = currentToken.column;

    if (!advance()) {
        return false;
    }

    /** @ -> not '_' -> main */
    if (currentToken.type != TOKEN_IDENTIFIER ||
        strcmp(currentToken.attribute.text.data, "main") != 0 ||
        currentToken.line != atLine || currentToken.column != atColumn + 1) {
        syntaxError("expected '@main' without whitespace");
        return false;
    }

    *isMain = true;

    if (!advance()) {
        return false;
    }

    /** @main
     *  def functionName(...
     *
     * is allowed
     */
    if (currentToken.type == TOKEN_NEWLINE) {
        return advance();
    }

    return true;
}

/**
 * @brief Parses a type keyword and converts it to varType.
 * @param type Output parameter receiving the parsed type.
 * @return True if a valid type was consumed, false otherwise.
 */
static bool parseType(varType *type) {
    /** convert the type keyword into the corresponding internal type */
    switch (currentToken.type) {
    case TOKEN_KW_INT:
        *type = INT;
        break;
    case TOKEN_KW_DOUBLE:
        *type = DOUBLE;
        break;
    case TOKEN_KW_STRING:
        *type = STRING;
        break;
    case TOKEN_KW_BOOLEAN:
        *type = BOOL;
        break;
    case TOKEN_KW_UNIT:
        *type = UNIT;
        break;
    default:
        syntaxError("expected type");
        return false;
    }

    /** the type was recognized, move to the next token */
    return advance();
}

/**
 * @brief Releases a partially constructed function parameter array.
 * @param params Parameter array to release, or NULL if empty.
 * @param count Number of initialized entries in the array.
 */
static void freeParameters(astFuncParam *params, size_t count) {
    /** each parameter name was allocated separately by copyName() */
    for (size_t i = 0; i < count; i++) {
        free(params[i].name);
    }

    /** free the array after all its names */
    free(params);
}

/**
 * @brief Parses a comma-separated list of typed function parameters.
 * @param params Output pointer to the dynamically allocated parameter array.
 * @param count Output number of parsed parameters.
 * @return True if the list is valid, false on error.
 *
 * The closing ')' is left for the caller to consume. On failure, the caller
 * must release any stored parameters using freeParameters().
 */
static bool parseParameters(astFuncParam **params, size_t *count) {
    size_t capacity = 0;

    /** a closing ')' immediately means the parameter list is empty */
    if (currentToken.type == TOKEN_RPAREN) {
        return true;
    }

    /** each iteration reads one parameter: name -> : -> Type */
    while (true) {
        if (currentToken.type != TOKEN_IDENTIFIER) {
            syntaxError("expected parameter name");
            return false;
        }

        /** save the name before advancing frees the original token */
        char *name = copyName();
        if (!advance() ||
            !match(TOKEN_COLON, "expected ':' after parameter name")) {
            free(name);
            return false;
        }

        /** read the declared parameter type */
        varType type;
        if (!parseType(&type)) {
            free(name);
            return false;
        }

        /** grow the array when there is no space for another parameter */
        if (*count == capacity) {
            size_t newCapacity = capacity == 0 ? 4 : capacity * 2;
            astFuncParam *newParams =
                realloc(*params, newCapacity * sizeof(**params));
            checkMalloc(newParams);
            *params = newParams;
            capacity = newCapacity;
        }

        /** store the name and type in the next free slot */
        (*params)[*count] = (astFuncParam){.name = name, .type = type};
        (*count)++;

        /** a comma means another parameter follows; otherwise stop here */
        if (currentToken.type != TOKEN_COMMA) {
            return true;
        }
        if (!advance()) {
            return false;
        }
    }
}

/**
 * @brief Parses a function definition and creates its AST node.
 * @param isMain Whether the function has an @main annotation.
 * @return New AST_FUNC_DEF node on success, NULL on parsing failure.
 *
 * Supports typed parameters, an optional return type, and a simple literal
 * body. Blocks and general expressions are not supported yet.
 */
static astNode *parseFunctionDef(bool isMain) {
    /** function -> def */
    if (!match(TOKEN_KW_DEF, "expected 'def'")) {
        return NULL;
    }

    /** def -> name */
    if (currentToken.type != TOKEN_IDENTIFIER) {
        syntaxError("expected function name");
        return NULL;
    }

    /** save name and parse paramList */
    char *name = copyName();
    astFuncParam *params = NULL;
    size_t paramCount = 0;

    if (!advance() || !match(TOKEN_LPAREN, "expected '('") ||
        !parseParameters(&params, &paramCount) ||
        !match(TOKEN_RPAREN, "expected ')' after parameters")) {
        freeParameters(params, paramCount);
        free(name);
        return NULL;
    }

    /** if: ) -> : -> returnType
     * defaults to unknown if:
     *      ) -> =
     */
    varType returnType = UNKNOWN;
    if (currentToken.type == TOKEN_COLON) {
        /** the same type parser works for parameters and return types */
        if (!advance() || !parseType(&returnType)) {
            freeParameters(params, paramCount);
            free(name);
            return NULL;
        }
    }

    /** ) or : returnType -> = */
    if (!match(TOKEN_ASSIGN, "expected '=' after function header")) {
        freeParameters(params, paramCount);
        free(name);
        return NULL;
    }

    /** = -> functionBody */
    astNode *body = NULL;
    if (currentToken.type == TOKEN_INT_LITERAL ||
        currentToken.type == TOKEN_DOUBLE_LITERAL ||
        currentToken.type == TOKEN_STRING_LITERAL ||
        currentToken.type == TOKEN_BOOL_LITERAL) {
        body = createLiteralNode(&currentToken);
        if (!advance()) {
            freeAst(body);
            freeParameters(params, paramCount);
            free(name);
            return NULL;
        }
        /** if the body starts with '(', only the Unit literal "()" is supported
         * other parenthesized expressions will be handled later
         */
    } else if (currentToken.type == TOKEN_LPAREN) {
        if (!advance() || !match(TOKEN_RPAREN, "expected ')' for Unit value")) {
            freeParameters(params, paramCount);
            free(name);
            return NULL;
        }
        body = createUnitNode();
        /** blocks and complete expressions are TODO */
    } else {
        syntaxError("expected simple expression (blocks not supported yet)");
        freeParameters(params, paramCount);
        free(name);
        return NULL;
    }

    /** since TODO, body ends there, anything else is error */
    if (currentToken.type != TOKEN_NEWLINE && currentToken.type != TOKEN_EOF) {
        syntaxError("expected end of function body");
        freeAst(body);
        freeParameters(params, paramCount);
        free(name);
        return NULL;
    }

    /** body ends with \n or EOF */
    if (currentToken.type == TOKEN_NEWLINE && !advance()) {
        freeAst(body);
        freeParameters(params, paramCount);
        free(name);
        return NULL;
    }

    /** create a funcDefNode and save data */
    astNode *function =
        createFuncDefNode(name, isMain, params, paramCount, body);
    if (function == NULL) {
        freeAst(body);
        freeParameters(params, paramCount);
        free(name);
        ERRCODE = 99;
        return NULL;
    }

    /** def functionName(): -> returnType */
    function->data.funcDef.explicitReturnType = returnType;
    return function;
}

/**
 * @brief Parses the source program from stdin and creates its AST.
 * @return Program AST on success, NULL on error.
 *
 * The caller owns the returned AST and must release it with freeAst().
 */
astNode *parse(void) {
    /** init lexer, currentToken and rootNode */
    currentToken = (Token){0};
    lexer = lexerCreate(stdin);
    if (lexer == NULL) {
        fprintf(stderr, "Internal error: could not create lexer\n");
        ERRCODE = 99;
        return NULL;
    }
    astNode *root = NULL;

    /** main parsing loop, after we get a token and prolog is correct */
    if (advance() && parseProlog()) {
        root = createProgramNode();

        while (currentToken.type != TOKEN_EOF) {
            bool isMain = false;
            /** check for @main, parse funcDef and add it to rootNode */
            if (!parseMainAnnotation(&isMain)) {
                break;
            }
            if (currentToken.type != TOKEN_KW_DEF) {
                syntaxError("expected function definition");
                break;
            }

            astNode *function = parseFunctionDef(isMain);
            if (function == NULL) {
                break;
            }
            addFunctionToProgram(root, function);
        }
    }

    /** on error free the partial AST and release lexer */
    if (ERRCODE != 0) {
        freeAst(root);
        root = NULL;
    }

    tokenFree(&currentToken);
    lexerDestroy(lexer);
    lexer = NULL;
    return root;
}
