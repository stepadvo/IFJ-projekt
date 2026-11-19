/**
 * @file test_expression.cpp
 * @brief GoogleTest for IFJ26 precedence and expression stack.
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 */

#include <gtest/gtest.h>

extern "C" {
#include "ast.h"
#include "expression.h"
}

TEST(Expression, PlusUnaryBinary) {
    EXPECT_EQ(exprSymbolFromToken(TOKEN_PLUS, true), EXPR_UNARY);
    EXPECT_EQ(exprSymbolFromToken(TOKEN_PLUS, false), EXPR_ADD);
    EXPECT_EQ(exprSymbolFromToken(TOKEN_MINUS, true), EXPR_UNARY);
    EXPECT_EQ(exprSymbolFromToken(TOKEN_MINUS, false), EXPR_ADD);
}
TEST(Expression, MultiplicationBeforeAddition) {
    EXPECT_EQ(exprPrecedence(EXPR_ADD, EXPR_MULTIPLY), PREC_SHIFT);
    EXPECT_EQ(exprPrecedence(EXPR_MULTIPLY, EXPR_ADD), PREC_REDUCE);
}
TEST(Expression, LeftAssociativity) {
    EXPECT_EQ(exprPrecedence(EXPR_MULTIPLY, EXPR_MULTIPLY), PREC_REDUCE);
    EXPECT_EQ(exprPrecedence(EXPR_ADD, EXPR_ADD), PREC_REDUCE);
}
TEST(Expression, RightUnary) {
    EXPECT_EQ(exprPrecedence(EXPR_UNARY, EXPR_UNARY), PREC_SHIFT);
    EXPECT_EQ(exprPrecedence(EXPR_MULTIPLY, EXPR_UNARY), PREC_SHIFT);
    EXPECT_EQ(exprPrecedence(EXPR_UNARY, EXPR_MULTIPLY), PREC_REDUCE);
}
TEST(Expression, LogicalPriority) {
    EXPECT_EQ(exprPrecedence(EXPR_OR, EXPR_AND), PREC_SHIFT);
    EXPECT_EQ(exprPrecedence(EXPR_AND, EXPR_OR), PREC_REDUCE);
    EXPECT_EQ(exprPrecedence(EXPR_AND, EXPR_COMPARISON), PREC_SHIFT);
}
TEST(Expression, ChainedComparisonsInvalid) {
    EXPECT_EQ(exprPrecedence(EXPR_EQUALITY, EXPR_EQUALITY), PREC_ERROR);
    EXPECT_EQ(exprPrecedence(EXPR_COMPARISON, EXPR_COMPARISON), PREC_ERROR);
}
TEST(Expression, ParenthesesMatch) {
    EXPECT_EQ(exprPrecedence(EXPR_LPAREN, EXPR_RPAREN), PREC_MATCH);
}
TEST(Expression, ValueReduces) {
    EXPECT_EQ(exprPrecedence(EXPR_VALUE, EXPR_END), PREC_REDUCE);
    EXPECT_EQ(exprPrecedence(EXPR_VALUE, EXPR_RPAREN), PREC_REDUCE);
}
TEST(Expression, InitialAndFinish) {
    EXPECT_EQ(exprPrecedence(EXPR_END, EXPR_VALUE), PREC_SHIFT);
    EXPECT_EQ(exprPrecedence(EXPR_END, EXPR_END), PREC_ACCEPT);
}
TEST(Expression, InvalidInputRejected) {
    EXPECT_EQ(exprPrecedence(EXPR_INVALID, EXPR_VALUE), PREC_ERROR);
    EXPECT_EQ(exprPrecedence(EXPR_END, EXPR_INVALID), PREC_ERROR);
}
TEST(Expression, StackPushPop) {
    ExprStack stack;
    exprStackInit(&stack);
    EXPECT_EQ(stack.count, 0u);
    exprStackPush(&stack, {EXPR_END, TOKEN_EOF, nullptr});
    exprStackPush(&stack, {EXPR_VALUE, TOKEN_INT_LITERAL, nullptr});
    ASSERT_EQ(stack.count, 2u);
    ExprStackItem item = {};
    ASSERT_TRUE(exprStackPop(&stack, &item));
    EXPECT_EQ(item.symbol, EXPR_VALUE);
    ASSERT_TRUE(exprStackPop(&stack, &item));
    EXPECT_FALSE(exprStackPop(&stack, &item));
    exprStackDestroy(&stack);
}
TEST(Expression, StackTopTerminalSkipsNonterminal) {
    ExprStack stack;
    exprStackInit(&stack);
    exprStackPush(&stack, {EXPR_END, TOKEN_EOF, nullptr});
    exprStackPush(&stack, {EXPR_ADD, TOKEN_PLUS, nullptr});
    exprStackPush(&stack, {EXPR_NONTERMINAL, TOKEN_INVALID, nullptr});
    ASSERT_NE(exprStackTopTerminal(&stack), nullptr);
    EXPECT_EQ(exprStackTopTerminal(&stack)->symbol, EXPR_ADD);
    exprStackDestroy(&stack);
}
TEST(Expression, StackGrowth) {
    ExprStack stack;
    exprStackInit(&stack);
    for (int i = 0; i < 100; ++i)
        exprStackPush(&stack, {EXPR_END, TOKEN_EOF, nullptr});
    EXPECT_EQ(stack.count, 100u);
    EXPECT_GE(stack.capacity, 100u);
    exprStackDestroy(&stack);
}
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/*
 * end test_expression.cpp
 */
