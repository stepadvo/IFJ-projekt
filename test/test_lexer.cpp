/**
 * @file test_lexer.cpp
 * @brief Tests of the IFJ26 lexical analyzer.
 * @author Ivo Zahradníček, xzahrai00@stud.fit.vutbr.cz
 *
 * Verifies token recognition, literal values, source positions,
 * indentation and newline metadata. Covers valid inputs,
 * lexical errors, end-of-input behavior and token ownership.
 *
 * GPT-6.1 Sol assisted with this code.
 * Prompt:
 * Implementuj test_lexer.cpp pomocí GoogleTest.
 * Použij existující compiler.h a lexer.h.
 * Hlavičky C modulů zahrň přes extern "C".
 * Testuj veřejné funkce lexerCreate(), getToken(), lexerDestroy()
 * a tokenFree().
 *
 * Vytvoř fixture LexerTest s dočasným vstupním souborem, ukazatelem
 * na lexer a nulově inicializovaným tokenem. Přidej pomocné funkce
 * pro přípravu vstupu, kontrolu dalšího tokenu, posloupnosti typů
 * a textového atributu včetně délky a ukončovací nuly.
 * Před každým novým vstupem ukliď předchozí prostředky a resetuj
 * ERRCODE. Zkontroluj vytvoření souboru, zápis celého textu,
 * přesun na začátek a vytvoření lexeru. V TearDown() vše uvolni.
 * Selhání přípravy musí zastavit i volající test.
 *
 * Vytvoř deset testů; související vstupy seskup do tabulek:
 * 1. Všechna klíčová slova podle token.h, identifikátory abc_123,
 *    _name, IF, trueValue a název dlouhý 200 znaků. Ověř text
 *    identifikátorů a hodnoty Boolean literálů true a false.
 * 2. Všechny podporované operátory a oddělovače včetně jejich
 *    dvojznakových variant; ověř pořadí vrácených tokenů.
 * 3. Čísla 0, 42, 1_000, INT64_MAX, 0.5, .5, 1.5e2, 2E-2,
 *    1_000.2_5, 1.5D a 1e+2d. Ověř převedené hodnoty,
 *    pro Double použij EXPECT_DOUBLE_EQ. Na vstupu
 *    "42.toString -1" ověř samostatnou tečku, pozici a text
 *    identifikátoru a oddělený token minus před číslem.
 * 4. Prázdný a dlouhý obyčejný řetězec, všechny podporované
 *    escape sekvence a víceřádkový řetězec zachovávající obsah.
 *    Ověř také počáteční odsazení víceřádkového řetězce,
 *    pozici následujícího tokenu a to, že vnitřní LF nenastaví
 *    jeho newlineBefore.
 * 5. Přeskakování řádkových a vnořených blokových komentářů,
 *    prázdných řádků a komentářů mezi tokeny. Ověř výslednou
 *    posloupnost, čísla řádků a newlineBefore.
 * 6. Na vstupu s úvodním prázdným řádkem a pokračováním
 *    v závorkách ověř line, column, indent, startsLogicalLine
 *    a newlineBefore. Uvnitř závorek neočekávej TOKEN_NEWLINE.
 * 7. Chybné vstupy: samotné podtržítko, neznámý znak,
 *    samostatné & a |, neplatný escape, LF v obyčejném řetězci,
 *    neukončené řetězce a komentáře, kód za víceřádkovým
 *    blokovým komentářem na jeho uzavíracím řádku, úvodní
 *    tabulátor, nedostatečně odsazené pokračování, chybné
 *    číselné oddělovače, exponenty, přípony a přetečení.
 *    Čti do chyby nebo EOF s omezeným počtem volání.
 *    Ověř LEXER_ERROR, ERRCODE, TOKEN_INVALID a newlineBefore.
 * 8. Samostatně otestuj zvolenou politiku DKA pro úvodní nuly:
 *    odmítni 00, 0_0, 012, 01.5 a 01e2.
 * 9. Prázdný vstup, vstup obsahující pouze komentáře či bílé
 *    znaky, syntetický NEWLINE před EOF a opakované EOF.
 *    Ověř, že dva různé výstupní tokeny vlastní nezávislý text,
 *    jejich text přežije zničení lexeru a vstup zůstane otevřený.
 * 10. lexerCreate(nullptr) musí vrátit nullptr a nastavit
 *     interní chybu 99. lexerDestroy(nullptr) musí být bezpečné.
 *     Lexikální ani inicializační chyba nesmí přepsat dříve
 *     nastavený nenulový ERRCODE.
 *
 * Pro tabulkové případy používej SCOPED_TRACE.
 */

