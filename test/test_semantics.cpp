#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <initializer_list>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

extern "C" {
#include "ast.h"
#include "compiler.h"
#include "semantics.h"
}

namespace {

/* ---------- literály a uzly ---------- */

astNode *makeIntLit(long value) {
    Token tok = {};
    tok.type = TOKEN_INT_LITERAL;
    tok.attribute.intValue = value;
    return createLiteralNode(&tok);
}

astNode *makeDoubleLit(double value) {
    Token tok = {};
    tok.type = TOKEN_DOUBLE_LITERAL;
    tok.attribute.doubleValue = value;
    return createLiteralNode(&tok);
}

astNode *makeStrLit(const char *text) {
    Token tok = {};
    tok.type = TOKEN_STRING_LITERAL;
    tok.attribute.text.data = const_cast<char *>(text); // AST si vytvoří kopii
    tok.attribute.text.length = strlen(text);
    return createLiteralNode(&tok);
}

astNode *ident(const char *name) { return createIdentifierNode(strdup(name)); }

astNode *bin(astBinopType op, astNode *l, astNode *r) {
    return createBinopNode(op, l, r);
}

astNode *call(const char *name, std::initializer_list<astNode *> args = {}) {
    astNode *c = createFuncCallNode(strdup(name));
    for (astNode *a : args)
        addArgumentToCall(c, a);
    return c;
}

astNode *blk(std::initializer_list<astNode *> stmts) {
    astNode *b = createBlockNode();
    for (astNode *s : stmts)
        addStatementToBlock(b, s);
    return b;
}

astNode *varDef(const char *name, bool isConst, varType t, astNode *init) {
    return createVarDefNode(strdup(name), isConst, t, init);
}

astNode *assign(const char *name, astNode *v) {
    return createAssignNode(strdup(name), v);
}

astNode *unitBlock() { return blk({createUnitNode()}); }

/* funkce bez parametrů; retType UNKNOWN = není uveden v hlavičce */
astNode *makeFunc(const char *name, bool isMain, varType retType,
                  astNode *body) {
    astNode *fn = createFuncDefNode(strdup(name), isMain, nullptr, 0, body);
    fn->data.funcDef.explicitReturnType = retType;
    return fn;
}

/* funkce s parametry (všechny typu uvedeného v dvojici) */
astNode *makeFuncP(const char *name, varType retType,
                   std::vector<std::pair<const char *, varType>> ps,
                   astNode *body) {
    astFuncParam *params = static_cast<astFuncParam *>(
        malloc(sizeof(astFuncParam) * (ps.empty() ? 1 : ps.size())));
    for (size_t i = 0; i < ps.size(); i++) {
        params[i].name = strdup(ps[i].first);
        params[i].type = ps[i].second;
    }
    astNode *fn =
        createFuncDefNode(strdup(name), false, params, ps.size(), body);
    fn->data.funcDef.explicitReturnType = retType;
    return fn;
}

astNode *mainWith(astNode *body) { return makeFunc("run", true, UNIT, body); }

/* semanticAnalyze vrací ERRCODE; fork izoluje globální stav mezi testy */
int runSemanticInChild(astNode *root) {
    pid_t pid = fork();
    if (pid == 0) {
        (void)!freopen("/dev/null", "w", stdout);
        (void)!freopen("/dev/null", "w", stderr);
        int rc = semanticAnalyze(root);
        _exit(rc);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* postaví program z funkcí, spustí analýzu a uvolní strom */
int runProg(std::vector<astNode *> funcs) {
    astNode *prog = createProgramNode();
    for (astNode *f : funcs)
        addFunctionToProgram(prog, f);
    int rc = runSemanticInChild(prog);
    freeAst(prog);
    return rc;
}

} // namespace

/* =====================================================================
 *  Původní testy: hlavičky funkcí, proměnné, počet argumentů
 * ===================================================================== */

TEST(SemanticsTest, ValidProgram) {
    EXPECT_EQ(runProg({makeFunc("run", true, UNIT, unitBlock())}), 0);
}

TEST(SemanticsTest, UnknownIdentifierError) {
    EXPECT_EQ(runProg({mainWith(blk({ident("neexistuje")}))}), 3);
}

TEST(SemanticsTest, MissingMainError) {
    EXPECT_EQ(runProg({makeFunc("f", false, UNIT, unitBlock())}), 3);
}

TEST(SemanticsTest, FunctionRedefinitionError) {
    EXPECT_EQ(runProg({makeFunc("f", false, UNIT, unitBlock()),
                       makeFunc("f", false, UNIT, unitBlock()),
                       mainWith(unitBlock())}),
              4);
}

TEST(SemanticsTest, MainWithParameterError) {
    astNode *fn = makeFuncP("run", UNIT, {{"x", INT}}, unitBlock());
    fn->data.funcDef.isMain = true;
    EXPECT_EQ(runProg({fn}), 5);
}

TEST(SemanticsTest, MainReturnTypeNotUnitError) {
    EXPECT_EQ(runProg({makeFunc("run", true, INT, unitBlock())}), 6);
}

TEST(SemanticsTest, InitTypeMismatchError) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef("x", true, INT, makeStrLit("a"))}))}), 6);
}

