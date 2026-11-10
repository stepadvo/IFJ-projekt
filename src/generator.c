/**
 * @file generator.c
 * @brief implementation of IFJcode26 generator
 * @author Štěpán Dvořák, xdvoras00@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include "generator.h"
#include "ast.h"
#include "symtable.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef IFJ_GENERATOR_DEBUG
#else
#endif

codeGenState *codeGenInit() {
    static codeGenState state = {NULL, NULL, NULL, NULL};
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

int codeGenBlock(codeGenState *state, astNode *blockRoot);

int codeGenFunction(codeGenState *state, astNode *funRoot);

int codeGenOutCode(codeGenState *state) {
    outBufNode *next;
    int len = 0;

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
}

/*
 * --------------------
 *  INTERNAL FUNCTIONS
 * --------------------
 */

void codeGenDestructBuf(codeGenBufNode *buf) {
    // free all the metadata
    bufSectionNode *nextNode;
    for (bufSectionNode *curr = buf->firstNode; curr != NULL; curr = nextNode) {
        nextNode = curr->cont;
        free(curr);
    }

    // free all the text areas
    bufSectionArea *nextArea;
    for (bufSectionArea *curr = buf->firstArea; curr != NULL; curr = nextArea) {
        nextArea = curr->cont;
        free(curr);
    }

    free(buf);
}

/*
 * end generator.c
 */
