#include "ast.h"
#include "generator.h"
#include "symtable.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void testn(astNode **root) {
    astNode *node = *root;
    (void)node;
    // prepare the ast

    // prepare the symtable

    // generate the ast and output the result
}

void test1(astNode **root) {
    astNode *node = *root;
    // test getting a long name
    // the unique id will be single digit
    char longName[IFJ_GEN_MAX_ID_LEN];
    for (size_t i = 0; i < IFJ_GEN_MAX_ID_LEN - strlen("LF@$1") - 1; i++) {
        longName[i] = 'm';
    }
    longName[IFJ_GEN_MAX_ID_LEN - strlen("LF@$1") - 1] = '\0';

    // prepare the ast
    /*
     *          func current(Unit)->Unit
     *                  |
     *                block
     *         /           \
     *       vardef var1  call print(var1)
     *         |
     *       literal 1
     */
    astNode *body = createBlockNode();
    node = createFuncDefNode("current", true, NULL, 0, body);
    addStatementToBlock(
        body, createVarDefNode(
                  "var1", false, INT,
                  createLiteralNode(&(Token){.type = TOKEN_INT_LITERAL,
                                             .attribute = {.intValue = 1}})));
    astNode *call = createFuncCallNode("print");
    addArgumentToCall(call, createIdentifierNode("var1"));
    addStatementToBlock(body, call);
    call->data.funcCall.argIfAny = INT;
    // test creating a character with a very long name
    addStatementToBlock(body, createVarDefNode(longName, true, INT,
                                               createIdentifierNode("var1")));

    // prepare the symtable
    symTableInit();
    paramData params[1] = {{.dataType = ANY, .id = "value"}};
    symTableInsertFun("print", UNIT, 1, params);
    symTableInsertFun("current", UNIT, 0, NULL);

    symTableActivateFun("current");
    beginBlock();
    symTableInsertVar("var1", INT, false);
    symTableInsertVar(longName, INT, false);

    // generate the ast and output the result
    codeGenState *state = codeGenInit();
    codeGenBlock(state, body, false);
    codeGenFunction(state, node);
    codeGenOutCode(state);
}

