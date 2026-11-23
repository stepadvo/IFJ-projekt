/**
 * @file test_ast.cpp
 * @brief GoogleTest for AST constructors and ownership.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 */

#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>

extern "C" {
#include "ast.h"
#include "token.h"
}

static char *copyName(const char *x) {
    char *p = static_cast<char *>(malloc(strlen(x) + 1));
    if (p)
        strcpy(p, x);
    return p;
}
static astNode *number(int n) {
    Token t = {};
    t.type = TOKEN_INT_LITERAL;
    t.attribute.intValue = n;
    return createLiteralNode(&t);
}
TEST(Ast, EmptyProgram) {
    astNode *n = createProgramNode();
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->type, AST_PROGRAM);
    EXPECT_EQ(n->data.program.count, 0u);
    EXPECT_EQ(n->resultType, UNKNOWN);
    freeAst(n);
}
TEST(Ast, EmptyBlock) {
    astNode *n = createBlockNode();
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->type, AST_BLOCK);
    EXPECT_EQ(n->data.block.count, 0u);
    EXPECT_EQ(n->data.block.bufferNum, -1);
    EXPECT_EQ(n->data.block.parentIfForBody, nullptr);
    freeAst(n);
}
TEST(Ast, BlockAppendOrderAndCapacity) {
    astNode *b = createBlockNode();
    for (int i = 0; i < 32; ++i)
        ASSERT_EQ(addStatementToBlock(b, number(i)), 0);
    ASSERT_EQ(b->data.block.count, 32u);
    EXPECT_GE(b->data.block.capacity, 32u);
    for (int i = 0; i < 32; ++i)
        EXPECT_EQ(b->data.block.statements[i]->data.literal.attribute.intValue,
                  i);
    freeAst(b);
}
TEST(Ast, InvalidBlockAppend) {
    astNode *b = createBlockNode(), *n = number(1);
    EXPECT_EQ(addStatementToBlock(nullptr, n), -1);
    EXPECT_EQ(addStatementToBlock(b, nullptr), -1);
    EXPECT_EQ(addStatementToBlock(n, b), -1);
    freeAst(n);
    freeAst(b);
}
TEST(Ast, ProgramAppendFunction) {
    astNode *p = createProgramNode();
    astNode *f =
        createFuncDefNode(copyName("f"), true, nullptr, 0, createBlockNode());
    ASSERT_EQ(addFunctionToProgram(p, f), 0);
    ASSERT_EQ(p->data.program.count, 1u);
    EXPECT_STREQ(p->data.program.functions[0]->data.funcDef.name, "f");
    freeAst(p);
}
TEST(Ast, BinaryExpressionChildren) {
    astNode *n = createBinopNode(MINUS, number(10), number(3));
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->type, AST_BINOP);
    EXPECT_EQ(n->data.binop.left->data.literal.attribute.intValue, 10);
    EXPECT_EQ(n->data.binop.right->data.literal.attribute.intValue, 3);
    freeAst(n);
}
TEST(Ast, UnaryExpression) {
    astNode *n = createUnopNode(UNARY_MINUS, number(3));
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->type, AST_UNOP);
    EXPECT_EQ(n->data.unop.op, UNARY_MINUS);
    freeAst(n);
}
TEST(Ast, UnitLiteral) {
    astNode *n = createUnitNode();
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->type, AST_LITERAL);
    EXPECT_EQ(n->resultType, UNIT);
    freeAst(n);
}
TEST(Ast, StringTokenCopied) {
    char text[] = "hello";
    Token t = {};
    t.type = TOKEN_STRING_LITERAL;
    t.attribute.text.data = text;
    t.attribute.text.length = 5;
    astNode *n = createLiteralNode(&t);
    ASSERT_NE(n, nullptr);
    text[0] = 'X';
    EXPECT_STREQ(n->data.literal.attribute.text.data, "hello");
    freeAst(n);
}
TEST(Ast, IfWithoutElse) {
    astNode *n = createIfNode(number(1), createBlockNode(), nullptr);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->data.ifStmt.elseBlock, nullptr);
    EXPECT_EQ(n->data.ifStmt.thenBlock->data.block.parentIfForBody, nullptr);
    freeAst(n);
}
TEST(Ast, ForWithGuards) {
    astNode *n = createForNode(copyName("i"), number(1), number(9), true,
                               number(2), createBlockNode());
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->type, AST_FOR);
    EXPECT_TRUE(n->data.forStmt.endInclusive);
    EXPECT_EQ(addGuardToFor(n, number(1)), 0);
    EXPECT_EQ(n->data.forStmt.guardCount, 1u);
    EXPECT_EQ(n->data.forStmt.body->data.block.parentIfForBody, n);
    freeAst(n);
}
TEST(Ast, FunctionCallArguments) {
    astNode *n = createFuncCallNode(copyName("f"));
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(addArgumentToCall(n, number(1)), 0);
    EXPECT_EQ(addArgumentToCall(n, number(2)), 0);
    EXPECT_EQ(n->data.funcCall.argCount, 2u);
    freeAst(n);
}
TEST(Ast, NullFree) { freeAst(nullptr); }
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/*
 * end test_ast.cpp
 */