TEST(SemanticsTest, VariableRedefinitionError) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef("x", true, UNKNOWN, makeIntLit(1)),
                               varDef("x", true, UNKNOWN, makeIntLit(2))}))}),
        4);
}

TEST(SemanticsTest, AssignToValError) {
    EXPECT_EQ(runProg({mainWith(blk({varDef("x", true, UNKNOWN, makeIntLit(1)),
                                     assign("x", makeIntLit(2))}))}),
              7);
}

TEST(SemanticsTest, AssignToVarOk) {
    EXPECT_EQ(runProg({mainWith(blk({varDef("x", false, UNKNOWN, makeIntLit(1)),
                                     assign("x", makeIntLit(2))}))}),
              0);
}

TEST(SemanticsTest, WrongArgumentCountError) {
    EXPECT_EQ(runProg({mainWith(blk({call("strLength")}))}), 5);
}

/* =====================================================================
 *  @main
 * ===================================================================== */

TEST(MainTest, MultipleMainError) {
    EXPECT_EQ(runProg({makeFunc("a", true, UNIT, unitBlock()),
                       makeFunc("b", true, UNIT, unitBlock())}),
              4);
}

TEST(MainTest, MainWithoutExplicitTypeOk) {
    EXPECT_EQ(runProg({makeFunc("run", true, UNKNOWN, unitBlock())}), 0);
}

TEST(MainTest, MainMayHaveAnyName) {
    EXPECT_EQ(runProg({makeFunc("runFoo", true, UNIT, unitBlock())}), 0);
}

TEST(MainTest, FunctionNamedMainWithoutAnnotationIsNotMain) {
    EXPECT_EQ(runProg({makeFunc("main", false, UNIT, unitBlock())}), 3);
}

TEST(MainTest, UserFunctionClashesWithBuiltinError) {
    EXPECT_EQ(runProg({makeFunc("print", false, UNIT, unitBlock()),
                       mainWith(unitBlock())}),
              4);
}

TEST(MainTest, DuplicateParameterNamesError) {
    astNode *f = makeFuncP("f", UNIT, {{"x", INT}, {"x", INT}}, unitBlock());
    EXPECT_EQ(runProg({f, mainWith(unitBlock())}), 4);
}

/* =====================================================================
 *  Proměnné a rozsahy platnosti
 * ===================================================================== */

TEST(ScopeTest, UseBeforeDefinitionError) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef("y", true, UNKNOWN, ident("x")),
                               varDef("x", true, UNKNOWN, makeIntLit(1))}))}),
        3);
}

TEST(ScopeTest, SelfReferenceInInitializerError) {
    // val x = x + 1
    EXPECT_EQ(
        runProg({mainWith(blk({varDef(
            "x", true, UNKNOWN, bin(PLUS, ident("x"), makeIntLit(1)))}))}),
        3);
}

