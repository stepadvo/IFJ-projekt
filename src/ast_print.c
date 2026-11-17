#include "ast_print.h"
#include <inttypes.h>
#include <stdio.h>

static void printIndent(int depth) {
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
}

static void printAstRec(astNode *node, int depth) {
    if (node == NULL) {
        printIndent(depth);
        printf("(NULL)\n");
        return;
    }

    printIndent(depth);

    switch (node->type) {
    case AST_PROGRAM:
        printf("Program\n");
        for (size_t i = 0; i < node->data.program.count; i++) {
            printAstRec(node->data.program.functions[i], depth + 1);
        }
        break;

    case AST_FUNC_DEF:
        printf("FuncDef(name: %s, @main: %s)\n", node->data.funcDef.name,
               node->data.funcDef.isMain ? "true" : "false");

        if (node->data.funcDef.paramCount > 0) {
            printIndent(depth + 1);
            printf("Params: ");
            for (size_t i = 0; i < node->data.funcDef.paramCount; i++) {
                printf("%s", node->data.funcDef.params[i].name);
                if (i < node->data.funcDef.paramCount - 1)
                    printf(", ");
            }
            printf("\n");
        }

        printAstRec(node->data.funcDef.body, depth + 1);
        break;

    case AST_BLOCK:
        printf("Block\n");
        for (size_t i = 0; i < node->data.block.count; i++) {
            printAstRec(node->data.block.statements[i], depth + 1);
        }
        break;

    case AST_VAR_DEF:
        printf("VarDef(%s %s)\n", node->data.varDef.isConstant ? "val" : "var",
               node->data.varDef.name);
        printAstRec(node->data.varDef.initializer, depth + 1);
        break;

    case AST_ASSIGN:
        printf("Assign(name: %s)\n", node->data.assign.name);
        printAstRec(node->data.assign.value, depth + 1);
        break;

    case AST_IF:
        printf("If\n");
        printIndent(depth + 1);
        printf("Condition:\n");
        printAstRec(node->data.ifStmt.condition, depth + 2);

        printIndent(depth + 1);
        printf("Then:\n");
        printAstRec(node->data.ifStmt.thenBlock, depth + 2);

        if (node->data.ifStmt.elseBlock != NULL) {
            printIndent(depth + 1);
            printf("Else:\n");
            printAstRec(node->data.ifStmt.elseBlock, depth + 2);
        }
        break;

    case AST_WHILE:
        printf("While\n");
        printIndent(depth + 1);
        printf("Condition:\n");
        printAstRec(node->data.whileStmt.condition, depth + 2);
        printIndent(depth + 1);
        printf("Body:\n");
        printAstRec(node->data.whileStmt.bodyBlock, depth + 2);
        break;

    case AST_FOR:
        printf("For(iterator: %s, endInclusive: %s)\n",
               node->data.forStmt.iteratorName,
               node->data.forStmt.endInclusive ? "true" : "false");

        printIndent(depth + 1);
        printf("Start:\n");
        printAstRec(node->data.forStmt.start, depth + 2);

        printIndent(depth + 1);
        printf("End:\n");
        printAstRec(node->data.forStmt.end, depth + 2);

        if (node->data.forStmt.step != NULL) {
            printIndent(depth + 1);
            printf("Step:\n");
            printAstRec(node->data.forStmt.step, depth + 2);
        }

        for (size_t i = 0; i < node->data.forStmt.guardCount; i++) {
            printIndent(depth + 1);
            printf("Guard %zu:\n", i);
            printAstRec(node->data.forStmt.guards[i], depth + 2);
        }

        printIndent(depth + 1);
        printf("Body:\n");
        printAstRec(node->data.forStmt.body, depth + 2);
        break;

    case AST_FUNC_CALL:
        printf("FuncCall(name: %s)\n", node->data.funcCall.funcName);
        for (size_t i = 0; i < node->data.funcCall.argCount; i++) {
            printAstRec(node->data.funcCall.arguments[i], depth + 1);
        }
        break;

    case AST_RETURN:
        printf("Return\n");
        if (node->data.returnStmt.value != NULL) {
            printAstRec(node->data.returnStmt.value, depth + 1);
        }
        break;

    case AST_BINOP:
        printf("BinOp(TokenType: %d)\n", node->data.binop.op);
        printAstRec(node->data.binop.left, depth + 1);
        printAstRec(node->data.binop.right, depth + 1);
        break;

    case AST_UNOP:
        printf("UnOp(TokenType: %d)\n", node->data.unop.op);
        printAstRec(node->data.unop.operand, depth + 1);
        break;

    case AST_LITERAL:
        switch (node->data.literal.litType) {
        case TOKEN_INT_LITERAL:
            printf("Literal(Int, value: %" PRId64 ")\n",
                   node->data.literal.attribute.intValue);
            break;

        case TOKEN_DOUBLE_LITERAL:
            printf("Literal(Double, value: %.17g)\n",
                   node->data.literal.attribute.doubleValue);
            break;

        case TOKEN_STRING_LITERAL:
            printf("Literal(String, value: \"%s\")\n",
                   node->data.literal.attribute.text.data != NULL
                       ? node->data.literal.attribute.text.data
                       : "(NULL)");
            break;

        case TOKEN_BOOL_LITERAL:
            printf("Literal(Bool, value: %s)\n",
                   node->data.literal.attribute.boolValue ? "true" : "false");
            break;

        case TOKEN_LPAREN:
            printf("Literal(Unit, value: ())\n");
            break;

        default:
            printf("Literal(TokenType: %d)\n",
                   (int)node->data.literal.litType);
            break;
        }
        break;

    case AST_IDENTIFIER:
        printf("Identifier(name: %s)\n", node->data.identifier.name);
        break;

    default:
        printf("Unknown AST Node Type\n");
        break;
    }
}

void printAst(astNode *root) {
    printf("AST ============\n");
    printAstRec(root, 0);
    printf("================\n");
}
