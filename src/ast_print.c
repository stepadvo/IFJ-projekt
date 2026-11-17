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

/** @brief Returns the source symbol for a unary operator. */
static const char *unopSymbol(astUnopType unopType) {
    switch (unopType) {
    case NOT:
        return "!";
    case UNARY_PLUS:
        return "+";
    case UNARY_MINUS:
        return "-";
    default:
        return "?!";
    }
}

/** @brief Returns the source symbol for a binary operator. */
static const char *binopSymbol(astBinopType binopType) {
    switch (binopType) {
    case PLUS:
        return "+";
    case CONCATENATION:
        return "+";
    case MINUS:
        return "-";
    case MULTIPLY:
        return "*";
    case DIVIDE:
        return "/";
    case I_DIVIDE:
        return "/";
    case EQUAL:
        return "==";
    case NOT_EQUAL:
        return "!=";
    case LESS:
        return "<";
    case LESS_EQUAL:
        return "<=";
    case GREATER:
        return ">";
    case GREATER_EQUAL:
        return ">=";
    case AND:
        return "&&";
    case OR:
        return "||";
    default:
        return "?!";
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
        printf("BinOp (%s)", binopSymbol(node->data.binop.op));
        break;

    case AST_UNOP:
        printf("UnOp (%s)", unopSymbol(node->data.unop.op));
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