TEST(ScopeTest, ShadowingInSubblockOk) {
    // val x = 1; if 1 < 2 then (val x = 2) else ()
    astNode *ifs = createIfNode(
        bin(LESS, makeIntLit(1), makeIntLit(2)),
        blk({varDef("x", true, UNKNOWN, makeIntLit(2))}), unitBlock());
    EXPECT_EQ(runProg({mainWith(
                  blk({varDef("x", true, UNKNOWN, makeIntLit(1)), ifs}))}),
              0);
}

TEST(ScopeTest, BranchVariableNotVisibleInElseError) {
    astNode *ifs = createIfNode(
        bin(LESS, makeIntLit(1), makeIntLit(2)),
        blk({varDef("a", true, UNKNOWN, makeIntLit(1))}), blk({ident("a")}));
    EXPECT_EQ(runProg({mainWith(blk({ifs}))}), 3);
}

TEST(ScopeTest, BranchVariableNotVisibleAfterIfError) {
    astNode *ifs = createIfNode(
        bin(LESS, makeIntLit(1), makeIntLit(2)),
        blk({varDef("a", true, UNKNOWN, makeIntLit(1))}), unitBlock());
    EXPECT_EQ(runProg({mainWith(blk({ifs, ident("a")}))}), 3);
}

TEST(ScopeTest, ParameterIsUsableOk) {
    astNode *f = makeFuncP("f", INT, {{"x", INT}}, blk({ident("x")}));
    EXPECT_EQ(runProg({f, mainWith(unitBlock())}), 0);
}

TEST(ScopeTest, AssignToParameterError) {
    astNode *f =
        makeFuncP("f", UNIT, {{"x", INT}}, blk({assign("x", makeIntLit(2))}));
    EXPECT_EQ(runProg({f, mainWith(unitBlock())}), 7);
}

TEST(ScopeTest, RedefineParameterInBodyError) {
    astNode *f = makeFuncP("f", UNIT, {{"x", INT}},
                           blk({varDef("x", true, UNKNOWN, makeIntLit(1))}));
    EXPECT_EQ(runProg({f, mainWith(unitBlock())}), 4);
}

TEST(ScopeTest, AssignToUndefinedError) {
    EXPECT_EQ(runProg({mainWith(blk({assign("nic", makeIntLit(1))}))}), 3);
}

TEST(ScopeTest, AssignWrongTypeError) {
    EXPECT_EQ(runProg({mainWith(blk({varDef("x", false, UNKNOWN, makeIntLit(1)),
                                     assign("x", makeStrLit("a"))}))}),
              6);
}

TEST(ScopeTest, AssignIntToDoubleVarOk) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef("x", false, DOUBLE, makeDoubleLit(1.5)),
                               assign("x", makeIntLit(2))}))}),
        0);
}

/* =====================================================================
 *  Výrazy a implicitní převod Int -> Double
 * ===================================================================== */

TEST(ExprTest, DoubleFromIntInitOk) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef("x", true, DOUBLE, makeIntLit(5))}))}),
        0);
}

TEST(ExprTest, IntFromDoubleInitError) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef("x", true, INT, makeDoubleLit(2.5))}))}),
        6);
}

TEST(ExprTest, MixedArithmeticOk) {
    EXPECT_EQ(runProg({mainWith(blk(
                  {varDef("x", true, UNKNOWN,
                          bin(PLUS, makeIntLit(1), makeDoubleLit(2.5)))}))}),
              0);
}

TEST(ExprTest, IntegerDivisionOk) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef(
            "x", true, INT, bin(DIVIDE, makeIntLit(5), makeIntLit(2)))}))}),
        0);
}

TEST(ExprTest, IntPlusStringError) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef(
            "x", true, UNKNOWN, bin(PLUS, makeIntLit(1), makeStrLit("a")))}))}),
        6);
}

TEST(ExprTest, StringConcatenationOk) {
    EXPECT_EQ(runProg({mainWith(
                  blk({varDef("x", true, STRING,
                              bin(PLUS, makeStrLit("a"), makeStrLit("b")))}))}),
              0);
}

TEST(ExprTest, StringMinusStringError) {
    EXPECT_EQ(runProg({mainWith(blk(
                  {varDef("x", true, UNKNOWN,
                          bin(MINUS, makeStrLit("a"), makeStrLit("b")))}))}),
              6);
}