#include <cstdio>
#include <gtest/gtest.h>
#include <initializer_list>
#include <string>

extern "C" {
#include "compiler.h"
#include "lexer.h"
}

class LexerTest : public ::testing::Test {
  protected:
    FILE *input = nullptr;
    Lexer *lexer = nullptr;
    Token token{};

    void clear() {
        tokenFree(&token);
        lexerDestroy(lexer);
        lexer = nullptr;
        if (input != nullptr) {
            std::fclose(input);
            input = nullptr;
        }
    }

    void open(const std::string &source) {
        clear();
        ERRCODE = 0;
        input = std::tmpfile();
        ASSERT_NE(input, nullptr);
        ASSERT_EQ(std::fwrite(source.data(), 1, source.size(), input),
                  source.size());
        ASSERT_EQ(std::fseek(input, 0, SEEK_SET), 0);
        lexer = lexerCreate(input);
        ASSERT_NE(lexer, nullptr);
    }

    ::testing::AssertionResult next(TokenType expected) {
        LexerStatus status = getToken(lexer, &token);
        if (status != LEXER_OK || token.type != expected || ERRCODE != 0) {
            return ::testing::AssertionFailure()
                   << "expected type " << expected << ", got " << token.type
                   << ", status " << status << ", ERRCODE " << ERRCODE;
        }
        return ::testing::AssertionSuccess();
    }

    void sequence(std::initializer_list<TokenType> types) {
        for (TokenType type : types) {
            ASSERT_TRUE(next(type));
        }
    }

    void text(const std::string &expected) {
        ASSERT_NE(token.attribute.text.data, nullptr);
        EXPECT_EQ(token.attribute.text.length, expected.size());
        EXPECT_EQ(
            std::string(token.attribute.text.data, token.attribute.text.length),
            expected);
        EXPECT_EQ(token.attribute.text.data[token.attribute.text.length], '\0');
    }

    void TearDown() override { clear(); }
};

TEST_F(LexerTest, IdentifiersKeywordsAndBooleans) {
    const struct {
        const char *source;
        TokenType type;
    } keywords[] = {{"def", TOKEN_KW_DEF},
                    {"do", TOKEN_KW_DO},
                    {"Double", TOKEN_KW_DOUBLE},
                    {"else", TOKEN_KW_ELSE},
                    {"if", TOKEN_KW_IF},
                    {"import", TOKEN_KW_IMPORT},
                    {"Int", TOKEN_KW_INT},
                    {"return", TOKEN_KW_RETURN},
                    {"String", TOKEN_KW_STRING},
                    {"then", TOKEN_KW_THEN},
                    {"Unit", TOKEN_KW_UNIT},
                    {"val", TOKEN_KW_VAL},
                    {"var", TOKEN_KW_VAR},
                    {"while", TOKEN_KW_WHILE},
                    {"Boolean", TOKEN_KW_BOOLEAN},
                    {"for", TOKEN_KW_FOR},
                    {"case", TOKEN_KW_CASE},
                    {"class", TOKEN_KW_CLASS},
                    {"extension", TOKEN_KW_EXTENSION},
                    {"List", TOKEN_KW_LIST},
                    {"match", TOKEN_KW_MATCH},
                    {"type", TOKEN_KW_TYPE}};
    for (const auto &item : keywords) {
        SCOPED_TRACE(item.source);
        ASSERT_NO_FATAL_FAILURE(open(item.source));
        ASSERT_NO_FATAL_FAILURE(
            sequence({item.type, TOKEN_NEWLINE, TOKEN_EOF}));
    }
    for (const std::string &name :
         {std::string("abc_123"), std::string("_name"), std::string("IF"),
          std::string("trueValue"), std::string(200, 'a')}) {
        ASSERT_NO_FATAL_FAILURE(open(name));
        ASSERT_TRUE(next(TOKEN_IDENTIFIER));
        text(name);
    }
    ASSERT_NO_FATAL_FAILURE(open("true false"));
    ASSERT_TRUE(next(TOKEN_BOOL_LITERAL));
    EXPECT_TRUE(token.attribute.boolValue);
    ASSERT_TRUE(next(TOKEN_BOOL_LITERAL));
    EXPECT_FALSE(token.attribute.boolValue);
}

