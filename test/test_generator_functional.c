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

    // prepare the symtable
    symTableInit();
    paramData params[1] = {{.dataType = ANY, .id = "value"}};
    symTableInsertFun("print", UNIT, 1, params);
    symTableInsertFun("current", UNIT, 0, NULL);

    symTableActivateFun("current");
    beginBlock();
    symTableInsertVar("var1", INT, false);
    symTableFindVar("var1", NULL);

    // generate the ast and output the result
    codeGenState *state = codeGenInit();
    codeGenBlock(state, body);
    codeGenFunction(state, node);
    codeGenOutCode(state);
}

void test2(astNode **root) {
    astNode *node = *root;
    (void)node;
    // prepare the ast

    // prepare the symtable

    // generate the ast and output the result
}

void test3(astNode **root) {
    astNode *node = *root;
    (void)node;
    // prepare the ast

    // prepare the symtable

    // generate the ast and output the result
}

void test4(astNode **root) {
    astNode *node = *root;
    (void)node;
    // prepare the ast

    // prepare the symtable

    // generate the ast and output the result
}

void test5(astNode **root) {
    astNode *node = *root;
    (void)node;
    // prepare the ast

    // prepare the symtable

    // generate the ast and output the result
}

void test6(astNode **root) {
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
    codeGenBlock(state, IterWhileBody);
    endBlock();
    codeGenBlock(state, IterBody);

    codeGenFunction(state, funIter);

    // recursive function
    symTableActivateFun("factorialRecursive");
    beginBlock();
    beginBlock();
    codeGenBlock(state, RecurThenBlock);
    endBlock();
    beginBlock();
    symTableInsertVar("previous", INT, true);
    symTableInsertVar("smaller", INT, true);
    codeGenBlock(state, RecurElseBlock);
    endBlock();
    codeGenBlock(state, RecurBody);

    codeGenFunction(state, funRecur);

    // main funciton
    symTableActivateFun("factorialExample");
    beginBlock();
    symTableInsertVar("n", INT, true);
    beginBlock();
    beginBlock();
    symTableInsertVar("iterative", INT, true);
    symTableInsertVar("recursive", INT, true);
    codeGenBlock(state, mainElseElse);
    endBlock();
    beginBlock();
    codeGenBlock(state, mainElseThen);
    endBlock();
    codeGenBlock(state, mainElse);
    endBlock();
    beginBlock();
    codeGenBlock(state, mainThen);
    endBlock();
    codeGenBlock(state, ExampleBody);

    codeGenFunction(state, funExample);

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

    freeAst(root);
    symTableDestruct();

    return EXIT_SUCCESS;
}