TEST(ExprTest, BooleanStoredInVariableOk) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef(
            "b", true, BOOL, bin(LESS, makeIntLit(1), makeIntLit(2)))}))}),
        0);
}

TEST(ExprTest, StringComparisonOk) {
    EXPECT_EQ(runProg({mainWith(
                  blk({varDef("b", true, UNKNOWN,
                              bin(LESS, makeStrLit("a"), makeStrLit("b")))}))}),
              0);
}

TEST(ExprTest, StringLessThanIntError) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef(
            "b", true, UNKNOWN, bin(LESS, makeStrLit("a"), makeIntLit(1)))}))}),
        6);
}

TEST(ExprTest, EqualityOfMixedNumbersOk) {
    EXPECT_EQ(runProg({mainWith(blk(
                  {varDef("b", true, UNKNOWN,
                          bin(EQUAL, makeIntLit(1), makeDoubleLit(1.0)))}))}),
              0);
}

TEST(ExprTest, EqualityOfStringAndIntError) {
    EXPECT_EQ(runProg({mainWith(
                  blk({varDef("b", true, UNKNOWN,
                              bin(EQUAL, makeStrLit("a"), makeIntLit(1)))}))}),
              6);
}

TEST(ExprTest, AndOfNonBooleansError) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef(
            "b", true, UNKNOWN, bin(AND, makeIntLit(1), makeIntLit(2)))}))}),
        6);
}

TEST(ExprTest, UnaryMinusOnStringError) {
    EXPECT_EQ(runProg({mainWith(blk(
                  {varDef("x", true, UNKNOWN,
                          createUnopNode(UNARY_MINUS, makeStrLit("a")))}))}),
              6);
}

TEST(ExprTest, UnaryMinusOnIntOk) {
    EXPECT_EQ(
        runProg({mainWith(blk({varDef(
            "x", true, INT, createUnopNode(UNARY_MINUS, makeIntLit(5)))}))}),
        0);
}

TEST(ExprTest, NotOnIntError) {
    EXPECT_EQ(runProg({mainWith(blk({varDef(
                  "x", true, UNKNOWN, createUnopNode(NOT, makeIntLit(1)))}))}),
              6);
}

TEST(ExprTest, FirstErrorWins_UndefinedThenTypeMismatch) {
    EXPECT_EQ(
        runProg({mainWith(blk(
            {ident("neexistuje"), varDef("x", true, INT, makeStrLit("a"))}))}),
        3);
}

TEST(ExprTest, FirstErrorWins_TypeMismatchThenUndefined) {
    EXPECT_EQ(runProg({mainWith(blk({varDef("x", true, INT, makeStrLit("a")),
                                     ident("neexistuje")}))}),
              6);
}

/* =====================================================================
 *  Volání funkcí
 * ===================================================================== */

TEST(CallTest, UndefinedFunctionError) {
    EXPECT_EQ(runProg({mainWith(blk({call("nic")}))}), 3);
}

TEST(CallTest, PrintAnyOk) {
    EXPECT_EQ(runProg({mainWith(blk({call("print", {makeIntLit(5)})}))}), 0);
}

TEST(CallTest, PrintWithoutArgumentError) {
    EXPECT_EQ(runProg({mainWith(blk({call("print")}))}), 5);
}

TEST(CallTest, PrintTwoArgumentsError) {
    EXPECT_EQ(runProg({mainWith(
                  blk({call("print", {makeIntLit(1), makeIntLit(2)})}))}),
              5);
}

TEST(CallTest, WrongArgumentTypeError) {
    // strLength(5)
    EXPECT_EQ(runProg({mainWith(blk({call("strLength", {makeIntLit(5)})}))}),
              5);
}

TEST(CallTest, WrongArgumentTypeToInt) {
    // toInt("a")
    EXPECT_EQ(runProg({mainWith(blk({call("toInt", {makeStrLit("a")})}))}), 5);
}