void test2(astNode **root) {
    astNode *node = *root;
    // prepare the ast
    node = createProgramNode();

    Token liter1 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 1};
    Token liter2 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 2};
    Token liter3 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 3};
    Token liter4 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 4};

    // prepare function: traced(value: Int): Int
    astNode *tracedBody = createBlockNode();
    astFuncParam paramsTraced[1] = {{.name = "value", .type = INT}};
    astNode *funTraced =
        createFuncDefNode("traced", false, paramsTraced, 1, tracedBody);
    funTraced->data.funcDef.explicitReturnType = INT;

    // print("[")
    astNode *callOpenBracket = createFuncCallNode("print");
    Token strOpenBracket = {.type = TOKEN_STRING_LITERAL,
                            .attribute.text.data = "[",
                            .attribute.text.length = 1};
    addArgumentToCall(callOpenBracket, createLiteralNode(&strOpenBracket));
    addStatementToBlock(tracedBody, callOpenBracket);

    // print(value)
    astNode *callPrintValue = createFuncCallNode("print");
    addArgumentToCall(callPrintValue, createIdentifierNode("value"));
    addStatementToBlock(tracedBody, callPrintValue);

    // print("]")
    astNode *callCloseBracket = createFuncCallNode("print");
    Token strCloseBracket = {.type = TOKEN_STRING_LITERAL,
                             .attribute.text.data = "]",
                             .attribute.text.length = 1};
    addArgumentToCall(callCloseBracket, createLiteralNode(&strCloseBracket));
    addStatementToBlock(tracedBody, callCloseBracket);

    // return value (implicit return of last expression)
    addStatementToBlock(tracedBody,
                        createReturnNode(createIdentifierNode("value")));

    // prepare main function: funexpExample(): Unit
    astNode *exampleBody = createBlockNode();
    astNode *funExample =
        createFuncDefNode("funexpExample", true, NULL, 0, exampleBody);
    funExample->data.funcDef.explicitReturnType = UNIT;

    // val result = traced(2) + 3 * traced(traced(4) + 1)
    astNode *callTraced2 = createFuncCallNode("traced");
    addArgumentToCall(callTraced2, createLiteralNode(&liter2));

    astNode *callTraced4 = createFuncCallNode("traced");
    addArgumentToCall(callTraced4, createLiteralNode(&liter4));

    astNode *innerAdd =
        createBinopNode(PLUS, callTraced4, createLiteralNode(&liter1));

    astNode *callTracedOuter = createFuncCallNode("traced");
    addArgumentToCall(callTracedOuter, innerAdd);

    astNode *mulExpr =
        createBinopNode(MULTIPLY, createLiteralNode(&liter3), callTracedOuter);

    astNode *resultExpr = createBinopNode(PLUS, callTraced2, mulExpr);

    addStatementToBlock(exampleBody,
                        createVarDefNode("result", true, INT, resultExpr));

    // print("\nresult: ")
    astNode *callPrintResultLabel = createFuncCallNode("print");
    Token strResultLabel = {.type = TOKEN_STRING_LITERAL,
                            .attribute.text.data = "\nresult: ",
                            .attribute.text.length = 9};
    addArgumentToCall(callPrintResultLabel, createLiteralNode(&strResultLabel));
    addStatementToBlock(exampleBody, callPrintResultLabel);

    // print(toStr(result))
    astNode *callToStr = createFuncCallNode("toStr");
    addArgumentToCall(callToStr, createIdentifierNode("result"));
    callToStr->data.funcCall.argIfAny = INT;

    astNode *callPrintResultVal = createFuncCallNode("print");
    addArgumentToCall(callPrintResultVal, callToStr);
    addStatementToBlock(exampleBody, callPrintResultVal);

    // print("\n")
    astNode *callPrintNewline = createFuncCallNode("print");
    Token strNewline = {.type = TOKEN_STRING_LITERAL,
                        .attribute.text.data = "\n",
                        .attribute.text.length = 1};
    addArgumentToCall(callPrintNewline, createLiteralNode(&strNewline));
    addStatementToBlock(exampleBody, callPrintNewline);

    addFunctionToProgram(node, funTraced);
    addFunctionToProgram(node, funExample);

    // prepare the symtable
    symTableInit();
    paramData paramTraced[1] = {{.id = "value", .dataType = INT}};
    symTableInsertFun("traced", INT, 1, paramTraced);
    symTableInsertFun("funexpExample", UNIT, 0, NULL);
    paramData paramPrint[1] = {{.id = "n", .dataType = INT}};
    symTableInsertFun("print", UNIT, 1, paramPrint);
    symTableInsertFun("toStr", STRING, 1, paramPrint);

    codeGenState *state = codeGenInit();

    // generate traced
    symTableActivateFun("traced");
    beginBlock();
    codeGenBlock(state, tracedBody, false);
    endBlock();
    codeGenFunction(state, funTraced);

    // generate funexpExample
    symTableActivateFun("funexpExample");
    beginBlock();
    symTableInsertVar("result", INT, false);
    codeGenBlock(state, exampleBody, false);
    endBlock();
    codeGenFunction(state, funExample);

    codeGenOutCode(state);
    // prepare the ast

    // prepare the symtable

    // generate the ast and output the result
}

