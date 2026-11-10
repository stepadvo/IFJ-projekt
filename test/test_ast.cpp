/**
 * @file test_ast.h
 * @brief tests for the AST
 * @author Jiří Dohnal, xdohnaj00@stud.fit.vutbr.cz
 */

#include <gtest/gtest.h>

extern "C" {
#include "ast.h"
}

// unit tests

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/*
 * end test_ast.h
 */