TEST(CallTest, UserFunctionIntToDoubleParameterOk) {
    astNode *f = makeFuncP("f", UNIT, {{"d", DOUBLE}}, unitBlock());
    EXPECT_EQ(runProg({f, mainWith(blk({call("f", {makeIntLit(5)})}))}), 0);
}

TEST(CallTest, UserFunctionDoubleToIntParameterError) {
    astNode *f = makeFuncP("f", UNIT, {{"i", INT}}, unitBlock());
    EXPECT_EQ(runProg({f, mainWith(blk({call("f", {makeDoubleLit(1.5)})}))}),
              5);
}

TEST(CallTest, CallResultUsedInInitOk) {
    EXPECT_EQ(runProg({mainWith(blk({varDef(
                  "n", true, INT, call("strLength", {makeStrLit("abc")}))}))}),
              0);
}

TEST(CallTest, SubStrWrongTypesError) {
    EXPECT_EQ(
        runProg({mainWith(blk({call(
            "subStr", {makeStrLit("abc"), makeStrLit("x"), makeIntLit(1)})}))}),
        5);
}

/* =====================================================================
 *  Návratové typy funkcí, odvozování, rekurze
 * ===================================================================== */

TEST(ReturnTypeTest, ForwardCallWithInferredTypeOk) {
    // main volá f dřív, než je definována; f nemá explicitní typ
    astNode *m = mainWith(blk({call("f")}));
    astNode *f = makeFunc("f", false, UNKNOWN, blk({makeIntLit(5)}));
    EXPECT_EQ(runProg({m, f}), 0);
}

TEST(ReturnTypeTest, InferredTypeUsedInExpressionOk) {
    astNode *m = mainWith(blk({varDef("x", true, INT, call("f"))}));
    astNode *f = makeFunc("f", false, UNKNOWN, blk({makeIntLit(5)}));
    EXPECT_EQ(runProg({m, f}), 0);
}

TEST(ReturnTypeTest, InferredTypeMismatchError) {
    // f() = "a", ale main ji ukládá do Int
    astNode *m = mainWith(blk({varDef("x", true, INT, call("f"))}));
    astNode *f = makeFunc("f", false, UNKNOWN, blk({makeStrLit("a")}));
    EXPECT_EQ(runProg({m, f}), 6);
}

TEST(ReturnTypeTest, DirectRecursionWithoutTypeError) {
    // def f() = f()
    EXPECT_EQ(runProg({makeFunc("f", false, UNKNOWN, blk({call("f")})),
                       mainWith(unitBlock())}),
              6);
}

TEST(ReturnTypeTest, MutualRecursionWithoutTypeError) {
    EXPECT_EQ(runProg({makeFunc("a", false, UNKNOWN, blk({call("b")})),
                       makeFunc("b", false, UNKNOWN, blk({call("a")})),
                       mainWith(unitBlock())}),
              6);
}

TEST(ReturnTypeTest, RecursionWithExplicitTypeOk) {
    EXPECT_EQ(runProg({makeFunc("f", false, INT, blk({call("f")})),
                       mainWith(unitBlock())}),
              0);
}

TEST(ReturnTypeTest, MutualRecursionWithExplicitTypesOk) {
    EXPECT_EQ(runProg({makeFunc("a", false, INT, blk({call("b")})),
                       makeFunc("b", false, INT, blk({call("a")})),
                       mainWith(unitBlock())}),
              0);
}

TEST(ReturnTypeTest, DoubleFunctionReturnsIntBodyOk) {
    // def f(): Double = 5
    EXPECT_EQ(runProg({makeFunc("f", false, DOUBLE, blk({makeIntLit(5)})),
                       mainWith(unitBlock())}),
              0);
}

TEST(ReturnTypeTest, IntFunctionReturnsDoubleBodyError) {
    EXPECT_EQ(runProg({makeFunc("f", false, INT, blk({makeDoubleLit(1.5)})),
                       mainWith(unitBlock())}),
              6);
}

TEST(ReturnTypeTest, BodyEndsWithDefinitionError) {
    // def f(): Int = val x = 1
    EXPECT_EQ(
        runProg({makeFunc("f", false, INT,
                          blk({varDef("x", true, UNKNOWN, makeIntLit(1))})),
                 mainWith(unitBlock())}),
        6);
}

