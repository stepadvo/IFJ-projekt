/**
 * @file test_example.h
 * @brief example header
 * @author Name Surname, xlogin@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include <cstring>
#include <gtest/gtest.h>

extern "C" {
#include "generator.h"
#include "header_example.h"
#include "symtable.h"
}

class GenTest : public ::testing::Test {
  protected:
    codeGenState *state;

    void SetUp() override {
        state = codeGenInit();
        if (symTableInit() != 0) {
            return;
        }

        ERRCODE = 0;
    }

    void TearDown() override {
        codeGenDestruct(state);
        symTableDestruct();
        EXPECT_EQ(ERRCODE, 0);
    }
};

TEST_F(GenTest, test_init) {
    EXPECT_EQ(state->firstBuf, nullptr);
    EXPECT_EQ(state->lastBuf, nullptr);
    EXPECT_EQ(state->firstFun, nullptr);
    EXPECT_EQ(state->lastFun, nullptr);

    EXPECT_EQ(strlen(state->mainFunName), 0);
    for (int i = 0; i < FUN_NUM; i++) {
        EXPECT_EQ(state->builtinNeeded[i], false);
    }
}

TEST_F(GenTest, test_createBuf) {
    codeGenBufNode *buffer = codeGenCreateAndInsertBuf(state);
    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(state->firstBuf, buffer);
    EXPECT_EQ(state->lastBuf, buffer);

    EXPECT_NE(buffer->firstSection, nullptr);
    EXPECT_NE(buffer->lastSection, nullptr);
    EXPECT_EQ(buffer->totalSize, 0);

    EXPECT_EQ(buffer->prevBuf, nullptr);
    EXPECT_EQ(buffer->nextBuf, nullptr);
}

// Test fixture for Code Generator tests if setup/teardown is needed later
class CodeGeneratorTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Common setup if needed
    }

    void TearDown() override {
        // Common teardown if needed
    }
};

/**
 * @brief Tests state initialization and destruction lifecycle.
 */
TEST_F(CodeGeneratorTest, InitAndDestruct) {
    codeGenState *state = codeGenInit();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->firstBuf, nullptr);
    EXPECT_EQ(state->lastBuf, nullptr);
    EXPECT_EQ(state->firstFun, nullptr);
    EXPECT_EQ(state->lastFun, nullptr);

    codeGenDestruct(state);
}

/**
 * @brief Tests buffer creation, text insertion, output generation, and cleanup.
 */
TEST_F(CodeGeneratorTest, BufferOperations) {
    codeGenState *state = codeGenInit();
    ASSERT_NE(state, nullptr);

    // Create and insert a new buffer node
    codeGenBufNode *buf = codeGenCreateAndInsertBuf(state);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(state->firstBuf, buf);
    EXPECT_EQ(state->lastBuf, buf);

    // Test inserting text into the buffer
    const char *testTxt = "MOVE LF@x$1 int@42\n";
    int res = codeGenInsertTxt(testTxt, buf);
    EXPECT_EQ(res, 0);
    EXPECT_GT(buf->totalSize, 0);

    // Test outputting the buffer contents into a string buffer
    char outBuf[1024];
    memset(outBuf, 0, sizeof(outBuf));
    int bytesWritten = codeGenOutputBuf(outBuf, buf);
    EXPECT_GT(bytesWritten, 0);
    EXPECT_NE(strstr(outBuf, "MOVE LF@x$1 int@42"), nullptr);

    // Test removing buffer from state and destroying it separately
    codeGenRemoveBuf(state, buf);
    EXPECT_EQ(state->firstBuf, nullptr);
    EXPECT_EQ(state->lastBuf, nullptr);

    codeGenDestruct(state);
}

/**
 * @brief Tests error handling conditions (e.g., passing NULL pointers).
 */
TEST_F(CodeGeneratorTest, ErrorHandlingNulls) {
    codeGenState *state = codeGenInit();
    ASSERT_NE(state, nullptr);

    // According to docs: codeGenBlock returns 1 if state or blockRoot are NULL
    EXPECT_EQ(codeGenBlock(nullptr, nullptr, false), 1);
    EXPECT_EQ(codeGenBlock(state, nullptr, false), 1);
    EXPECT_EQ(codeGenBlock(nullptr, reinterpret_cast<astNode *>(0x1), false),
              1);

    // According to docs: codeGenFunction returns 1 if state or funRoot are NULL
    EXPECT_EQ(codeGenFunction(nullptr, nullptr), 1);
    EXPECT_EQ(codeGenFunction(state, nullptr), 1);
    EXPECT_EQ(codeGenFunction(nullptr, reinterpret_cast<astNode *>(0x1)), 1);

    codeGenDestruct(state);
}

/**
 * @brief Tests utility helpers like variable naming and label generation.
 */
TEST_F(CodeGeneratorTest, UtilityFunctions) {
    codeGenState *state = codeGenInit();
    ASSERT_NE(state, nullptr);
    codeGenBufNode *buf = codeGenCreateAndInsertBuf(state);
    ASSERT_NE(buf, nullptr);
    // Test variable name assembly
    if (symTableInit() == -1) {
        return;
    }
    symTableInsertFun("hello", UNIT, 0, nullptr);
    symTableActivateFun("hello");
    beginBlock();
    symTableInsertVar("counter", INT, false);
    const char *varName = codeGenGetVarName(buf, "counter", NULL);
    ASSERT_NE(varName, nullptr);
    EXPECT_EQ(strncmp(varName, "LF@counter", 10), 0);

    // Test label ID generation

    const char *labelId = codeGenGetLabelId(buf);
    ASSERT_NE(labelId, nullptr);

    codeGenDestruct(state);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/*
 * end test_example.h
 */