TEST_F(LexerTest, OperatorsAndPunctuation) {
    ASSERT_NO_FATAL_FAILURE(
        open("= + - * / == != < <= > >= ! && || <- ( ) : , ; . @"));
    sequence(
        {TOKEN_ASSIGN,     TOKEN_PLUS,    TOKEN_MINUS,         TOKEN_MULTIPLY,
         TOKEN_DIVIDE,     TOKEN_EQUAL,   TOKEN_NOT_EQUAL,     TOKEN_LESS,
         TOKEN_LESS_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_NOT,
         TOKEN_AND,        TOKEN_OR,      TOKEN_LEFT_ARROW,    TOKEN_LPAREN,
         TOKEN_RPAREN,     TOKEN_COLON,   TOKEN_COMMA,         TOKEN_SEMICOLON,
         TOKEN_DOT,        TOKEN_AT,      TOKEN_NEWLINE,       TOKEN_EOF});
}

TEST_F(LexerTest, NumericValuesAndDotLookahead) {
    const struct {
        const char *source;
        int64_t value;
    } integers[] = {{"0", 0},
                    {"42", 42},
                    {"1_000", 1000},
                    {"9223372036854775807", INT64_MAX}};
    for (const auto &item : integers) {
        SCOPED_TRACE(item.source);
        ASSERT_NO_FATAL_FAILURE(open(item.source));
        ASSERT_TRUE(next(TOKEN_INT_LITERAL));
        EXPECT_EQ(token.attribute.intValue, item.value);
        ASSERT_NO_FATAL_FAILURE(sequence({TOKEN_NEWLINE, TOKEN_EOF}));
    }
    const struct {
        const char *source;
        double value;
    } doubles[] = {
        {"0.5", 0.5},           {".5", 0.5},   {"1.5e2", 150.0}, {"2E-2", 0.02},
        {"1_000.2_5", 1000.25}, {"1.5D", 1.5}, {"1e+2d", 100.0}};
    for (const auto &item : doubles) {
        SCOPED_TRACE(item.source);
        ASSERT_NO_FATAL_FAILURE(open(item.source));
        ASSERT_TRUE(next(TOKEN_DOUBLE_LITERAL));
        EXPECT_DOUBLE_EQ(token.attribute.doubleValue, item.value);
        ASSERT_NO_FATAL_FAILURE(sequence({TOKEN_NEWLINE, TOKEN_EOF}));
    }
    ASSERT_NO_FATAL_FAILURE(open("42.toString -1"));
    ASSERT_TRUE(next(TOKEN_INT_LITERAL));
    EXPECT_EQ(token.attribute.intValue, 42);
    ASSERT_NO_FATAL_FAILURE(sequence({TOKEN_DOT, TOKEN_IDENTIFIER}));
    EXPECT_EQ(token.column, 4u);
    text("toString");
    ASSERT_NO_FATAL_FAILURE(sequence({TOKEN_MINUS, TOKEN_INT_LITERAL}));
    EXPECT_EQ(token.attribute.intValue, 1);
}