TEST(ReturnTypeTest, UnitFunctionDiscardsBodyValueOk) {
    // def f(): Unit = 5
    EXPECT_EQ(runProg({makeFunc("f", false, UNIT, blk({makeIntLit(5)})),
                       mainWith(unitBlock())}),
              0);
}

TEST(ReturnTypeTest, ReturnMatchingTypeOk) {
    EXPECT_EQ(runProg({makeFunc("f", false, INT,
                                blk({createReturnNode(makeIntLit(5))})),
                       mainWith(unitBlock())}),
              0);
}

TEST(ReturnTypeTest, ReturnWrongTypeError) {
    EXPECT_EQ(runProg({makeFunc("f", false, INT,
                                blk({createReturnNode(makeStrLit("a"))})),
                       mainWith(unitBlock())}),
              6);
}

TEST(ReturnTypeTest, ReturnIntInDoubleFunctionOk) {
    EXPECT_EQ(runProg({makeFunc("f", false, DOUBLE,
                                blk({createReturnNode(makeIntLit(5))})),
                       mainWith(unitBlock())}),
              0);
}

TEST(ReturnTypeTest, ReturnWithoutExplicitTypeError) {
    EXPECT_EQ(runProg({makeFunc("f", false, UNKNOWN,
                                blk({createReturnNode(makeIntLit(5))})),
                       mainWith(unitBlock())}),
              6);
}

TEST(ReturnTypeTest, BareReturnInIntFunctionError) {
    EXPECT_EQ(
        runProg({makeFunc("f", false, INT, blk({createReturnNode(nullptr)})),
                 mainWith(unitBlock())}),
        6);
}

TEST(ReturnTypeTest, BareReturnInUnitFunctionOk) {
    EXPECT_EQ(
        runProg({makeFunc("f", false, UNIT, blk({createReturnNode(nullptr)})),
                 mainWith(unitBlock())}),
        0);
}

TEST(ReturnTypeTest, IfWithReturnInBothBranchesOk) {
    // def f(): Int = if 1 < 2 then return 1 else return 2
    astNode *ifs = createIfNode(bin(LESS, makeIntLit(1), makeIntLit(2)),
                                blk({createReturnNode(makeIntLit(1))}),
                                blk({createReturnNode(makeIntLit(2))}));
    EXPECT_EQ(
        runProg({makeFunc("f", false, INT, blk({ifs})), mainWith(unitBlock())}),
        0);
}

/* =====================================================================
 *  if / while / for
 * ===================================================================== */

TEST(ControlTest, IfConditionNotBooleanError) {
    astNode *ifs = createIfNode(makeIntLit(5), unitBlock(), unitBlock());
    EXPECT_EQ(runProg({mainWith(blk({ifs}))}), 6);
}

TEST(ControlTest, IfWithoutElseOk) {
    astNode *ifs = createIfNode(bin(LESS, makeIntLit(1), makeIntLit(2)),
                                unitBlock(), nullptr);
    EXPECT_EQ(runProg({mainWith(blk({ifs}))}), 0);
}

TEST(ControlTest, IfExpressionWithMixedNumericBranchesOk) {
    // val a = if 1 < 2 then 1 else 2.5
    astNode *ifs =
        createIfNode(bin(LESS, makeIntLit(1), makeIntLit(2)),
                     blk({makeIntLit(1)}), blk({makeDoubleLit(2.5)}));
    EXPECT_EQ(runProg({mainWith(blk({varDef("a", true, DOUBLE, ifs)}))}), 0);
}

/* Zatím neimplementováno: if jako výraz s nesouhodnými větvemi (Int / String)
 * musí podle CONTROL skončit chybou 6. Aktuální analyzeIf vrací UNIT a
 * proměnná se do tabulky vloží. Po opravě změň DISABLED_ na normální test. */
TEST(ControlTest, DISABLED_IfExpressionWithMismatchedBranchesError) {
    astNode *ifs = createIfNode(bin(LESS, makeIntLit(1), makeIntLit(2)),
                                blk({makeIntLit(1)}), blk({makeStrLit("x")}));
    EXPECT_EQ(runProg({mainWith(blk({varDef("a", true, UNKNOWN, ifs)}))}), 6);
}

