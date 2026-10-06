/**
 * @file test_example.h
 * @brief example header
 * @author Name Surname, xlogin@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include <gtest/gtest.h>

extern "C" {
    #include "header_example.h"
}

// unit tests

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/*
 * end test_example.h
 */
