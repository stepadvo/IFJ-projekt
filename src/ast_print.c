/**
 * @file ast_print.c
 * @brief Readable debug output for the IFJ26 Abstract Syntax Tree.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 */

#include "ast_print.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief Returns a readable name for the declared data type. */
static const char *typeName(varType type) {
    switch (type) {
    case UNIT:
        return "Unit";
    case INT:
        return "Int";
    case DOUBLE:
        return "Double";
    case STRING:
        return "String";
    case BOOL:
        return "Boolean";
    case UNKNOWN:
        return "Unknown";
    case ANY:
        return "Any";
    default:
        return "InvalidType";
    }
}

/** @brief Converts token types to their actual enum names for debugging. */
static const char *tokenName(TokenType unopType, astBinopType binopType,
                             bool isBinop) {
    if (isBinop) {
        switch (unopType) {
        case TOKEN_INVALID:
            return "TOKEN_INVALID";
        case TOKEN_EOF:
            return "TOKEN_EOF";
        case TOKEN_NEWLINE:
            return "TOKEN_NEWLINE";
        case TOKEN_IDENTIFIER:
            return "TOKEN_IDENTIFIER";
        case TOKEN_INT_LITERAL:
            return "TOKEN_INT_LITERAL";
        case TOKEN_DOUBLE_LITERAL:
            return "TOKEN_DOUBLE_LITERAL";
        case TOKEN_STRING_LITERAL:
            return "TOKEN_STRING_LITERAL";
        case TOKEN_BOOL_LITERAL:
            return "TOKEN_BOOL_LITERAL";
        case TOKEN_KW_DEF:
            return "TOKEN_KW_DEF";
        case TOKEN_KW_DO:
            return "TOKEN_KW_DO";
        case TOKEN_KW_DOUBLE:
            return "TOKEN_KW_DOUBLE";
        case TOKEN_KW_ELSE:
            return "TOKEN_KW_ELSE";
        case TOKEN_KW_IF:
            return "TOKEN_KW_IF";
        case TOKEN_KW_IMPORT:
            return "TOKEN_KW_IMPORT";
        case TOKEN_KW_INT:
            return "TOKEN_KW_INT";
        case TOKEN_KW_RETURN:
            return "TOKEN_KW_RETURN";
        case TOKEN_KW_STRING:
            return "TOKEN_KW_STRING";
        case TOKEN_KW_THEN:
            return "TOKEN_KW_THEN";
        case TOKEN_KW_UNIT:
            return "TOKEN_KW_UNIT";
        case TOKEN_KW_VAL:
            return "TOKEN_KW_VAL";
        case TOKEN_KW_VAR:
            return "TOKEN_KW_VAR";
        case TOKEN_KW_WHILE:
            return "TOKEN_KW_WHILE";
        case TOKEN_KW_BOOLEAN:
            return "TOKEN_KW_BOOLEAN";
        case TOKEN_KW_FOR:
            return "TOKEN_KW_FOR";
        case TOKEN_KW_CASE:
            return "TOKEN_KW_CASE";
        case TOKEN_KW_CLASS:
            return "TOKEN_KW_CLASS";
        case TOKEN_KW_EXTENSION:
            return "TOKEN_KW_EXTENSION";
        case TOKEN_KW_LIST:
            return "TOKEN_KW_LIST";
        case TOKEN_KW_MATCH:
            return "TOKEN_KW_MATCH";
        case TOKEN_KW_TYPE:
            return "TOKEN_KW_TYPE";
        case TOKEN_ASSIGN:
            return "TOKEN_ASSIGN";
        case TOKEN_PLUS:
            return "TOKEN_PLUS";
        case TOKEN_MINUS:
            return "TOKEN_MINUS";
        case TOKEN_MULTIPLY:
            return "TOKEN_MULTIPLY";
        case TOKEN_DIVIDE:
            return "TOKEN_DIVIDE";
        case TOKEN_EQUAL:
            return "TOKEN_EQUAL";
        case TOKEN_NOT_EQUAL:
            return "TOKEN_NOT_EQUAL";
        case TOKEN_LESS:
            return "TOKEN_LESS";
        case TOKEN_LESS_EQUAL:
            return "TOKEN_LESS_EQUAL";
        case TOKEN_GREATER:
            return "TOKEN_GREATER";
        case TOKEN_GREATER_EQUAL:
            return "TOKEN_GREATER_EQUAL";
        case TOKEN_NOT:
            return "TOKEN_NOT";
        case TOKEN_AND:
            return "TOKEN_AND";
        case TOKEN_OR:
            return "TOKEN_OR";
        case TOKEN_LEFT_ARROW:
            return "TOKEN_LEFT_ARROW";
        case TOKEN_LPAREN:
            return "TOKEN_LPAREN";
        case TOKEN_RPAREN:
            return "TOKEN_RPAREN";
        case TOKEN_COLON:
            return "TOKEN_COLON";
        case TOKEN_COMMA:
            return "TOKEN_COMMA";
        case TOKEN_SEMICOLON:
            return "TOKEN_SEMICOLON";
        case TOKEN_DOT:
            return "TOKEN_DOT";
        case TOKEN_AT:
            return "TOKEN_AT";
        default:
            return "UNKNOWN_TOKEN";
        }
    } else {
        switch (binopType) {
        case PLUS:
            return "TOKEN_PLUS";
        case CONCATENATION:
            return "TOKEN_CONCATENATION";
        case MINUS:
            return "TOKEN_MINUS";
        case MULTIPLY:
            return "TOKEN_MULTIPLY";
        case DIVIDE:
            return "TOKEN_DIVIDE";
        case I_DIVIDE:
            return "TOKEN_I_DIVIDE";
        case EQUAL:
            return "TOKEN_EQUAL";
        case NOT_EQUAL:
            return "TOKEN_NOT_EQUAL";
        case LESS:
            return "TOKEN_LESS";
        case LESS_EQUAL:
            return "TOKEN_EQUAL";
        case GREATER:
            return "TOKEN_GREATER";
        case GREATER_EQUAL:
            return "TOKEN_GREATER_EQUAL";
        case AND:
            return "TOKEN_AND";
        case OR:
            return "TOKEN_OR";
        default:
            return "?!";
        }
    }
}