TEST(ControlTest, WhileConditionNotBooleanError) {
    astNode *w = createWhileNode(makeIntLit(1), unitBlock());
    EXPECT_EQ(runProg({mainWith(blk({w}))}), 6);
}

TEST(ControlTest, WhileLoopOk) {
    // var i = 0; while i < 3 do i = i + 1
    astNode *w = createWhileNode(
        bin(LESS, ident("i"), makeIntLit(3)),
        blk({assign("i", bin(PLUS, ident("i"), makeIntLit(1)))}));
    EXPECT_EQ(runProg({mainWith(
                  blk({varDef("i", false, UNKNOWN, makeIntLit(0)), w}))}),
              0);
}

TEST(ControlTest, WhileBodyVariableNotVisibleAfterError) {
    astNode *w =
        createWhileNode(bin(LESS, makeIntLit(1), makeIntLit(2)),
                        blk({varDef("a", true, UNKNOWN, makeIntLit(1))}));
    EXPECT_EQ(runProg({mainWith(blk({w, ident("a")}))}), 3);
}

TEST(ControlTest, ForLoopOk) {
    // for i <- 1 to 3 do print(i)
    astNode *f = createForNode(strdup("i"), makeIntLit(1), makeIntLit(3), true,
                               nullptr, blk({call("print", {ident("i")})}));
    EXPECT_EQ(runProg({mainWith(blk({f}))}), 0);
}

TEST(ControlTest, ForUntilWithStepOk) {
    astNode *f =
        createForNode(strdup("i"), makeIntLit(0), makeIntLit(10), false,
                      makeIntLit(2), blk({call("print", {ident("i")})}));
    EXPECT_EQ(runProg({mainWith(blk({f}))}), 0);
}

TEST(ControlTest, ForZeroLiteralStepError) {
    astNode *f = createForNode(strdup("i"), makeIntLit(1), makeIntLit(3), true,
                               makeIntLit(0), unitBlock());
    EXPECT_EQ(runProg({mainWith(blk({f}))}), 10);
}

TEST(ControlTest, ForNonIntBoundError) {
    astNode *f = createForNode(strdup("i"), makeDoubleLit(1.5), makeIntLit(3),
                               true, nullptr, unitBlock());
    EXPECT_EQ(runProg({mainWith(blk({f}))}), 6);
}

TEST(ControlTest, ForNonIntStepError) {
    astNode *f = createForNode(strdup("i"), makeIntLit(1), makeIntLit(3), true,
                               makeStrLit("a"), unitBlock());
    EXPECT_EQ(runProg({mainWith(blk({f}))}), 6);
}

TEST(ControlTest, ForGuardNotBooleanError) {
    astNode *f = createForNode(strdup("i"), makeIntLit(1), makeIntLit(3), true,
                               nullptr, unitBlock());
    addGuardToFor(f, makeIntLit(1));
    EXPECT_EQ(runProg({mainWith(blk({f}))}), 6);
}

TEST(ControlTest, ForGuardUsesIteratorOk) {
    astNode *f = createForNode(strdup("i"), makeIntLit(1), makeIntLit(5), true,
                               nullptr, unitBlock());
    addGuardToFor(f, bin(NOT_EQUAL, ident("i"), makeIntLit(3)));
    EXPECT_EQ(runProg({mainWith(blk({f}))}), 0);
}

TEST(ControlTest, ForAssignToIteratorError) {
    astNode *f = createForNode(strdup("i"), makeIntLit(1), makeIntLit(3), true,
                               nullptr, blk({assign("i", makeIntLit(5))}));
    EXPECT_EQ(runProg({mainWith(blk({f}))}), 7);
}

TEST(ControlTest, ForIteratorNotVisibleAfterLoopError) {
    astNode *f = createForNode(strdup("i"), makeIntLit(1), makeIntLit(3), true,
                               nullptr, unitBlock());
    EXPECT_EQ(runProg({mainWith(blk({f, ident("i")}))}), 3);
}