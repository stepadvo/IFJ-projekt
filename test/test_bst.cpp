/**
 * @file test_bst.cpp
 * @brief tests for the bst ADT
 * @author Štěpán Dvořák, xdvoras00@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include <gtest/gtest.h>

extern "C" {
#include "bst.h"
#include "compiler.h"
}

/* tests generated using Gemini. Prompt:
 * This is a header file for an ADT of an avl search tree in C. Write unit
 * tests for it using gtest. If there are any operations missing from the
 * header, let me know before making the tests
 */
class BstTest : public ::testing::Test {
  protected:
    bst_t *tree;

    void SetUp() override {
        tree = BSTInit();
        ERRCODE = 0;
    }

    void TearDown() override {
        BSTDestruct(tree);
        EXPECT_EQ(ERRCODE, 0);
    }
};

TEST_F(BstTest, InitState) {
    EXPECT_EQ(BSTSize(tree), 0);
    EXPECT_NE(BSTIsBalanced(tree), 0); // An empty tree is balanced
    EXPECT_EQ(BSTSearch(tree, "nonexistent"), nullptr);
}

TEST_F(BstTest, NullSafety) {
    // BSTDestruct, BSTInsert and BSTClear should handle NULL
    BSTDestruct(nullptr);
    BSTClear(nullptr);
    EXPECT_EQ(BSTInsert(tree, "key", 0, nullptr), 0);
    EXPECT_EQ(BSTSize(nullptr), 0);
    EXPECT_EQ(BSTSearch(nullptr, "key"), nullptr);
}

TEST_F(BstTest, InsertAndSearch) {
    EXPECT_EQ(BSTInsert(tree, "apple", 10, nullptr), 0);
    EXPECT_EQ(BSTInsert(tree, "banana", 20, nullptr), 0);
    EXPECT_EQ(BSTInsert(tree, "cherry", 30, nullptr), 0);

    EXPECT_EQ(BSTSize(tree), 3);

    size_t *val1 = BSTSearch(tree, "apple");
    ASSERT_NE(val1, nullptr);
    EXPECT_EQ(*val1, 10);

    size_t *val2 = BSTSearch(tree, "banana");
    ASSERT_NE(val2, nullptr);
    EXPECT_EQ(*val2, 20);

    size_t *val3 = BSTSearch(tree, "cherry");
    ASSERT_NE(val3, nullptr);
    EXPECT_EQ(*val3, 30);

    EXPECT_EQ(BSTSearch(tree, "date"), nullptr);
}

TEST_F(BstTest, DuplicateKeys) {
    EXPECT_EQ(BSTInsert(tree, "apple", 10, nullptr), 0);
    // Attempt duplicate insertion
    EXPECT_EQ(BSTInsert(tree, "apple", 99, nullptr), -2);

    // Verify size hasn't changed and original value is intact
    EXPECT_EQ(BSTSize(tree), 1);
    size_t *val = BSTSearch(tree, "apple");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 10);
}

TEST_F(BstTest, RemoveNodes) {
    BSTInsert(tree, "dog", 1, nullptr);
    BSTInsert(tree, "cat", 2, nullptr);
    BSTInsert(tree, "bird", 3, nullptr);

    size_t oldData = 0;

    // Remove existing node
    EXPECT_EQ(BSTRemove(tree, "cat", &oldData), 0);
    EXPECT_EQ(oldData, 2);
    EXPECT_EQ(BSTSize(tree), 2);
    EXPECT_EQ(BSTSearch(tree, "cat"), nullptr);

    // Try removing non-existent node
    EXPECT_EQ(BSTRemove(tree, "fish", &oldData), -2);
    EXPECT_EQ(BSTSize(tree), 2);
}

TEST_F(BstTest, AVLBalanceMaintained) {
    // Inserting sequential elements forces AVL rotations
    BSTInsert(tree, "a", 1, nullptr);
    BSTInsert(tree, "b", 2, nullptr);
    BSTInsert(tree, "c", 3, nullptr);
    BSTInsert(tree, "d", 4, nullptr);
    BSTInsert(tree, "e", 5, nullptr);

    EXPECT_EQ(BSTSize(tree), 5);
    EXPECT_NE(BSTIsBalanced(tree), 0);
}

TEST_F(BstTest, ClearTree) {
    BSTInsert(tree, "one", 1, nullptr);
    BSTInsert(tree, "two", 2, nullptr);
    EXPECT_EQ(BSTSize(tree), 2);

    BSTClear(tree);
    EXPECT_EQ(BSTSize(tree), 0);
    EXPECT_EQ(BSTSearch(tree, "one"), nullptr);
    EXPECT_NE(BSTIsBalanced(tree), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/*
 * end test_bst.cpp
 */
