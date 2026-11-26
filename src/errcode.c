/**
 * @file compiler.c
 * @brief a top level file for the most basic routines
 * @author Štěpán Dvořák, xdvoras@stud.fit.vutbr.cz
 *
 * detailed description
 */

#include "compiler.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int ERRCODE;

void setErrCode(int errCode) {
    if (!ERRCODE)
        ERRCODE = errCode;
}

void logErr(errType type, size_t line, size_t column,
            const char *detailMsgFormat, ...) {
    // set the ERRCODE depending on the type
    setErrCode(type);

    // decide on the message depending on the error type
    char *message = "";
    switch (type) {
    case ERR_LEXER:
        message = "lexical error";
        break;
    case ERR_SYNTAX:
        message = "syntactical error";
        break;
    case ERR_SEM_UNDEF:
        message = "definition error";
        break;
    case ERR_SEM_REDEF:
        message = "redefiniton error";
        break;
    case ERR_SEM_ARG:
        message = "function argument error";
        break;
    case ERR_SEM_TYPE:
        message = "data type error";
        break;
    case ERR_SEM_RONLY:
        message = "modyfying val error";
        break;
    case ERR_SEM_OTHER:
        message = "semantic error";
        break;
    case ERR_INTERNAL:
        // if error is internal, write only the brief message
        fprintf(stderr, "%s: internal compiler error\n", __FILE_NAME__);
        return;
    }

    // decide whether line and column are shown and
    if (line != 0) {
        if (column != 0) {
            fprintf(stderr, "%s:%lu:%lu: %s\n", __FILE_NAME__, line, column,
                    message);
        } else {
            fprintf(stderr, "%s:%lu: %s\n", __FILE_NAME__, line, message);
        }
    } else {
        fprintf(stderr, "%s: %s\n", __FILE_NAME__, message);
    }

    // if detailed message is shown, write it to stderr
    if (detailMsgFormat != NULL) {
        // indent all the lines of the message
        bool newline = true;
        int pos = 0;
        // set lenght the maximum possible lenght of the new format
        char format[2 * strlen(detailMsgFormat)];

        for (size_t i = 0; i <= strlen(detailMsgFormat); i++) {
            if (newline) {
                format[pos++] = '\t';
            }

            newline = detailMsgFormat[i] == '\n';
            format[pos++] = detailMsgFormat[i];
        }
        // print out the format
        va_list list;
        va_start(list, detailMsgFormat);
        vfprintf(stderr, format, list);
    }
}

/*
 * end compiler.c
 */