TEST_F(LexerTest, OrdinaryAndMultilineStrings) {
    const std::string longText(200, 'x');
    const struct {
        std::string source;
        std::string value;
    } cases[] = {{"\"\"", ""},
                 {R"ifj("a\n\r\t\"\\b")ifj", "a\n\r\t\"\\b"},
                 {"\"" + longText + "\"", longText},
                 {"\"\"\"a\n  b\\n\"c\"\"d\"\"\"", "a\n  b\\n\"c\"\"d"}};
    for (const auto &item : cases) {
        SCOPED_TRACE(item.source);
        ASSERT_NO_FATAL_FAILURE(open(item.source));
        ASSERT_TRUE(next(TOKEN_STRING_LITERAL));
        text(item.value);
        ASSERT_NO_FATAL_FAILURE(sequence({TOKEN_NEWLINE, TOKEN_EOF}));
    }
    ASSERT_NO_FATAL_FAILURE(open("  \"\"\"a\nb\"\"\" x"));
    ASSERT_TRUE(next(TOKEN_STRING_LITERAL));
    EXPECT_EQ(token.indent, 2u);
    EXPECT_EQ(token.line, 1u);
    EXPECT_EQ(token.column, 3u);
    ASSERT_TRUE(next(TOKEN_IDENTIFIER));
    EXPECT_EQ(token.line, 2u);
    EXPECT_EQ(token.column, 6u);
    EXPECT_FALSE(token.newlineBefore);
    EXPECT_FALSE(token.startsLogicalLine);
}

TEST_F(LexerTest, CommentsAndBlankLines) {
    ASSERT_NO_FATAL_FAILURE(
        open("\t// ignored\n\n/* outer /* inner */ end */\n"
             "a/* inline */+b// tail\n/* multi\nend */\nc"));
    ASSERT_TRUE(next(TOKEN_IDENTIFIER));
    text("a");
    EXPECT_EQ(token.line, 4u);
    EXPECT_TRUE(token.newlineBefore);
    ASSERT_NO_FATAL_FAILURE(sequence(
        {TOKEN_PLUS, TOKEN_IDENTIFIER, TOKEN_NEWLINE, TOKEN_IDENTIFIER}));
    text("c");
    EXPECT_EQ(token.line, 7u);
    EXPECT_TRUE(token.newlineBefore);
    ASSERT_NO_FATAL_FAILURE(sequence({TOKEN_NEWLINE, TOKEN_EOF}));
}

TEST_F(LexerTest, PositionsAndContinuationIndentation) {
    ASSERT_NO_FATAL_FAILURE(open("\n  f(\n    1,\n    2)\n  x"));
    ASSERT_TRUE(next(TOKEN_IDENTIFIER));
    EXPECT_EQ(token.line, 2u);
    EXPECT_EQ(token.column, 3u);
    EXPECT_EQ(token.indent, 2u);
    EXPECT_TRUE(token.startsLogicalLine);
    EXPECT_TRUE(token.newlineBefore);
    ASSERT_TRUE(next(TOKEN_LPAREN));
    EXPECT_FALSE(token.startsLogicalLine);
    EXPECT_FALSE(token.newlineBefore);
    ASSERT_TRUE(next(TOKEN_INT_LITERAL));
    EXPECT_EQ(token.line, 3u);
    EXPECT_EQ(token.column, 5u);
    EXPECT_EQ(token.indent, 0u);
    EXPECT_FALSE(token.startsLogicalLine);
    EXPECT_TRUE(token.newlineBefore);
    ASSERT_NO_FATAL_FAILURE(sequence(
        {TOKEN_COMMA, TOKEN_INT_LITERAL, TOKEN_RPAREN, TOKEN_NEWLINE}));
    EXPECT_FALSE(token.newlineBefore);
    ASSERT_TRUE(next(TOKEN_IDENTIFIER));
    EXPECT_EQ(token.line, 5u);
    EXPECT_EQ(token.indent, 2u);
    EXPECT_TRUE(token.startsLogicalLine);
    EXPECT_TRUE(token.newlineBefore);
    ASSERT_NO_FATAL_FAILURE(sequence({TOKEN_NEWLINE, TOKEN_EOF}));
}

