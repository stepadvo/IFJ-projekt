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
 * to work are met*/
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
    static codeGenState state = {.firstBuf = NULL,
                                 .lastBuf = NULL,
                                 .bufsCreated = 0,
                                 .firstFun = NULL,
                                 .lastFun = NULL,
                                 .mainFunName = ""};
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
    // TODO
    (void)state;
    (void)funRoot;
    return 0;
}

int codeGenOutCode(codeGenState *state) {
    // output the file header
    int len = printf(".IFJcode26\n"
                     "CALL %s\n"
                     "EXIT int@0\n",
                     state->mainFunName);

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

void codeGenDestructBuf(codeGenBufNode *buf) {
    if (!buf) {
        return;
    }
    // free all the metadata
    bufInsertNode *nextNode;
    for (bufInsertNode *curr = buf->firstNode; curr != NULL; curr = nextNode) {
        nextNode = curr->next;
        free(curr);
    }

    // free all the text areas
    bufSection *nextSection;
    for (bufSection *curr = buf->firstSection; curr != NULL;
         curr = nextSection) {
        nextSection = curr->next;
        free(curr);
    }

    free(buf);
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

    return 0;
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
    const char *name = codeGenGetVarName(varDef->data.varDef.name, buffer);
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
    const char *name = codeGenGetVarName(assign->data.assign.name, buffer);
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
    (void)funcCall;
    (void)buffer;
    (void)leaveVal;
    return 0;
}

int codeGenStmtReturn(astNode *ret, codeGenBufNode *buffer) {
    // generate the RHS if the ptr to it isn't NULL, it will leave its result on
    // the stack
    if (ret->data.returnStmt.value) {
        codeGenStatement(ret->data.returnStmt.value, buffer, true);
    }

    const char *funName = symTableGetActive().id;
    const char out[] = "POPS retval$-1\n"
                       "JUMP %s$return\n";

    size_t outLen = strlen(out) + strlen(funName);
    char outBuf[outLen + 1];
    sprintf(outBuf, out, funName);

    return 0;
}

int codeGenStmtBinOp(astNode *binOp, codeGenBufNode *buffer) {
    (void)binOp;
    (void)buffer;
    return 0;
}

int codeGenStmtUnOp(astNode *unOp, codeGenBufNode *buffer) {
    (void)unOp;
    (void)buffer;
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
    /*
    TODO inquire about unit literal
    case UNIT:
        memcpy(buf, "nil@nil", 4);
        break;
     */
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
    const char *name = codeGenGetVarName(identif->data.identifier.name, buffer);
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

const char *codeGenGetVarName(const char *id, codeGenBufNode *buffer) {
    static char nameBuf[IFJ_GEN_MAX_ID_LEN];

    // find the name in symTable
    varData nameData = symTableFindVar(id);
    GEN_CHECK_PRINT(nameData.id != NULL, NULL,
                    "The variable name %s not found in symtable\n", id);

    // construct the name in IFJ26
    sprintf(nameBuf, "LF@%s$%i$%i", id, nameData.blockDepth, buffer->bufNum);
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
