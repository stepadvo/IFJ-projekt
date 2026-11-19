#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" {
#include "ast.h"
#include "compiler.h"
#include "semantics.h"
}

namespace {

// Pomocna funkce pro vytvoreni jednoducheho bloku s jednim returnem
astNode *makeReturnBlock(astNode *expr) {
    astNode *block = createBlockNode();
    astNode *ret = createReturnNode(expr);
    addStatementToBlock(block, ret);
    return block;
}

// Pomocna funkce pro vytvoreni jednoduche funkce
astNode *makeSimpleFunc(const char *name, bool isMain, varType retType,
                        astNode *body) {
    char *nameCopy = strdup(name);
    astNode *fn = createFuncDefNode(nameCopy, isMain, nullptr, 0, body);
    fn->data.funcDef.explicitReturnType = retType;
    return fn;
}

// Spusti semanticAnalyze v child procesu (uzitecne, pokud semError vola
// exit(code))
int runSemanticInChild(astNode *root) {
    pid_t pid = fork();
    if (pid == 0) {
        int rc = semanticAnalyze(root);
        _exit(rc);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

} // namespace

TEST(SemanticsTest, ValidProgram) {
    astNode *prog = createProgramNode();

    // Napriklad hlavni funkce vracejici INT (prizpusob literal/typ podle sveho
    // ast.h)
    Token tok = {};
    tok.type = TOKEN_INT_LITERAL;
    tok.attribute.intValue = 42;

    astNode *lit = createLiteralNode(&tok);
    astNode *body = makeReturnBlock(lit);
    astNode *mainFn = makeSimpleFunc("main", true, INT, body);

    addFunctionToProgram(prog, mainFn);

    int rc = runSemanticInChild(prog);
    EXPECT_EQ(rc, 0);

    freeAst(prog);
}

TEST(SemanticsTest, UnknownIdentifierError) {
    astNode *prog = createProgramNode();

    // Prikazovy blok odkazujici na neexistujici promennou "neexistuje"
    astNode *block = createBlockNode();
    astNode *id = createIdentifierNode(strdup("neexistuje"));
    addStatementToBlock(block, id);

    astNode *mainFn = makeSimpleFunc("main", true, UNIT, block);
    addFunctionToProgram(prog, mainFn);

    int rc = runSemanticInChild(prog);
    EXPECT_EQ(rc, 3); // ocekavany kod nedeklarovaneho identifikatoru

    freeAst(prog);
}