void test3(astNode **root) {
    astNode *node = *root;
    // prepare the ast
    node = createProgramNode();

    Token liter0 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 0};
    Token liter1 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 1};
    Token liter2 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 2};
    Token liter10 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 10};

    // prepare function: negate(value: Boolean): Boolean
    astNode *negateBody = createBlockNode();
    astFuncParam paramsNegate[1] = {{.name = "value", .type = BOOL}};
    astNode *funNegate =
        createFuncDefNode("negate", false, paramsNegate, 1, negateBody);
    funNegate->data.funcDef.explicitReturnType = BOOL;

    // !value (implicit return of last expression)
    addStatementToBlock(negateBody, createReturnNode(createUnopNode(
                                        NOT, createIdentifierNode("value"))));

    // prepare main function: boolthenExample(): Unit
    astNode *exampleBody = createBlockNode();
    astNode *funExample =
        createFuncDefNode("boolthenExample", true, NULL, 0, exampleBody);
    funExample->data.funcDef.explicitReturnType = UNIT;

    // val enabled = readBoolean()
    Token param = {.type = TOKEN_BOOL_LITERAL, .attribute.boolValue = true};
    addStatementToBlock(
        exampleBody,
        createVarDefNode("enabled", true, BOOL, createLiteralNode(&param)));

    // var divisor = 0
    addStatementToBlock(
        exampleBody,
        createVarDefNode("divisor", false, INT, createLiteralNode(&liter0)));

    // if enabled then divisor = 2 else ()
    astNode *divThenBlock = createBlockNode();
    addStatementToBlock(
        divThenBlock, createAssignNode("divisor", createLiteralNode(&liter2)));

    astNode *divElseBlock = createBlockNode();
    addStatementToBlock(divElseBlock, createUnitNode());

    astNode *divIf = createIfNode(createIdentifierNode("enabled"), divThenBlock,
                                  divElseBlock);
    addStatementToBlock(exampleBody, divIf);

    // val safe = divisor != 0 && (10 / divisor > 1)
    astNode *divisorNotZero = createBinopNode(
        NOT_EQUAL, createIdentifierNode("divisor"), createLiteralNode(&liter0));
    astNode *divTenByDivisor = createBinopNode(
        I_DIVIDE, createLiteralNode(&liter10), createIdentifierNode("divisor"));
    astNode *greaterThanOne =
        createBinopNode(GREATER, divTenByDivisor, createLiteralNode(&liter1));
    astNode *safeExpr = createBinopNode(AND, divisorNotZero, greaterThanOne);
    addStatementToBlock(exampleBody,
                        createVarDefNode("safe", true, BOOL, safeExpr));

    // val inverted = negate(enabled)
    astNode *callNegate = createFuncCallNode("negate");
    addArgumentToCall(callNegate, createIdentifierNode("enabled"));
    addStatementToBlock(exampleBody,
                        createVarDefNode("inverted", true, BOOL, callNegate));

    // val either = enabled || inverted
    astNode *eitherExpr = createBinopNode(OR, createIdentifierNode("enabled"),
                                          createIdentifierNode("inverted"));
    addStatementToBlock(exampleBody,
                        createVarDefNode("either", true, BOOL, eitherExpr));

    // print("safe: ")
    astNode *callPrintSafeLbl = createFuncCallNode("print");
    Token strSafeLbl = {.type = TOKEN_STRING_LITERAL,
                        .attribute.text.data = "safe: ",
                        .attribute.text.length = 6};
    addArgumentToCall(callPrintSafeLbl, createLiteralNode(&strSafeLbl));
    addStatementToBlock(exampleBody, callPrintSafeLbl);

    // print(safe)
    astNode *callPrintSafeVal = createFuncCallNode("print");
    addArgumentToCall(callPrintSafeVal, createIdentifierNode("safe"));
    addStatementToBlock(exampleBody, callPrintSafeVal);

    // print("\neither: ")
    astNode *callPrintEitherLbl = createFuncCallNode("print");
    Token strEitherLbl = {.type = TOKEN_STRING_LITERAL,
                          .attribute.text.data = "\neither: ",
                          .attribute.text.length = 9};
    addArgumentToCall(callPrintEitherLbl, createLiteralNode(&strEitherLbl));
    addStatementToBlock(exampleBody, callPrintEitherLbl);

    // print(either)
    astNode *callPrintEitherVal = createFuncCallNode("print");
    addArgumentToCall(callPrintEitherVal, createIdentifierNode("either"));
    addStatementToBlock(exampleBody, callPrintEitherVal);

    // print("\n")
    astNode *callPrintNl1 = createFuncCallNode("print");
    Token strNl1 = {.type = TOKEN_STRING_LITERAL,
                    .attribute.text.data = "\n",
                    .attribute.text.length = 1};
    addArgumentToCall(callPrintNl1, createLiteralNode(&strNl1));
    addStatementToBlock(exampleBody, callPrintNl1);

    // if safe then print("ready\n") else if enabled then print("blocked\n")
    // else print("disabled\n")
    astNode *elseIfElseBlock = createBlockNode();
    astNode *callPrintDisabled = createFuncCallNode("print");
    Token strDisabled = {.type = TOKEN_STRING_LITERAL,
                         .attribute.text.data = "disabled\n",
                         .attribute.text.length = 9};
    addArgumentToCall(callPrintDisabled, createLiteralNode(&strDisabled));
    addStatementToBlock(elseIfElseBlock, callPrintDisabled);

    astNode *elseIfThenBlock = createBlockNode();
    astNode *callPrintBlocked = createFuncCallNode("print");
    Token strBlocked = {.type = TOKEN_STRING_LITERAL,
                        .attribute.text.data = "blocked\n",
                        .attribute.text.length = 8};
    addArgumentToCall(callPrintBlocked, createLiteralNode(&strBlocked));
    addStatementToBlock(elseIfThenBlock, callPrintBlocked);

    astNode *innerStatusIf = createIfNode(createIdentifierNode("enabled"),
                                          elseIfThenBlock, elseIfElseBlock);

    astNode *outerStatusElseBlock = createBlockNode();
    addStatementToBlock(outerStatusElseBlock, innerStatusIf);

    astNode *outerStatusThenBlock = createBlockNode();
    astNode *callPrintReady = createFuncCallNode("print");
    Token strReady = {.type = TOKEN_STRING_LITERAL,
                      .attribute.text.data = "ready\n",
                      .attribute.text.length = 6};
    addArgumentToCall(callPrintReady, createLiteralNode(&strReady));
    addStatementToBlock(outerStatusThenBlock, callPrintReady);

    astNode *statusIf =
        createIfNode(createIdentifierNode("safe"), outerStatusThenBlock,
                     outerStatusElseBlock);
    addStatementToBlock(exampleBody, statusIf);

    // if !enabled then print("input was false\n")
    astNode *notEnabledThenBlock = createBlockNode();
    astNode *callPrintInputFalse = createFuncCallNode("print");
    Token strInputFalse = {.type = TOKEN_STRING_LITERAL,
                           .attribute.text.data = "input was false\n",
                           .attribute.text.length = 16};
    addArgumentToCall(callPrintInputFalse, createLiteralNode(&strInputFalse));
    addStatementToBlock(notEnabledThenBlock, callPrintInputFalse);

    astNode *notEnabledIf =
        createIfNode(createUnopNode(NOT, createIdentifierNode("enabled")),
                     notEnabledThenBlock, NULL);
    addStatementToBlock(exampleBody, notEnabledIf);

    addFunctionToProgram(node, funNegate);
    addFunctionToProgram(node, funExample);

    // prepare the symtable
    symTableInit();
    paramData paramNegate[1] = {{.id = "value", .dataType = BOOL}};
    symTableInsertFun("negate", BOOL, 1, paramNegate);
    symTableInsertFun("boolthenExample", UNIT, 0, NULL);
    symTableInsertFun("print", UNIT, 0, NULL);

    codeGenState *state = codeGenInit();

    // generate negate
    symTableActivateFun("negate");
    beginBlock();
    codeGenBlock(state, negateBody, false);
    endBlock();
    codeGenFunction(state, funNegate);

    // generate boolthenExample
    symTableActivateFun("boolthenExample");
    beginBlock();
    symTableInsertVar("enabled", BOOL, true);
    symTableInsertVar("divisor", INT, false);

    // divThenBlock and divElseBlock
    beginBlock();
    codeGenBlock(state, divThenBlock, false);
    endBlock();
    beginBlock();
    codeGenBlock(state, divElseBlock, false);
    endBlock();

    symTableInsertVar("safe", BOOL, true);
    symTableInsertVar("inverted", BOOL, true);
    symTableInsertVar("either", BOOL, true);

    // statusIf blocks (inner and outer)
    beginBlock();
    codeGenBlock(state, elseIfThenBlock, false);
    endBlock();
    beginBlock();
    codeGenBlock(state, elseIfElseBlock, false);
    endBlock();
    beginBlock();
    codeGenBlock(state, outerStatusThenBlock, false);
    endBlock();
    beginBlock();
    codeGenBlock(state, outerStatusElseBlock, false);
    endBlock();

    // notEnabledThenBlock
    beginBlock();
    codeGenBlock(state, notEnabledThenBlock, false);
    endBlock();

    codeGenBlock(state, exampleBody, false);
    endBlock();

    codeGenFunction(state, funExample);

    codeGenOutCode(state);
}