/** @brief Returns the symbol used in the source for an operator token. */
static const char *operatorSymbol(TokenType unopType, astBinopType binopType,
                                  bool isBinop) {
    if (!isBinop) {
        switch (unopType) {
        case TOKEN_ASSIGN:
            return "=";
        case TOKEN_PLUS:
            return "+";
        case TOKEN_MINUS:
            return "-";
        case TOKEN_MULTIPLY:
            return "*";
        case TOKEN_DIVIDE:
            return "/";
        case TOKEN_EQUAL:
            return "==";
        case TOKEN_NOT_EQUAL:
            return "!=";
        case TOKEN_LESS:
            return "<";
        case TOKEN_LESS_EQUAL:
            return "<=";
        case TOKEN_GREATER:
            return ">";
        case TOKEN_GREATER_EQUAL:
            return ">=";
        case TOKEN_NOT:
            return "!";
        case TOKEN_AND:
            return "&&";
        case TOKEN_OR:
            return "||";
        case TOKEN_LEFT_ARROW:
            return "<-";
        default:
            return "?!";
        }
    } else {
        switch (binopType) {
        case PLUS: /**< Plus: "+". */
            return "+";
        case CONCATENATION: /**< Concatenation: "[str] + [str]" */
            return "+";
        case MINUS: /**< Minus: "-". */
            return "-";
        case MULTIPLY: /**< Multiplication: "*". */
            return "*";
        case DIVIDE: /**< Division: "[float] / [float]". */
            return "/";
        case I_DIVIDE: /**< Integer divison: "[int] / [int]" */
            return "/";
        case EQUAL: /**< Equality: "==". */
            return "==";
        case NOT_EQUAL: /**< Inequality: "!=". */
            return "!=";
        case LESS: /**< Less than: "<". */
            return "<";
        case LESS_EQUAL: /**< Less than or equal to: "<=". */
            return "<=";
        case GREATER: /**< Greater than: ">". */
            return ">";
        case GREATER_EQUAL: /**< Greater than or equal to: ">=". */
            return ">=";
        case AND: /**< Logical conjunction: "&&". */
            return "&&";
        case OR: /**< Logical disjunction: "||". */
            return "||";
        default:
            return "?!";
        }
    }
}

/**
 * @brief Prints string contents without breaking the layout of the tree.
 *
 * Newlines, tabs, quotes and backslashes are escaped. The length comes from
 * TokenText, so embedded NUL bytes do not truncate the debug output.
 */
