/**
 * @file generator.c
 * @brief implementation of IFJcode26 generator
 * @author Štěpán Dvořák, xdvoras00@stud.fit.vutbr.cz
 */

#include "generator.h"
#include "ast.h"
#include "compiler.h"
#include "symtable.h"
#include "token.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IFJ_GEN_DEBUG
/** a macro for checking if the requirements set for codeGenBlock and codeGenFun
 * to work are met */
#ifdef IFJ_GEN_DEBUG
    #define GEN_CHECK_PRINT(cond, retFail, str, ...)                           \
        if (!(cond)) {                                                         \
            char buf[512];                                                     \
            sprintf(buf, str, ##__VA_ARGS__);                                  \
            fprintf(stderr, "GENERR: %s at %i: %s\n", __FUNCTION__, __LINE__,  \
                    buf);                                                      \
            return retFail;                                                    \
        }                                                                      \
        (void)0

    #define GEN_CHECK(cond, retFail)                                           \
        if (!(cond))                                                           \
        return retFail
#else
    #define GEN_CHECK(cond, str, ...) (void)0
    #define GEN_CHECK(cond, retFail) (void)0
#endif

#define IFJ_GEN_TRY(fun, retFail)                                              \
    if (fun != 0) {                                                            \
        return retFail;                                                        \
    }                                                                          \
    (void)0

codeGenState *codeGenInit() {
    // the text for builtin functions
    static char builtinCode[] =
        "LABEL ord\n"  // function ord(value:String, index:Int)->Int and
        "POPS GF@G1\n" // GF@G1 = value
        "POPS GF@G2\n" // GF@G2 = index

        // check if index is smaller, than 0
        "LT GF@G3 GF@G2 int@0\n"
        "JUMPIFEQ ord$fail GF@G3 bool@true\n"

        // check is index is greater of equal than the string
        // lentht(=not(strlen>index)) (GF@G3 = strlen(value))
        "STRLEN GF@G3 GF@G1\n"
        "GT GF@G3 GF@G3 GF@G2\n"
        "JUMPIFNEQ ord$fail GF@G3 BOOL@true\n"

        // get the character at index and push it to stack
        "GETCHAR GF@G1 GF@G1 GF@G2\n"
        "PUSHS GF@G1\n"
        "RETURN\n"

        // on fail push -1 to stack
        "LABEL ord$fail\n"
        "PUSHS INT@-1\n"
        "RETURN\n"

        // function subStr(value:String, start:Int, end:Int)->String
        "LABEL subStr\n"
        // initialize the function frame
        "CREATEFRAME\n"
        "PUSHFRAME\n"
        "DEFVAR LF@value\n"
        "DEFVAR LF@start\n"
        "DEFVAR LF@end\n"
        "POPS LF@value\n"
        "POPS LF@start\n"
        "POPS LF@end\n"

        // if start < 0
        "LT GF@G1 LF@start int@0\n"
        "JUMPIFEQ subStr$FAIL GF@G1 bool@true\n"

        // if end < 0
        "LT GF@G1 LF@end int@0\n"
        "JUMPIFEQ subStr$FAIL GF@G1 bool@true\n"

        // if start > end
        "GT GF@G1 LF@start LF@end\n"
        "JUMPIFEQ subStr$FAIL GF@G1 bool@true\n"

        // if if start >= strlen(value) (=if !(start < strlen(vlalue)))
        "STRLEN GF@G2 LF@value\n"
        "LT GF@G1 LF@start GF@G2\n"
        "JUMPIFEQ subStr$FAIL GF@G1 bool@false\n"

        // if end > strlen(value)
        "GT GF@G1 LF@end GF@G2\n"
        "JUMPIFEQ subStr$FAIL GF@G1 bool@true\n"

        "DEFVAR LF@res\n"
        "MOVE LF@res string@\n"

        // while start < end
        "LABEL subStr$while$start\n"
        "LT GF@G1 LF@start LF@end\n"
        "JUMPIFEQ subStr$while$end GF@G1 bool@false\n"

        // res = concat(res, value[start])
        "GETCHAR GF@G1 LF@value LF@start\n"
        "CONCAT LF@res LF@RES GF@G1\n"

        // start += 1
        "ADD LF@start LF@start int@1\n"
        "JUMP subStr$while$start\n"
        "LABEL subStr$while$end\n"

        "PUSHS LF@res\n"
        "POPFRAME\n"
        "RETURN\n"
        "LABEL subStr$FAIL\n"
        "EXIT int@40\n";
    static codeGenState state = {.firstBuf = NULL,
                                 .lastBuf = NULL,
                                 .bufsCreated = 0,
                                 .firstFun = NULL,
                                 .lastFun = NULL,
                                 .mainFunName = "",
                                 .builtinFuns = builtinCode};
    return &state;
}

void codeGenDestruct(codeGenState *state) {
    codeGenBufNode *next;
    for (codeGenBufNode *curr = state->firstBuf; curr != NULL; curr = next) {
        next = curr->nextBuf;
        codeGenDestructBuf(curr);
    }
    state->firstBuf = state->lastBuf = NULL;

    codeGenDiscard(state);
}

int codeGenBlock(codeGenState *state, astNode *blockRoot) {
    if (!state || !blockRoot) {
        return 1;
    }
    GEN_CHECK_PRINT(blockRoot->type == AST_BLOCK, -1,
                    "The node must be a block. Type is %i", blockRoot->type);

    // create a new buffer for generating
    codeGenBufNode *buffer = codeGenCreateBuf(state);
    if (!buffer) {
        return -1;
    }

    // generate all the statements in the block into the created buffer
    IFJ_GEN_TRY(codeGenStatementArr(blockRoot->data.block.statements,
                                    blockRoot->data.block.count, buffer),
                -1);

    // set the ptr to the created generated buffer in ast
    blockRoot->data.block.bufferPtr = (void *)buffer;
    return 0;
}

int codeGenFunction(codeGenState *state, astNode *funRoot) {
    if (!funRoot) {
        return 1;
    }

    // if the function is main, copy its name
    if (funRoot->data.funcDef.isMain) {
        strcpy(state->mainFunName, funRoot->data.funcDef.name);
    }

    codeGenBufNode *bodyBuffer =
        (codeGenBufNode *)(funRoot->data.funcDef.body->data.block.bufferPtr);

    const char header[] = "LABEL %s\n"
                          "CREATEFRAME\n"
                          "PUSHFRAME\n";
    const char singleParam[] = "DEFVAR %s$0\n"
                               "POPS %s$0\n";
    const char footer[] = "LABEL %s$return\n"
                          "POPFRAME\n"
                          "RETURN\n";

    // first get the size of the allocated chunk of memory
    // get the size of the function overhead
    size_t totalLen = 0;
    totalLen += strlen(header) + strlen(footer) - 2 * 2 +
                2 * strlen(funRoot->data.funcDef.name);
    for (size_t i = 0; i < funRoot->data.funcDef.paramCount; i++) {
        totalLen += strlen(singleParam) - 2 * 2 +
                    strlen(funRoot->data.funcDef.params[i].name);
    }
    // get the size of the body
    totalLen += codeGenBufLenght(bodyBuffer);

    // allocate a chunk of memory for the whole function and its output buffer
    outBufNode *outBuf = malloc(sizeof(outBufNode));

    if (!outBuf) {
        setErrCode(99);
        return -1;
    }

    char *outBufData = malloc(totalLen + 1);

    if (!outBufData) {
        setErrCode(99);
        free(outBuf);
        return -1;
    }
    outBuf->buf = outBufData;
    outBuf->bufSize = totalLen;

    int pos = 0; // for keeping the position in the outBufData
    // generate the function header
    pos += sprintf(&outBufData[pos], header, funRoot->data.funcDef.name);

    // generate the parameter creation
    for (size_t i = 0; i < funRoot->data.funcDef.paramCount; i++) {
        const char *paramName = funRoot->data.funcDef.params[i].name;
        pos += sprintf(&outBufData[pos], singleParam, paramName, paramName);
    }

    // insert the body
    pos += codeGenOutputBuf(&outBufData[pos], bodyBuffer);

    // generate the footer
    pos += sprintf(&outBufData[pos], footer, funRoot->data.funcDef.name);

    // insert the outBuf into the state
    if (state->lastFun) {
        state->lastFun->nextBuf = outBuf;
    }
    state->lastFun = outBuf;

    return 0;
}

int codeGenOutCode(codeGenState *state) {
    // output the file header (including the builtin functions at the end)
    int len = printf(".IFJcode26\n"
                     "DEFVAR GF@void\n"
                     "DEFVAR GF@G1\n"
                     "DEFVAR GF@G2\n"
                     "DEFVAR GF@G3\n"
                     "CALL %s\n"
                     "EXIT int@0\n%s",
                     state->mainFunName, state->builtinFuns);

    // output all the functoins
    outBufNode *next;
    for (outBufNode *curr = state->firstFun; curr != NULL; curr = next) {
        puts(curr->buf); // output the whole buffer to stdout

        len += curr->bufSize;
        next = curr->nextBuf;

        free(curr); // free the buffers as we go
    }
    state->firstFun = state->lastFun = NULL;

    return len;
}

void codeGenDiscard(codeGenState *state) {
    outBufNode *next;

    for (outBufNode *curr = state->firstFun; curr != NULL; curr = next) {
        next = curr->nextBuf;

        free(curr); // free the buffers as we go
    }
    state->firstFun = state->lastFun = NULL;

    // discard the main function name
    state->mainFunName[0] = '\0';
}

/*
 * --------------------
 *  INTERNAL FUNCTIONS
 * --------------------
 */

codeGenBufNode *codeGenCreateBuf(codeGenState *state) {
    codeGenBufNode *tempBufP = malloc(sizeof(codeGenBufNode));
    if (!tempBufP) {
        setErrCode(99);
        return NULL;
    }
    bufInsertNode *tempNodeP = malloc(sizeof(bufInsertNode));
    if (!tempNodeP) {
        goto err_node;
    }

    bufSection *tempSectionP = malloc(sizeof(bufSection));
    if (!tempSectionP) {
        goto err_area;
    }

    // initialize the buffer
    codeGenBufNode buf = {.firstNode = tempNodeP,
                          .lastNode = tempNodeP,
                          .firstSection = tempSectionP,
                          .lastSection = tempSectionP,
                          .totalSize = 0,
                          .labelNum = 0,
                          .nextBuf = NULL,
                          .bufNum = state->bufsCreated++};
    memcpy(tempBufP, &buf, sizeof(codeGenBufNode));

    return tempBufP;

err_area:
    free(tempNodeP);
err_node:
    free(tempBufP);
    setErrCode(99);
    return NULL;
}

void codeGenDestructBuf(codeGenBufNode *buffer) {
    if (!buffer) {
        return;
    }
    // free all the metadata
    bufInsertNode *nextNode;
    for (bufInsertNode *curr = buffer->firstNode; curr != NULL;
         curr = nextNode) {
        nextNode = curr->next;
        free(curr);
    }

    // free all the text areas
    bufSection *nextSection;
    for (bufSection *curr = buffer->firstSection; curr != NULL;
         curr = nextSection) {
        nextSection = curr->next;
        free(curr);
    }

    free(buffer);
}

int codeGenBufLenght(codeGenBufNode *buffer) {
    // add the size of the text in this buffer
    int len = buffer->totalSize;

    // add lenghts of all the inserted buffers
    bufInsertNode *node = buffer->firstNode;
    while (node) {
        for (int i = 0; i < node->size; i++) {
            len += codeGenBufLenght(node->data[i].childBuff);
        }
        node = node->next;
    }

    return len;
}

int codeGenInsertTxt(const char *txt, codeGenBufNode *buffer) {
    size_t txtLen = strlen(txt);
    bufSection *potentialDest = buffer->lastSection;

    if (txtLen + potentialDest->size >= IFJ_GEN_BUF_SIZE - 1) {
        // the last section's capacity is not sufficient
        potentialDest = malloc(sizeof(bufSection));
        if (!potentialDest) {
            setErrCode(99);
            return -1;
        }

        potentialDest->next = NULL;
        potentialDest->size = 0;

        // append new section to the end
        buffer->lastSection->next = potentialDest;
        buffer->lastSection = potentialDest;
    }
    // else potentialDest stays as the last section

    // copy the text into the destination buffer
    // dest - ptr to the first unused character in the ->data (null character)
    // len - copy the text and the null character
    memcpy(&(potentialDest->data[potentialDest->size]), txt, txtLen + 1);

    // update the size
    potentialDest->size += txtLen;
    buffer->totalSize += txtLen;
    return 0;
}

int codeGenInsertBufToTxt(codeGenBufNode *destBuf,
                          codeGenBufNode *insertedBuf) {
    bufInsertNode *potentialDest = destBuf->lastNode;
    if (potentialDest->size >= IFJ_GEN_BUF_INSERT_NUM) {
        // the last node's capacity is full - we need to create a new one
        potentialDest = malloc(sizeof(bufInsertNode));
        if (!potentialDest) {
            setErrCode(99);
            return -1;
        }

        potentialDest->next = NULL;
        potentialDest->size = 0;

        // we append the new node
        destBuf->lastNode->next = potentialDest;
        destBuf->lastNode = potentialDest;
    }
    // else potentialDest can stay as the last node in the list

    // insert the new reference to the first viable position
    potentialDest->data[potentialDest->size++] =
        (bufInsert){.childBuff = insertedBuf, .textPos = destBuf->totalSize};
    // update the buffer size

    return 0;
}

int codeGenOutputBuf(char *outBuf, codeGenBufNode *buffer) {
    int len = 0;

    // add lenghts of all the inserted buffers
    bufInsertNode *node = buffer->firstNode;
    bufSection *section = buffer->firstSection;
    int posInSection = 0;

    while (node) {
        for (int i = 0; i < node->size; i++) {
            bufInsert nextInsert = node->data[i];
            // the nextInsert.textPos will be acting a the number of characters,
            // that are left to be copied from now on

            // copy the text preceeding each inserted buffer
            // copy the first section if it can be all copied before the next
            // insert
            if (section->size - posInSection < nextInsert.textPos) {
                memcpy(&outBuf[len], &section->data[posInSection],
                       section->size - posInSection);
                len += section->size - posInSection;
                nextInsert.textPos -= (section->size - posInSection);

                section = section->next;
            }
            // copy all the sections, that can be copied whole
            while (section->size < nextInsert.textPos) {
                memcpy(&outBuf[len], section->data, section->size);
                len += section->size;
                nextInsert.textPos -= section->size;

                section = section->next;
            }
            // copy the rest from the section bigger, than what is left to be
            // copied
            memcpy(&outBuf[len], &section->data[posInSection],
                   nextInsert.textPos);
            len += nextInsert.textPos;
            posInSection = posInSection + nextInsert.textPos;

            // copy the inserted buffer
            len += codeGenOutputBuf(&outBuf[len], buffer);
        }
        node = node->next;
    }

    // copy what is left after the last inserted buffer
    while (section) {
        memcpy(&outBuf[len], section->data, section->size);
        len += section->size;
        section = section->next;
    }

    // make sure the string ends in \0 but don't include it in the lenght
    // (otherwise it will not be overwritten in the next insert)
    outBuf[len] = '\0';
    return len;
}

int codeGenStatementArr(astNode **nodes, size_t count, codeGenBufNode *buffer) {
    for (size_t i = 0; i < count; i++) {
        // generate all statements. By default don't leave value on stack
        // (return value should be under AST_RETURN)
        IFJ_GEN_TRY(codeGenStatement(nodes[i], buffer, false), -1);
    }
    return 0;
}

int codeGenStatement(astNode *node, codeGenBufNode *buffer, bool leaveVal) {
    // determine the type of node and call appropriate function
    // ignore a function only returning a value on stack if leaveVal is false
    switch (node->type) {
    case AST_VAR_DEF:
        IFJ_GEN_TRY(codeGenStmtVarDef(node, buffer), -1);
        break;

    case AST_ASSIGN:
        IFJ_GEN_TRY(codeGenStmtAssign(node, buffer), -1);
        break;

    case AST_IF:
        IFJ_GEN_TRY(codeGenStmtIf(node, buffer), -1);
        break;

    case AST_WHILE:
        IFJ_GEN_TRY(codeGenStmtWhile(node, buffer), -1);
        break;

    case AST_FOR:
        IFJ_GEN_TRY(codeGenStmtFor(node, buffer), -1);
        break;

    case AST_FUNC_CALL:
        if (leaveVal) {
            IFJ_GEN_TRY(codeGenStmtFuncCall(node, buffer, leaveVal), -1);
        }
        break;

    case AST_RETURN:
        IFJ_GEN_TRY(codeGenStmtReturn(node, buffer), -1);
        break;

    case AST_BINOP:
        if (leaveVal) {
            IFJ_GEN_TRY(codeGenStmtBinOp(node, buffer), -1);
        }
        break;

    case AST_UNOP:
        if (leaveVal) {
            IFJ_GEN_TRY(codeGenStmtUnOp(node, buffer), -1);
        }
        break;

    case AST_LITERAL:
        if (leaveVal) {
            IFJ_GEN_TRY(codeGenStmtLiteral(node, buffer), -1);
        }
        break;

    case AST_IDENTIFIER:
        if (leaveVal) {
            IFJ_GEN_TRY(codeGenStmtIdentif(node, buffer), -1);
        }
        break;

    case AST_BLOCK:
        IFJ_GEN_TRY(codeGenStmtBlock(node, buffer), -1);
        break;

    default:
        GEN_CHECK_PRINT(false, -1,
                        "Unsupported ast node in tree (PROGRAMM or FUNC_DEF "
                        "inside a function)");
    }
    return 0;
}

int codeGenStmtVarDef(astNode *varDef, codeGenBufNode *buffer) {
    // get the name in IFJcode26
    const char *name = codeGenGetVarName(varDef->data.varDef.name);
    GEN_CHECK(name != NULL, -1);

    const char overhead[] = "DEFVAR %s\n"
                            "POPS %s\n";
    size_t overheadLen = 2 * strlen(name) + strlen(overhead);
    // set the buffer lenght
    char buf[overheadLen + 1];

    sprintf(buf, overhead, name, name);
    // release hold of the name buffer and generate RHS (the initializing value
    // should be left on stack)

    // generate the assigned value and expect result on stack
    IFJ_GEN_TRY(codeGenStatement(varDef->data.varDef.initializer, buffer, true),
                -1);
    IFJ_GEN_TRY(codeGenInsertTxt(buf, buffer), -1);
    return 0;
}

int codeGenStmtAssign(astNode *assign, codeGenBufNode *buffer) {
    // get the name in IFJcode26
    const char *name = codeGenGetVarName(assign->data.assign.name);
    GEN_CHECK(name != NULL, -1);

    const char overhead[] = "POPS %s\n";
    size_t overheadLen = strlen(name) + strlen(overhead);
    // set the buffer lenght
    char buf[overheadLen + 1];

    sprintf(buf, overhead, name);
    // release hold of the name buffer and generate RHS (the initializing value
    // should be left on stack)

    // generate the assigned value and expect result on stack
    IFJ_GEN_TRY(codeGenStatement(assign->data.assign.value, buffer, true), -1);
    IFJ_GEN_TRY(codeGenInsertTxt(buf, buffer), -1);
    return 0;
}

int codeGenStmtIf(astNode *ifStmt, codeGenBufNode *buffer) {
    // get a unique label id
    const char *labelId = codeGenGetLabelId(buffer);
    GEN_CHECK(labelId != NULL, -1);
    size_t labelLen = strlen(labelId);

    // generate the header
    const char header[] = "PUSHS bool@true\n"
                          "JUMPIFNEQS %s_ELSE\n";
    size_t headerLen = labelLen + strlen(header);
    char headerBuf[headerLen + 1];
    sprintf(headerBuf, header, labelId);

    // generate the middle
    const char middle[] = "JUMP %s_END\n"
                          "LABEL %s_ELSE\n";
    size_t middleLen = 2 * labelLen + strlen(middle);
    char middleBuf[middleLen + 1];
    sprintf(middleBuf, middle, labelId, labelId);

    // generate the footer
    const char footer[] = "LABEL %s_END\n";
    size_t footerLen = labelLen + strlen(footer);
    char footerBuf[footerLen + 1];
    sprintf(footerBuf, footer, labelId);
    // now can release the hold of labelId and generate the subblocks

    // the condition leaves result on stack
    IFJ_GEN_TRY(codeGenStatement(ifStmt->data.ifStmt.condition, buffer, true),
                -1);
    IFJ_GEN_TRY(codeGenInsertTxt(headerBuf, buffer), -1);

    // then block
    IFJ_GEN_TRY(codeGenStatement(ifStmt->data.ifStmt.thenBlock, buffer, false),
                -1);
    IFJ_GEN_TRY(codeGenInsertTxt(middleBuf, buffer), -1);

    // else block
    IFJ_GEN_TRY(codeGenStatement(ifStmt->data.ifStmt.elseBlock, buffer, false),
                -1);
    IFJ_GEN_TRY(codeGenInsertTxt(footerBuf, buffer), -1);

    return 0;
}

int codeGenStmtWhile(astNode *whileLoop, codeGenBufNode *buffer) {
    // get a unique label id
    const char *labelId = codeGenGetLabelId(buffer);
    GEN_CHECK(labelId != NULL, -1);
    size_t labelLen = strlen(labelId);

    // generate the header
    const char header[] = "LABEL %s_START\n";
    size_t headerLen = labelLen + strlen(header);
    char headerBuf[headerLen + 1];
    sprintf(headerBuf, header, labelId);

    // generate the condition check
    const char condCheck[] = "PUSHS bool@true\n"
                             "JUMPIFNEQS %s_END\n";
    size_t condCheckLen = labelLen + strlen(condCheck);
    char condCheckBuf[condCheckLen + 1];
    sprintf(condCheckBuf, condCheck, labelId);

    // generate the footer
    const char footer[] = "JUMP %s_START\n"
                          "LABEL %s_END\n";
    size_t footerLen = 2 * labelLen + strlen(footer);
    char footerBuf[footerLen + 1];
    sprintf(footerBuf, footer, labelId, labelId);
    // now can release the hold of labelId and generate the subblocks

    IFJ_GEN_TRY(codeGenInsertTxt(headerBuf, buffer), -1);

    // the condition leaves result on stack
    IFJ_GEN_TRY(
        codeGenStatement(whileLoop->data.whileStmt.condition, buffer, true),
        -1);
    IFJ_GEN_TRY(codeGenInsertTxt(condCheckBuf, buffer), -1);

    // body of the loop
    IFJ_GEN_TRY(
        codeGenStatement(whileLoop->data.whileStmt.bodyBlock, buffer, false),
        -1);
    IFJ_GEN_TRY(codeGenInsertTxt(footerBuf, buffer), -1);

    return 0;
}

int codeGenStmtFor(astNode *forLoop, codeGenBufNode *buffer) {
    // get an unique label id (for labels and internal variables)
    const char *uqId = codeGenGetLabelId(buffer);
    size_t uqIdLen = strlen(uqId);

    // get the name of the iterator (will have the depth of the body block)
    int blockDepth = symTableBlockDepth() + 1;
    char iterName[IFJ_GEN_MAX_ID_LEN];
    size_t iterNameLen =
        sprintf(iterName, "DEFVAR LF@%s$%i", forLoop->data.forStmt.iteratorName,
                blockDepth);

    // get and save the header
    /*
     * the stack before adding header to the code will look like:
     * int@step
     * int@end
     * int@start
     *
     * so we pop them in the reverse order
     */
    const char header[] = "DEFVAR %s_STEP_V\n"
                          "POPS %s_STEP_V\n"
                          "DEFVAR %s_END_V\n"
                          "POPS %s_END_V\n"
                          "DEFVAR %s\n"
                          "POPS %s\n"
                          "LABEL %s_START_L\n";
    size_t headerLen = 2 * iterNameLen + 4 * uqIdLen + strlen(header);
    char headerBuf[headerLen + 1];
    sprintf(headerBuf, header, iterName, iterName, uqId, uqId, uqId, uqId,
            uqId);

    // get and save the guard check
    const char guardCheck[] = "PUSHS bool@true\n"
                              "JUMPNEQS %s_ITER_L\n";
    size_t guardCheckLen = uqIdLen + strlen(guardCheck);
    char guardCheckBuf[guardCheckLen + 1];
    sprintf(guardCheckBuf, guardCheck, uqId);

    // get and save the footer
    const char footerEndInclusive[] = "LABEL %s_ITER_L\n"
                                      "ADD %s %s %s_STEP_V\n"
                                      "PUSHS %s\nPUSHS "
                                      "%s_END_V\n"
                                      "LTS\n"
                                      "PUSHS bool@true\n"
                                      "JUMPEQ %s_START_L\n";
    const char footerEndExclusive[] = "LABEL %s_ITER_L\n"
                                      "ADD %s %s %s_STEP_V\n"
                                      "PUSHS %s\n"
                                      "PUSHS %s_END_V\n"
                                      "LTS\n"
                                      "PUSHS %s\n"
                                      "PUSHS %s_END_V\n"
                                      "EQS\n"
                                      "ORS\n"
                                      "PUSHS bool@true\n"
                                      "JUMPEQ %s_START_L\n";

    // set the footerLen as the bigger of the possible two (end exclusive)
    size_t footerLen =
        3 * iterNameLen + 4 * uqIdLen + strlen(footerEndExclusive);

    bool endInclusive = forLoop->data.forStmt.endInclusive;
    char footerBuf[footerLen + 1];

    if (endInclusive) {
        sprintf(footerBuf, footerEndInclusive, uqId, iterName, iterName, uqId,
                iterName, uqId, uqId);
    } else {
        sprintf(footerBuf, footerEndExclusive, uqId, iterName, iterName, uqId,
                iterName, uqId, iterName, uqId, uqId);
    }
    // now we can release hold of the uqId

    // evaluate the start, end and step values and push them to stack
    IFJ_GEN_TRY(codeGenStatement(forLoop->data.forStmt.start, buffer, true),
                -1);
    IFJ_GEN_TRY(codeGenStatement(forLoop->data.forStmt.end, buffer, true), -1);
    IFJ_GEN_TRY(codeGenStatement(forLoop->data.forStmt.step, buffer, true), -1);

    // insert the header
    IFJ_GEN_TRY(codeGenInsertTxt(headerBuf, buffer), -1);

    // insert all the guards
    for (size_t i = 0; i < forLoop->data.forStmt.guardCount; i++) {
        // leave the evaluation of the boolean value on stack
        IFJ_GEN_TRY(
            codeGenStatement(forLoop->data.forStmt.guards[i], buffer, true),
            -1);
        IFJ_GEN_TRY(codeGenInsertTxt(guardCheckBuf, buffer), -1);
    }

    // generate the body of the for loop
    IFJ_GEN_TRY(codeGenStatement(forLoop->data.forStmt.body, buffer, false),
                -1);

    // insert the footer
    IFJ_GEN_TRY(codeGenInsertTxt(footerBuf, buffer), -1);

    return 0;
}

int codeGenStmtFuncCall(astNode *funcCall, codeGenBufNode *buffer,
                        bool leaveVal) {
    // generate the statements from last to first (they leave their value on the
    // stack)
    for (size_t i = funcCall->data.funcCall.argCount; i != 0; i--) {
        IFJ_GEN_TRY(codeGenStatement(funcCall->data.funcCall.arguments[i - 1],
                                     buffer, true),
                    -1);
    }

    int builtin = codeGenBuiltinFunCall(funcCall, buffer);
    if (builtin == -1) {
        return -1;
    }

    if (builtin == 1) {
        // the function wasn't handeled by an inline code - generate a call
        // generate the function call
        const char call[] = "CALL %s\n";
        size_t callLen =
            strlen(call) + strlen(funcCall->data.funcCall.funcName);
        char callBuf[callLen + 1];
        sprintf(callBuf, call, funcCall->data.funcCall.funcName);

        IFJ_GEN_TRY(codeGenInsertTxt(callBuf, buffer), -1);
    }

    // if no result should be left on stack and the function will return one,
    // pop it
    varType retType = symTableFindFun(funcCall->data.funcCall.funcName).retType;
    if (!leaveVal && retType != UNIT) {
        IFJ_GEN_TRY(codeGenInsertTxt("POPS GF@void\n", buffer), -1);
    }

    return 0;
}

int codeGenStmtReturn(astNode *ret, codeGenBufNode *buffer) {
    // generate the RHS if the ptr to it isn't NULL, it will leave its result on
    // the stack
    if (ret->data.returnStmt.value) {
        codeGenStatement(ret->data.returnStmt.value, buffer, true);
    }

    // jump to the end of the function for return
    const char *funName = symTableGetActive().id;
    const char out[] = "JUMP %s$return\n";

    size_t outLen = strlen(out) + strlen(funName);
    char outBuf[outLen + 1];
    sprintf(outBuf, out, funName);

    return 0;
}

int codeGenStmtBinOp(astNode *binOp, codeGenBufNode *buffer) {
    // generate the 2 parameters and leave their values on stack
    IFJ_GEN_TRY(codeGenStatement(binOp->data.binop.left, buffer, true), -1);
    IFJ_GEN_TRY(codeGenStatement(binOp->data.binop.right, buffer, true), -1);

    // generate the operation
    TokenType type;
    switch (type) {
        /*
         //TODO install the correct cases after ast is updated
    case add:
        IFJ_GEN_TRY(codeGenInsertTxt("ADDS\n", buffer), -1);
        break;
    case sub:
        IFJ_GEN_TRY(codeGenInsertTxt("SUBS\n", buffer), -1);
        break;
    case mul:
        IFJ_GEN_TRY(codeGenInsertTxt("MULS\n", buffer), -1);
        break;
    case div:
        IFJ_GEN_TRY(codeGenInsertTxt("DIVS\n", buffer), -1);
        break;
    case idiv:
        IFJ_GEN_TRY(codeGenInsertTxt("IDIVS\n", buffer), -1);
        break;
    case concat:
        IFJ_GEN_TRY(codeGenInsertTxt("POPS GF@G1\n"
                                     "POPS GF@G2\n"
                                     "CONCAT GF@G2 GF@G2 GF@G1\n"
                                     "PUSHS GF@G1\n",
                                     buffer),
                    -1);
        break;
    case equation:
        IFJ_GEN_TRY(codeGenInsertTxt("EQS\n", buffer), -1);
        break;
    case less:
        IFJ_GEN_TRY(codeGenInsertTxt("LTS\n", buffer), -1);
        break;
    case greater:
        IFJ_GEN_TRY(codeGenInsertTxt("GTS\n", buffer), -1);
        break;
    case lesseq:
        IFJ_GEN_TRY(codeGenInsertTxt("GTS\n"
                                     "NOTS\n",
                                     buffer),
                    -1);
        break;
    case greateq:
        IFJ_GEN_TRY(codeGenInsertTxt("LTS\n"
                                     "NOTS\n",
                                     buffer),
                    -1);
        break;
    case and:
        IFJ_GEN_TRY(codeGenInsertTxt("ANDS\n", buffer), -1);
        break;
    case or:
        IFJ_GEN_TRY(codeGenInsertTxt("ORS\n", buffer), -1);
        break;
    */
    default:
        GEN_CHECK_PRINT(false, -1, "Unsupported binary operation\n");
    }
    return 0;
}

int codeGenStmtUnOp(astNode *unOp, codeGenBufNode *buffer) {
    // if the operator is unary - then we also need a prefix
    if (unOp->data.unop.op == TOKEN_MINUS) {
        IFJ_GEN_TRY(codeGenInsertTxt("PUSHS int@0\n", buffer), -1);
    }

    // generate the operand and leave its value on stack
    IFJ_GEN_TRY(codeGenStatement(unOp->data.unop.operand, buffer, true), -1);

    // generate the operator suffix
    switch (unOp->data.unop.op) {
    case TOKEN_NOT:
        IFJ_GEN_TRY(codeGenInsertTxt("NOTS\n", buffer), -1);
        break;
    case TOKEN_PLUS:
        // no further instructions needed
        break;
    case TOKEN_MINUS:
        IFJ_GEN_TRY(codeGenInsertTxt("SUBS\n", buffer), -1);
        break;
    default:
        GEN_CHECK_PRINT(false, -1,
                        "Node set as %i inside an unary operator node. "
                        "Possible unary operators are NOT, PLUS, MINUS",
                        unOp->data.unop.op);
    }

    return 0;
}

int codeGenStmtLiteral(astNode *lit, codeGenBufNode *buffer) {
    // identify the type of literal
    char buf[IFJ_GEN_MAX_ID_LEN];
    switch (lit->data.literal.litType) {
    case TOKEN_INT_LITERAL:
        sprintf(buf, "int@%li\n", lit->data.literal.attribute.intValue);
        break;
    case TOKEN_BOOL_LITERAL:
        sprintf(buf, "bool@%s\n", "true");
        break;
    case TOKEN_DOUBLE_LITERAL:
        sprintf(buf, "float@%a\n", lit->data.literal.attribute.doubleValue);
        break;
    case TOKEN_STRING_LITERAL:
        // call a function to handle the string literal
        return codeGenStmtStringLit(lit->data.literal.attribute.text.data,
                                    lit->data.literal.attribute.text.length,
                                    buffer);
    // TODO maybe unit literal uses a different enum element
    case TOKEN_KW_UNIT:
        // make buf an empty string
        buf[0] = '\0';
        break;
    default:
        GEN_CHECK_PRINT(
            false, -1,
            "A literal at %p doesn't have the litType set as a literal", lit);
    }

    IFJ_GEN_TRY(codeGenInsertTxt(buf, buffer), -1);
    return 0;
}

int codeGenStmtStringLit(const char *str, size_t lenght,
                         codeGenBufNode *buffer) {
    // each character can be replaced with at most 4 characters of escape
    // sequence + lenght of the prefix
    char prefix[] = "string@";
    size_t prefixLen = strlen(prefix);
    char buf[lenght * 4 + prefixLen + 1];

    // insert the prefix
    strcpy(buf, prefix);
    size_t bufIdx = prefixLen;

    for (size_t i = 0; i < lenght; i++) {
        // convert the character to an escape sequence if needed
        char c = str[i];
        if (c <= 32 || c == 35 || c == 92) {
            // set the first character as backslash
            buf[bufIdx] = '\\';

            // convert to decimal
            buf[bufIdx + 3] = c % 10;
            c /= 10;
            buf[bufIdx + 2] = c % 10;
            c /= 10;
            buf[bufIdx + 1] = c;
            bufIdx += 4;
        }
        // else just copy the character
        else {
            buf[bufIdx++] = c;
        }
    }

    IFJ_GEN_TRY(codeGenInsertTxt(buf, buffer), -1);
    return 0;
}

int codeGenStmtIdentif(astNode *identif, codeGenBufNode *buffer) {
    // translate the name to IFJcode26
    const char *name = codeGenGetVarName(identif->data.identifier.name);
    GEN_CHECK(name != NULL, -1);

    // assemble the final string and insert to buffer
    const char prefix[] = "PUSHS %s\n";
    size_t prefixLen = strlen(prefix);
    char buf[prefixLen + IFJ_GEN_MAX_ID_LEN + 1];
    sprintf(buf, prefix, name);

    IFJ_GEN_TRY(codeGenInsertTxt(buf, buffer), -1);
    return 0;
}

int codeGenStmtBlock(astNode *block, codeGenBufNode *buffer) {
    // check if the child block has been generated
    GEN_CHECK_PRINT(block->data.block.bufferPtr != NULL, -1,
                    "A child block at %p wasn't generated yet", block);

    // insert a reference to the child block
    IFJ_GEN_TRY(codeGenInsertBufToTxt(buffer, block->data.block.bufferPtr), -1);
    return 0;
}

int codeGenBuiltinFunCall(astNode *funcCall, codeGenBufNode *buffer) {
    const char *name = funcCall->data.funcCall.funcName;
    if (strcmp(name, "print") == 0) {
        // one value of unknown type should be on stack
        // find a way to be able to check whether the parameter is Unit
        bool isUnitTemp = false;
        if (isUnitTemp) {
            IFJ_GEN_TRY(codeGenInsertTxt("WRITE string@()\n", buffer), -1);
        } else {
            IFJ_GEN_TRY(codeGenInsertTxt("POPS GF@G1\n"
                                         "WRITE GF@G1\n",
                                         buffer),
                        -1);
        }
        return 0;
    }
    if (strcmp(name, "readLine") == 0) {
        // no values should be on stack
        IFJ_GEN_TRY(codeGenInsertTxt("READ GF@G1 string\n"
                                     "PUSHS GF@G1\n",
                                     buffer),
                    -1);
        return 0;
    }
    if (strcmp(name, "readInt") == 0) {
        // no values should be on stack
        IFJ_GEN_TRY(codeGenInsertTxt("READ GF@G1 int\n"
                                     "PUSHS GF@G1\n",
                                     buffer),
                    -1);
        return 0;
    }
    if (strcmp(name, "readDouble") == 0) {
        // no values should be on stack
        IFJ_GEN_TRY(codeGenInsertTxt("READ GF@G1 float\n"
                                     "PUSHS GF@G1\n",
                                     buffer),
                    -1);
        return 0;
    }
    if (strcmp(name, "toStr") == 0) {
        // TODO need to know, what data type is the parameter
        varType typeTemp = INT;
        switch (typeTemp) {
        case INT:
            IFJ_GEN_TRY(codeGenInsertTxt("INT2STRS\n", buffer), -1);
            break;
        case DOUBLE:
            IFJ_GEN_TRY(codeGenInsertTxt("FLOAT2STRS\n", buffer), -1);
            break;
        case STRING:
            return 0; // nothing needs to change, keep the value on stack
        case BOOL:
            // TODO decide whether to handle by insert or a call
            break;
        case UNIT:
            IFJ_GEN_TRY(codeGenInsertTxt("POPS GF@void\n"
                                         "PUSHS srinng@()\n",
                                         buffer),
                        -1);
            break;
        default:
            // should not happen, check
            return -1;
        }
        return 0;
    }
    if (strcmp(name, "toInt") == 0) {
        // one value of type float should be on stack
        IFJ_GEN_TRY(codeGenInsertTxt("FLOAT2INTS\n", buffer), -1);
        return 0;
    }
    if (strcmp(name, "StrLength") == 0) {
        // one value of type string should be on stack
        IFJ_GEN_TRY(codeGenInsertTxt("POPS GF@G1\n"
                                     "STRLEN GF@G1 GF@G1\n"
                                     "PUSHS GF@G1\n",
                                     buffer),
                    -1);
        return 0;
    }
    if (strcmp(name, "chr") == 0) {
        // one value of type int should be on stack
        // no error handeling left to the interpretor of IFJcode26
        IFJ_GEN_TRY(codeGenInsertTxt("POPS GF@G1\n"
                                     "INT2CHAR GF@G1 GF@G1\n"
                                     "PUSHS GF@G1\n",
                                     buffer),
                    -1);
        return 0;
    }
    // the rest of the builtin functions are handeled by a call => return -1
    return 1;
}

const char *codeGenGetVarName(const char *id) {
    static char nameBuf[IFJ_GEN_MAX_ID_LEN];

    int uniqueId;
    // find the name in symTable
    varData nameData = symTableFindVar(id, &uniqueId);
    GEN_CHECK_PRINT(nameData.id != NULL, NULL,
                    "The variable name %s not found in symtable\n", id);

    // construct the name in IFJ26
    sprintf(nameBuf, "LF@%s$%i", id, uniqueId);
    return nameBuf;
}

const char *codeGenGetLabelId(codeGenBufNode *buffer) {
    static char idBuf[IFJ_GEN_MAX_ID_LEN];

    // find the function name and block depth in symTable (should be the active
    // function)
    const char *funName = symTableGetActive().id;
    GEN_CHECK_PRINT(funName != NULL, NULL, "No function is active\n");
    int blockDepth = symTableBlockDepth();

    int inBlockUnique = buffer->labelNum++;

    sprintf(idBuf, "%s$%i$%i", funName, blockDepth, inBlockUnique);
    return idBuf;
}

/*
 * end generator.c
 */