void test4(astNode **root) {
    astNode *node = *root;
    (void)node;
    // prepare the ast

    // prepare the symtable

    // generate the ast and output the result
}

void test5(astNode **root) {
    // test inspired by the factorial example function
    astNode *node = *root;
    // prepare the ast
    node = createProgramNode();

    Token litern = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 5};
    Token liter0 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 0};
    Token liter1 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 1};
    Token liter2 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 2};
    Token liter12 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 12};

    // prepare the iterative function
    astNode *IterBody = createBlockNode();
    astFuncParam paramsIter[1] = {{.name = "n", .type = INT}};
    astNode *funIter =
        createFuncDefNode("factorialIterative", false, paramsIter, 1, IterBody);
    funIter->data.funcDef.explicitReturnType = INT;
    // body of the function
    addStatementToBlock(IterBody, createVarDefNode("result", false, INT,
                                                   createLiteralNode(&liter1)));
    addStatementToBlock(IterBody, createVarDefNode("i", false, INT,
                                                   createLiteralNode(&liter2)));
    astNode *IterWhileBody = createBlockNode();
    addStatementToBlock(
        IterBody,
        createWhileNode(createBinopNode(LESS_EQUAL, createIdentifierNode("i"),
                                        createIdentifierNode("n")),
                        IterWhileBody));
    // while block
    addStatementToBlock(
        IterWhileBody,
        createAssignNode(
            "result", createBinopNode(MULTIPLY, createIdentifierNode("result"),
                                      createIdentifierNode("i"))));
    addStatementToBlock(
        IterWhileBody,
        createAssignNode("i", createBinopNode(PLUS, createIdentifierNode("i"),
                                              createLiteralNode(&liter1))));
    addStatementToBlock(IterBody,
                        createReturnNode(createIdentifierNode("result")));

    // prepare the recursive function
    astNode *RecurBody = createBlockNode();
    astFuncParam paramsRecur[1] = {{.name = "n", .type = INT}};
    astNode *funRecur = createFuncDefNode("factorialRecursive", false,
                                          paramsRecur, 1, RecurBody);
    funRecur->data.funcDef.explicitReturnType = INT;
    // body of the function
    astNode *RecurThenBlock = createBlockNode();
    astNode *RecurElseBlock = createBlockNode();
    addStatementToBlock(
        RecurBody,
        createIfNode(createBinopNode(LESS_EQUAL, createIdentifierNode("n"),
                                     createLiteralNode(&liter1)),
                     RecurThenBlock, RecurElseBlock));
    // then statement
    addStatementToBlock(RecurThenBlock,
                        createReturnNode(createLiteralNode(&liter1)));
    // else statement
    addStatementToBlock(
        RecurElseBlock,
        createVarDefNode("previous", true, INT,
                         createBinopNode(MINUS, createIdentifierNode("n"),
                                         createLiteralNode(&liter1))));
    astNode *callRec = createFuncCallNode("factorialRecursive");
    addArgumentToCall(callRec, createIdentifierNode("previous"));
    addStatementToBlock(RecurElseBlock,
                        createVarDefNode("smaller", true, INT, callRec));
    addStatementToBlock(RecurElseBlock, createReturnNode(createBinopNode(
                                            MULTIPLY, createIdentifierNode("n"),
                                            createIdentifierNode("smaller"))));

    // prepare the main function
    astNode *ExampleBody = createBlockNode();
    astNode *funExample =
        createFuncDefNode("factorialExample", false, NULL, 0, ExampleBody);
    funExample->data.funcDef.explicitReturnType = UNIT;
    funExample->data.funcDef.isMain = true;
    // main body
    astNode *mainThen = createBlockNode();
    astNode *mainElse = createBlockNode();
    astNode *mainElseThen = createBlockNode();
    astNode *mainElseElse = createBlockNode();
    addStatementToBlock(
        ExampleBody,
        createVarDefNode("n", false, INT, createLiteralNode(&litern)));
    addStatementToBlock(
        ExampleBody,
        createIfNode(createBinopNode(LESS, createIdentifierNode("n"),
                                     createLiteralNode(&liter0)),
                     mainThen, mainElse));
    // then block
    astNode *call1 = createFuncCallNode("print");
    char str1s[] = "Use an integer from 0 to 12.\n";
    Token str1 = {.type = TOKEN_STRING_LITERAL,
                  .attribute.text.data = str1s,
                  .attribute.text.length = strlen(str1s)};
    addArgumentToCall(call1, createLiteralNode(&str1));
    addStatementToBlock(mainThen, call1);
    // else block
    addStatementToBlock(
        mainElse,
        createIfNode(createBinopNode(GREATER, createIdentifierNode("n"),
                                     createLiteralNode(&liter12)),
                     mainElseThen, mainElseElse));
    // else then block
    astNode *call2 = createFuncCallNode("print");
    Token str2 = {.type = TOKEN_STRING_LITERAL,
                  .attribute.text.data = str1s,
                  .attribute.text.length = strlen(str1s)};
    addArgumentToCall(call2, createLiteralNode(&str2));
    addStatementToBlock(mainElseThen, call2);
    // else else block

    astNode *call3 = createFuncCallNode("factorialIterative");
    addArgumentToCall(call3, createIdentifierNode("n"));
    addStatementToBlock(mainElseElse,
                        createVarDefNode("iterative", true, INT, call3));

    astNode *call4 = createFuncCallNode("factorialRecursive");
    addArgumentToCall(call4, createIdentifierNode("n"));
    addStatementToBlock(mainElseElse,
                        createVarDefNode("recursive", true, INT, call4));

    astNode *call5 = createFuncCallNode("print");
    char str5s[] = "iterative: ";
    Token str5 = {.type = TOKEN_STRING_LITERAL,
                  .attribute.text.data = str5s,
                  .attribute.text.length = strlen(str5s)};
    addArgumentToCall(call5, createLiteralNode(&str5));
    addStatementToBlock(mainElseElse, call5);

    astNode *call6 = createFuncCallNode("print");
    addArgumentToCall(call6, createIdentifierNode("iterative"));
    addStatementToBlock(mainElseElse, call6);

    astNode *call7 = createFuncCallNode("print");
    char str7s[] = "\nrecursive: ";
    Token str7 = {.type = TOKEN_STRING_LITERAL,
                  .attribute.text.data = str7s,
                  .attribute.text.length = strlen(str7s)};
    addArgumentToCall(call7, createLiteralNode(&str7));
    addStatementToBlock(mainElseElse, call7);

    astNode *call8 = createFuncCallNode("print");
    addArgumentToCall(call8, createIdentifierNode("recursive"));
    addStatementToBlock(mainElseElse, call8);

    astNode *call9 = createFuncCallNode("print");
    Token str9 = {.type = TOKEN_STRING_LITERAL,
                  .attribute.text.data = "\n",
                  .attribute.text.length = 1};
    addArgumentToCall(call9, createLiteralNode(&str9));
    addStatementToBlock(mainElseElse, call9);

    addFunctionToProgram(node, funIter);
    addFunctionToProgram(node, funRecur);
    addFunctionToProgram(node, funExample);

    // prepare the symtable
    symTableInit();
    paramData param[1] = {{.id = "n", .dataType = INT}};
    symTableInsertFun("factorialIterative", INT, 1, param);
    symTableInsertFun("factorialRecursive", INT, 1, param);
    symTableInsertFun("factorialExample", UNIT, 0, NULL);
    symTableInsertFun("print", UNIT, 0, NULL);

    codeGenState *state = codeGenInit();

    // generate from top to bottom, how the semantics would handle it
    // iterative function
    symTableActivateFun("factorialIterative");
    beginBlock();
    symTableInsertVar("result", INT, true);
    symTableInsertVar("i", INT, true);
    beginBlock();
    codeGenBlock(state, IterWhileBody, false);
    endBlock();
    codeGenBlock(state, IterBody, false);

    codeGenFunction(state, funIter);

    // recursive function
    symTableActivateFun("factorialRecursive");
    beginBlock();
    beginBlock();
    codeGenBlock(state, RecurThenBlock, false);
    endBlock();
    beginBlock();
    symTableInsertVar("previous", INT, true);
    symTableInsertVar("smaller", INT, true);
    codeGenBlock(state, RecurElseBlock, false);
    endBlock();
    codeGenBlock(state, RecurBody, false);

    codeGenFunction(state, funRecur);

    // main funciton
    symTableActivateFun("factorialExample");
    beginBlock();
    symTableInsertVar("n", INT, true);
    beginBlock();
    beginBlock();
    symTableInsertVar("iterative", INT, true);
    symTableInsertVar("recursive", INT, true);
    codeGenBlock(state, mainElseElse, false);
    endBlock();
    beginBlock();
    codeGenBlock(state, mainElseThen, false);
    endBlock();
    codeGenBlock(state, mainElse, false);
    endBlock();
    beginBlock();
    codeGenBlock(state, mainThen, false);
    endBlock();
    codeGenBlock(state, ExampleBody, false);

    codeGenFunction(state, funExample);

    codeGenOutCode(state);
}