static void printQuotedText(const TokenText *text) {
    putchar('"');
    if (text == NULL || text->data == NULL) {
        fputs("<null>", stdout);
    } else {
        for (size_t i = 0; i < text->length; i++) {
            unsigned char ch = (unsigned char)text->data[i];
            switch (ch) {
            case '\\':
                fputs("\\\\", stdout);
                break;
            case '"':
                fputs("\\\"", stdout);
                break;
            case '\n':
                fputs("\\n", stdout);
                break;
            case '\r':
                fputs("\\r", stdout);
                break;
            case '\t':
                fputs("\\t", stdout);
                break;
            case '\0':
                fputs("\\0", stdout);
                break;
            default:
                if (ch < 32 || ch == 127) {
                    printf("\\x%02X", ch);
                } else {
                    putchar(ch);
                }
                break;
            }
        }
    }
    putchar('"');
}

/** @brief Prints the current node's summary without traversing children. */
static void printNodeSummary(const astNode *node) {
    switch (node->type) {
    case AST_PROGRAM:
        printf("Program (%zu %s)", node->data.program.count,
               node->data.program.count == 1 ? "function" : "functions");
        break;

    case AST_FUNC_DEF:
        printf("%sFunction %s(", node->data.funcDef.isMain ? "@main " : "",
               node->data.funcDef.name != NULL ? node->data.funcDef.name
                                               : "<null>");
        for (size_t i = 0; i < node->data.funcDef.paramCount; i++) {
            if (i > 0)
                fputs(", ", stdout);
            const astFuncParam *param = &node->data.funcDef.params[i];
            printf("%s: %s", param->name != NULL ? param->name : "<null>",
                   typeName(param->type));
        }
        putchar(')');
        if (node->data.funcDef.explicitReturnType != UNKNOWN) {
            printf(" -> %s", typeName(node->data.funcDef.explicitReturnType));
        }
        break;

    case AST_BLOCK:
        printf("Block (%zu %s)", node->data.block.count,
               node->data.block.count == 1 ? "statement" : "statements");
        break;

    case AST_VAR_DEF:
        printf("VarDef %s %s", node->data.varDef.isConstant ? "val" : "var",
               node->data.varDef.name != NULL ? node->data.varDef.name
                                              : "<null>");
        if (node->data.varDef.explicitType != UNKNOWN) {
            printf(": %s", typeName(node->data.varDef.explicitType));
        }
        break;

    case AST_ASSIGN:
        printf("Assign %s", node->data.assign.name != NULL
                                ? node->data.assign.name
                                : "<null>");
        break;

    case AST_IF:
        fputs("If", stdout);
        break;

    case AST_WHILE:
        fputs("While", stdout);
        break;

    case AST_FOR:
        printf("For %s (%s)",
               node->data.forStmt.iteratorName != NULL
                   ? node->data.forStmt.iteratorName
                   : "<null>",
               node->data.forStmt.endInclusive ? "to, inclusive"
                                               : "until, exclusive");
        break;

    case AST_FUNC_CALL:
        printf("Call %s", node->data.funcCall.funcName != NULL
                              ? node->data.funcCall.funcName
                              : "<null>");
        if (node->data.funcCall.argCount == 0) {
            fputs("()", stdout);
        } else {
            printf("(%zu %s)", node->data.funcCall.argCount,
                   node->data.funcCall.argCount == 1 ? "argument"
                                                     : "arguments");
        }
        break;

    case AST_RETURN:
        fputs(node->data.returnStmt.value != NULL ? "Return" : "Return (void)",
              stdout);
        break;

    case AST_BINOP:
        printf("BinOp %s [%s]",
               operatorSymbol((TokenType){0}, node->data.binop.op, true),
               tokenName((TokenType){0}, node->data.binop.op, true));
        break;

    case AST_UNOP:
        printf("UnOp %s [%s]",
               operatorSymbol(node->data.unop.op, (astBinopType){0}, false),
               tokenName(node->data.unop.op, (astBinopType){0}, false));
        break;

    case AST_LITERAL:
        switch (node->data.literal.litType) {
        case TOKEN_INT_LITERAL:
            printf("Literal Int: %" PRId64,
                   node->data.literal.attribute.intValue);
            break;
        case TOKEN_DOUBLE_LITERAL:
            printf("Literal Double: %.17g",
                   node->data.literal.attribute.doubleValue);
            break;
        case TOKEN_STRING_LITERAL:
            fputs("Literal String: ", stdout);
            printQuotedText(&node->data.literal.attribute.text);
            break;
        case TOKEN_BOOL_LITERAL:
            printf("Literal Boolean: %s",
                   node->data.literal.attribute.boolValue ? "true" : "false");
            break;
        case TOKEN_LPAREN:
            fputs("Literal Unit: ()", stdout);
            break;
        default:
<<<<<<< HEAD
=======
            printf("Literal [%s]", tokenName(node->data.literal.litType,
                                             (astBinopType){0}, false));
>>>>>>> origin/generator_untested
            break;
        }
        break;

    case AST_IDENTIFIER:
        printf("Identifier %s", node->data.identifier.name != NULL
                                    ? node->data.identifier.name
                                    : "<null>");
        break;

    default:
        printf("Unknown AST node (%d)", (int)node->type);
        break;
    }
}

