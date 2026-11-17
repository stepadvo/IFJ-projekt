/**
 * @file test_token.cpp
 * @brief Tests of token cleanup.
 * @author Ivo Zahradníček, xzahrai00@stud.fit.vutbr.cz
 *
 * Exercises cleanup of owned identifier and string text
 * and verifies that token fields are reset. Covers repeated
 * cleanup calls, empty tokens and NULL pointers.
 *
 * GPT-6.1 Sol assisted with this code.
 * Prompt:
 * Implementuj test_token.cpp pomocí GoogleTest.
 * Zahrň existující token.h a testuj tokenFree().
 * Hlavičky C modulů zahrň přes extern "C".
 *
 * V anonymním namespace vytvoř pomocnou funkci expectEmpty(),
 * která ověří TOKEN_INVALID, nulový attribute.intValue,
 * line, column a indent a nepravdivé příznaky
 * startsLogicalLine a newlineBefore.
 *
 * Vytvoř tři testy:
 * 1. Pro TOKEN_IDENTIFIER i TOKEN_STRING_LITERAL alokuj pomocí
 *    malloc() pět bajtů a zkopíruj do nich "text" včetně
 *    ukončovací nuly. Nastav délku na čtyři a vyplň metadata
 *    nenulovými hodnotami. Zavolej tokenFree() a ověř reset.
 *    Potom uvolnění zopakuj a znovu zkontroluj prázdný token.
 * 2. Pro číselné a Boolean literály, TOKEN_PLUS a TOKEN_EOF
 *    nastav nenulový atribut a pozici. Ověř, že tokenFree()
 *    bezpečně resetuje tokeny, které nevlastní text.
 * 3. Ověř bezpečné volání tokenFree(nullptr) a opakované
 *    uvolnění nulově inicializovaného tokenu.
 *
 * Použij ASSERT_NE pro kontrolu alokace a EXPECT pro kontroly
 * resetovaných polí.
 */

#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>

extern "C" {
#include "token.h"
}

namespace {
void expectEmpty(const Token &token) {
    EXPECT_EQ(token.type, TOKEN_INVALID);
    EXPECT_EQ(token.attribute.intValue, 0);
    EXPECT_EQ(token.line, 0u);
    EXPECT_EQ(token.column, 0u);
    EXPECT_EQ(token.indent, 0u);
    EXPECT_FALSE(token.startsLogicalLine);
    EXPECT_FALSE(token.newlineBefore);
}
} // namespace

TEST(TokenTest, FreesOwnedIdentifierAndStringText) {
    for (TokenType type : {TOKEN_IDENTIFIER, TOKEN_STRING_LITERAL}) {
        Token token{};
        token.type = type;
        token.attribute.text.data = static_cast<char *>(std::malloc(5));
        ASSERT_NE(token.attribute.text.data, nullptr);
        std::memcpy(token.attribute.text.data, "text", 5);
        token.attribute.text.length = 4;
        token.line = 3;
        token.column = 7;
        token.indent = 2;
        token.startsLogicalLine = true;
        token.newlineBefore = true;

        tokenFree(&token);
        expectEmpty(token);
        tokenFree(&token);
        expectEmpty(token);
    }
}

TEST(TokenTest, ResetsTokensWithoutText) {
    for (TokenType type : {TOKEN_INT_LITERAL, TOKEN_DOUBLE_LITERAL,
                           TOKEN_BOOL_LITERAL, TOKEN_PLUS, TOKEN_EOF}) {
        Token token{};
        token.type = type;
        token.attribute.intValue = 123;
        token.line = 4;
        token.column = 8;
        tokenFree(&token);
        expectEmpty(token);
    }
}

TEST(TokenTest, AcceptsNullAndEmptyTokens) {
    tokenFree(nullptr);
    Token token{};
    tokenFree(&token);
    tokenFree(&token);
    expectEmpty(token);
}