void test6(astNode **root) {
    astNode *node = *root;
    // prepare the ast
    node = createProgramNode();

    Token liter2 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 2};
    Token liter3 = {.type = TOKEN_INT_LITERAL, .attribute.intValue = 3};
    Token literDouble1_5 = {.type = TOKEN_DOUBLE_LITERAL,
                            .attribute.doubleValue = 1.5};
    Token literBoolTrue = {.type = TOKEN_BOOL_LITERAL,
                           .attribute.boolValue = true};

    // prepare function: combineValues(a: Int, b: Double, flag: Boolean): Double
    astNode *combineBody = createBlockNode();
    astFuncParam paramsCombine[3] = {{.name = "a", .type = INT},
                                     {.name = "b", .type = DOUBLE},
                                     {.name = "flag", .type = BOOL}};
    astNode *funCombine = createFuncDefNode("combineValues", false,
                                            paramsCombine, 3, combineBody);
    funCombine->data.funcDef.explicitReturnType = DOUBLE;

    // val label = "values:"
    Token strLabel = {.type = TOKEN_STRING_LITERAL,
                      .attribute.text.data = "values:",
                      .attribute.text.length = 7};
    addStatementToBlock(
        combineBody,
        createVarDefNode("label", true, STRING, createLiteralNode(&strLabel)));

    // val strLabel = toStr(label)  [STRING]
    astNode *callToStrLabel = createFuncCallNode("toStr");
    callToStrLabel->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callToStrLabel, createIdentifierNode("label"));
    addStatementToBlock(combineBody, createVarDefNode("strLabel", true, STRING,
                                                      callToStrLabel));

    // val strFlag = toStr(flag)  [BOOLEAN]
    astNode *callToStrFlag = createFuncCallNode("toStr");
    callToStrFlag->data.funcCall.argIfAny = BOOL;
    addArgumentToCall(callToStrFlag, createIdentifierNode("flag"));
    addStatementToBlock(
        combineBody, createVarDefNode("strFlag", true, STRING, callToStrFlag));

    // val strUnit = toStr(())  [UNIT]
    astNode *callToStrUnit = createFuncCallNode("toStr");
    callToStrUnit->data.funcCall.argIfAny = UNIT;
    addArgumentToCall(callToStrUnit, createUnitNode());
    addStatementToBlock(
        combineBody, createVarDefNode("strUnit", true, STRING, callToStrUnit));

    // val strDouble = toStr(b)  [DOUBLE]
    astNode *callToStrDouble = createFuncCallNode("toStr");
    callToStrDouble->data.funcCall.argIfAny = DOUBLE;
    addArgumentToCall(callToStrDouble, createIdentifierNode("b"));
    addStatementToBlock(combineBody, createVarDefNode("strDouble", true, STRING,
                                                      callToStrDouble));

    // print(strLabel); print(" "); print(strFlag); print(" "); print(strUnit);
    // print(" "); print(strDouble); print("\n")
    astNode *callPrint1 = createFuncCallNode("print");
    callPrint1->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrint1, createIdentifierNode("strLabel"));
    addStatementToBlock(combineBody, callPrint1);

    astNode *callPrintSp1 = createFuncCallNode("print");
    Token strSp1 = {.type = TOKEN_STRING_LITERAL,
                    .attribute.text.data = " ",
                    .attribute.text.length = 1};
    callPrintSp1->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrintSp1, createLiteralNode(&strSp1));
    addStatementToBlock(combineBody, callPrintSp1);

    astNode *callPrint2 = createFuncCallNode("print");
    callPrint2->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrint2, createIdentifierNode("strFlag"));
    addStatementToBlock(combineBody, callPrint2);

    astNode *callPrintSp2 = createFuncCallNode("print");
    Token strSp2 = {.type = TOKEN_STRING_LITERAL,
                    .attribute.text.data = " ",
                    .attribute.text.length = 1};
    callPrintSp2->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrintSp2, createLiteralNode(&strSp2));
    addStatementToBlock(combineBody, callPrintSp2);

    astNode *callPrint3 = createFuncCallNode("print");
    callPrint3->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrint3, createIdentifierNode("strUnit"));
    addStatementToBlock(combineBody, callPrint3);

    astNode *callPrintSp3 = createFuncCallNode("print");
    Token strSp3 = {.type = TOKEN_STRING_LITERAL,
                    .attribute.text.data = " ",
                    .attribute.text.length = 1};
    callPrintSp3->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrintSp3, createLiteralNode(&strSp3));
    addStatementToBlock(combineBody, callPrintSp3);

    astNode *callPrint4 = createFuncCallNode("print");
    callPrint4->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrint4, createIdentifierNode("strDouble"));
    addStatementToBlock(combineBody, callPrint4);

    astNode *callPrintNlCombine = createFuncCallNode("print");
    Token strNlCombine = {.type = TOKEN_STRING_LITERAL,
                          .attribute.text.data = "\n",
                          .attribute.text.length = 1};
    callPrintNlCombine->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrintNlCombine, createLiteralNode(&strNlCombine));
    addStatementToBlock(combineBody, callPrintNlCombine);

    // val base = a * 2
    addStatementToBlock(
        combineBody,
        createVarDefNode("base", true, INT,
                         createBinopNode(MULTIPLY, createIdentifierNode("a"),
                                         createLiteralNode(&liter2))));

    // val convBase = base.toDouble
    addStatementToBlock(
        combineBody,
        createVarDefNode("convBase", true, DOUBLE,
                         createInt2DoubleNode(createIdentifierNode("base"))));

    // val total = convBase + b
    addStatementToBlock(
        combineBody,
        createVarDefNode("total", true, DOUBLE,
                         createBinopNode(PLUS, createIdentifierNode("convBase"),
                                         createIdentifierNode("b"))));

    // return total
    addStatementToBlock(combineBody,
                        createReturnNode(createIdentifierNode("total")));

    // prepare main function: multiParamExample(): Unit
    astNode *mainBody = createBlockNode();
    astNode *funMain =
        createFuncDefNode("multiParamExample", true, NULL, 0, mainBody);
    funMain->data.funcDef.explicitReturnType = UNIT;

    // val res = combineValues(3, 1.5, true)
    astNode *callCombine = createFuncCallNode("combineValues");
    addArgumentToCall(callCombine, createLiteralNode(&liter3));
    addArgumentToCall(callCombine, createLiteralNode(&literDouble1_5));
    addArgumentToCall(callCombine, createLiteralNode(&literBoolTrue));
    addStatementToBlock(mainBody,
                        createVarDefNode("res", true, DOUBLE, callCombine));

    // print("unit_print: ")
    astNode *callPrintUnitLbl = createFuncCallNode("print");
    Token strUnitLbl = {.type = TOKEN_STRING_LITERAL,
                        .attribute.text.data = "unit_print: ",
                        .attribute.text.length = 12};
    callPrintUnitLbl->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrintUnitLbl, createLiteralNode(&strUnitLbl));
    addStatementToBlock(mainBody, callPrintUnitLbl);

    // print(())  [UNIT]
    astNode *callPrintUnit = createFuncCallNode("print");
    callPrintUnit->data.funcCall.argIfAny = UNIT;
    addArgumentToCall(callPrintUnit, createUnitNode());
    addStatementToBlock(mainBody, callPrintUnit);

    // print("\nres: ")
    astNode *callPrintResLbl = createFuncCallNode("print");
    Token strResLbl = {.type = TOKEN_STRING_LITERAL,
                       .attribute.text.data = "\nres: ",
                       .attribute.text.length = 6};
    callPrintResLbl->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrintResLbl, createLiteralNode(&strResLbl));
    addStatementToBlock(mainBody, callPrintResLbl);

    // print(toStr(res))  [DOUBLE -> STRING]
    astNode *callToStrRes = createFuncCallNode("toStr");
    callToStrRes->data.funcCall.argIfAny = DOUBLE;
    addArgumentToCall(callToStrRes, createIdentifierNode("res"));

    astNode *callPrintResVal = createFuncCallNode("print");
    callPrintResVal->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrintResVal, callToStrRes);
    addStatementToBlock(mainBody, callPrintResVal);

    // print("\n")
    astNode *callPrintNlMain = createFuncCallNode("print");
    Token strNlMain = {.type = TOKEN_STRING_LITERAL,
                       .attribute.text.data = "\n",
                       .attribute.text.length = 1};
    callPrintNlMain->data.funcCall.argIfAny = STRING;
    addArgumentToCall(callPrintNlMain, createLiteralNode(&strNlMain));
    addStatementToBlock(mainBody, callPrintNlMain);

    addFunctionToProgram(node, funCombine);
    addFunctionToProgram(node, funMain);

    // prepare the symtable
    symTableInit();
    paramData paramsCombineSym[3] = {{.id = "a", .dataType = INT},
                                     {.id = "b", .dataType = DOUBLE},
                                     {.id = "flag", .dataType = BOOL}};
    symTableInsertFun("combineValues", DOUBLE, 3, paramsCombineSym);
    symTableInsertFun("multiParamExample", UNIT, 0, NULL);
    paramData paramPrint[1] = {{.id = "n", .dataType = INT}};
    symTableInsertFun("print", UNIT, 1, paramPrint);
    symTableInsertFun("toStr", STRING, 1, paramPrint);

    codeGenState *state = codeGenInit();

    // generate combineValues
    symTableActivateFun("combineValues");
    beginBlock();
    symTableInsertVar("label", STRING, true);
    symTableInsertVar("strLabel", STRING, true);
    symTableInsertVar("strFlag", STRING, true);
    symTableInsertVar("strUnit", STRING, true);
    symTableInsertVar("strDouble", STRING, true);
    symTableInsertVar("base", INT, true);
    symTableInsertVar("convBase", DOUBLE, true);
    symTableInsertVar("total", DOUBLE, true);
    codeGenBlock(state, combineBody, false);
    endBlock();
    codeGenFunction(state, funCombine);

    // generate multiParamExample
    symTableActivateFun("multiParamExample");
    beginBlock();
    symTableInsertVar("res", DOUBLE, true);
    codeGenBlock(state, mainBody, false);
    endBlock();
    codeGenFunction(state, funMain);

    codeGenOutCode(state);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "1 argument expected - the number of the test\n");
        return EXIT_FAILURE;
    }

    astNode *root;

    switch (atoi(argv[1])) {
    case 1:
        test1(&root);
        break;
    case 2:
        test2(&root);
        break;
    case 3:
        test3(&root);
        break;
    case 4:
        test4(&root);
        break;
    case 5:
        test5(&root);
        break;
    case 6:
        test6(&root);
        break;
    default:
        fprintf(stderr, "not a valid test number\n");
        return EXIT_FAILURE;
    }

    symTableDestruct();
    freeAst(root);

    return EXIT_SUCCESS;
}