/**
 * @brief Creates the indent prefix for a node's children.
 * @param prefix Prefix used to print the current node.
 * @param isLast Whether the current node is the last child of its parent.
 * @return An allocated prefix which the caller must free.
 */
static char *childPrefix(const char *prefix, bool isLast) {
    const char *branch = isLast ? "    " : "│   ";
    size_t prefixLength = strlen(prefix);
    size_t branchLength = strlen(branch);
    char *result = malloc(prefixLength + branchLength + 1);
    checkMalloc(result);
    memcpy(result, prefix, prefixLength);
    memcpy(result + prefixLength, branch, branchLength + 1);
    return result;
}

/**
 * @brief Prints a node and all its descendants.
 * @param node The current AST node.
 * @param prefix Indentation inherited from earlier ancestors.
 * @param isLast True if this is its parent's last child.
 * @param label Optional name of the child relation (e.g. "Condition").
 */
static void printTree(const astNode *node, const char *prefix, bool isLast,
                      const char *label) {
    printf("%s%s", prefix, isLast ? "└── " : "├── ");
    if (label != NULL)
        printf("%s: ", label);
    if (node == NULL) {
        puts("(null)");
        return;
    }
    printNodeSummary(node);
    putchar('\n');

    char *next = childPrefix(prefix, isLast);
    switch (node->type) {
    case AST_PROGRAM:
        for (size_t i = 0; i < node->data.program.count; i++) {
            printTree(node->data.program.functions[i], next,
                      i + 1 == node->data.program.count, NULL);
        }
        break;

    case AST_FUNC_DEF:
        printTree(node->data.funcDef.body, next, true, "Body");
        break;

    case AST_BLOCK:
        for (size_t i = 0; i < node->data.block.count; i++) {
            printTree(node->data.block.statements[i], next,
                      i + 1 == node->data.block.count, NULL);
        }
        break;

    case AST_VAR_DEF:
        printTree(node->data.varDef.initializer, next, true, "Initializer");
        break;

    case AST_ASSIGN:
        printTree(node->data.assign.value, next, true, "Value");
        break;

    case AST_IF:
        printTree(node->data.ifStmt.condition, next, false, "Condition");
        printTree(node->data.ifStmt.thenBlock, next,
                  node->data.ifStmt.elseBlock == NULL, "Then");
        if (node->data.ifStmt.elseBlock != NULL) {
            printTree(node->data.ifStmt.elseBlock, next, true, "Else");
        }
        break;

    case AST_WHILE:
        printTree(node->data.whileStmt.condition, next, false, "Condition");
        printTree(node->data.whileStmt.bodyBlock, next, true, "Body");
        break;

    case AST_FOR:
        printTree(node->data.forStmt.start, next, false, "Start");
        printTree(node->data.forStmt.end, next, false, "End");
        if (node->data.forStmt.step != NULL) {
            printTree(node->data.forStmt.step, next, false, "Step");
        }
        for (size_t i = 0; i < node->data.forStmt.guardCount; i++) {
            char label[32];
            snprintf(label, sizeof(label), "Guard %zu", i + 1);
            printTree(node->data.forStmt.guards[i], next, false, label);
        }
        printTree(node->data.forStmt.body, next, true, "Body");
        break;

    case AST_FUNC_CALL:
        for (size_t i = 0; i < node->data.funcCall.argCount; i++) {
            char label[32];
            snprintf(label, sizeof(label), "Arg %zu", i + 1);
            printTree(node->data.funcCall.arguments[i], next,
                      i + 1 == node->data.funcCall.argCount, label);
        }
        break;

    case AST_RETURN:
        if (node->data.returnStmt.value != NULL) {
            printTree(node->data.returnStmt.value, next, true, "Value");
        }
        break;

    case AST_BINOP:
        printTree(node->data.binop.left, next, false, "Left");
        printTree(node->data.binop.right, next, true, "Right");
        break;

    case AST_UNOP:
        printTree(node->data.unop.operand, next, true, "Operand");
        break;

    case AST_LITERAL:
    case AST_IDENTIFIER:
        break;
    }
    free(next);
}

/** @brief Prints the entire AST in a structured, human-readable form. */
void printAst(astNode *root) {
    puts("AST ================================");
    printTree(root, "", true, NULL);
    puts("====================================");
}