TEST_F(LexerTest, InvalidInputs) {
    const char *cases[] = {"_",
                           "$",
                           "&",
                           "|",
                           "\"unterminated",
                           "\"bad\\q\"",
                           "\"a\nb\"",
                           "\"\"\"unfinished",
                           "/* unfinished",
                           "/* a\nb */ x",
                           "\tx",
                           "f(\nx)",
                           "  f(\n  x)",
                           "1_",
                           "1__2",
                           "1e",
                           "1e+",
                           "1e_2",
                           "1.2_",
                           "42d",
                           "123abc",
                           "9223372036854775808",
                           "1e9999"};
    for (const char *source : cases) {
        SCOPED_TRACE(source);
        ASSERT_NO_FATAL_FAILURE(open(source));
        LexerStatus status = LEXER_OK;
        for (size_t i = 0; i < std::string(source).size() + 2; ++i) {
            status = getToken(lexer, &token);
            if (status != LEXER_OK || token.type == TOKEN_EOF)
                break;
        }
        EXPECT_EQ(status, LEXER_ERROR);
        EXPECT_EQ(ERRCODE, LEXER_ERROR);
        EXPECT_EQ(token.type, TOKEN_INVALID);
        EXPECT_FALSE(token.newlineBefore);
    }
}

TEST_F(LexerTest, LeadingZerosFollowChosenDkaPolicy) {
    for (const char *source : {"00", "0_0", "012", "01.5", "01e2"}) {
        SCOPED_TRACE(source);
        ASSERT_NO_FATAL_FAILURE(open(source));
        EXPECT_EQ(getToken(lexer, &token), LEXER_ERROR);
        EXPECT_EQ(ERRCODE, LEXER_ERROR);
        EXPECT_EQ(token.type, TOKEN_INVALID);
    }
}

TEST_F(LexerTest, EndOfInputAndIndependentTokenOwnership) {
    for (const char *source : {"", " \t\n// comment", "/* comment */"}) {
        ASSERT_NO_FATAL_FAILURE(open(source));
        ASSERT_NO_FATAL_FAILURE(sequence({TOKEN_EOF, TOKEN_EOF}));
    }
    ASSERT_NO_FATAL_FAILURE(open("first second"));
    ASSERT_TRUE(next(TOKEN_IDENTIFIER));
    Token other{};
    LexerStatus status = getToken(lexer, &other);
    EXPECT_EQ(status, LEXER_OK);
    EXPECT_EQ(other.type, TOKEN_IDENTIFIER);
    lexerDestroy(lexer);
    lexer = nullptr;
    text("first");
    if (other.type == TOKEN_IDENTIFIER) {
        EXPECT_STREQ(other.attribute.text.data, "second");
    }
    tokenFree(&other);
    EXPECT_EQ(std::fseek(input, 0, SEEK_SET), 0);
    ASSERT_NO_FATAL_FAILURE(open("x"));
    ASSERT_NO_FATAL_FAILURE(
        sequence({TOKEN_IDENTIFIER, TOKEN_NEWLINE, TOKEN_EOF, TOKEN_EOF}));
}

TEST_F(LexerTest, InitializationAndFirstErrorPreservation) {
    ERRCODE = 0;
    EXPECT_EQ(lexerCreate(nullptr), nullptr);
    EXPECT_EQ(ERRCODE, LEXER_INTERNAL_ERROR);
    lexerDestroy(nullptr);
    ASSERT_NO_FATAL_FAILURE(open("$"));
    ERRCODE = 2;
    EXPECT_EQ(getToken(lexer, &token), LEXER_ERROR);
    EXPECT_EQ(ERRCODE, 2);
    EXPECT_EQ(lexerCreate(nullptr), nullptr);
    EXPECT_EQ(ERRCODE, 2);
}
